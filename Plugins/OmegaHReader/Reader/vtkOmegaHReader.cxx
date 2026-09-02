// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOmegaHReader.h"

#include "vtk_zlib.h" // for uncompress()

#include "vtkAbstractArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkDummyController.h"
#include "vtkFileResourceStream.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongLongArray.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkResourceStream.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

vtkStandardNewMacro(vtkOmegaHReader);

namespace
{

//-----------------------------------------------------------------------------
// Constants from omega_h's src/Omega_h_defines.hpp and src/Omega_h_file.cpp
// (format version 11 is the latest; versions 9-11 are supported here).

constexpr unsigned char kMagic[2] = { 0xa1, 0x1a };

// omega_h tag types: only these four exist.
constexpr unsigned char kTagI8 = 0;
constexpr unsigned char kTagI32 = 2;
constexpr unsigned char kTagI64 = 3;
constexpr unsigned char kTagF64 = 5;

// omega_h element families.
constexpr unsigned char kFamilySimplex = 0;
constexpr unsigned char kFamilyHypercube = 1;
constexpr unsigned char kFamilyMixed = 2;

// The marker string written before the class ids of every tag (version > 9).
constexpr char kClassIdsMarker[] = "n_geom_ents";

// Latest format version (omega_h's src/Omega_h_file.hpp latest_version).
constexpr int kLatestVersion = 11;

bool IsLittleEndianCpu()
{
  const std::uint16_t canary = 0x1;
  return *reinterpret_cast<const unsigned char*>(&canary) == 0x1;
}

// The .osh streams are little-endian; swap on big-endian CPUs (as omega_h
// itself does in Omega_h_file.cpp).
const bool kNeedsSwap = !IsLittleEndianCpu();

template <typename T>
void SwapBytes(T* value)
{
  auto* p = reinterpret_cast<unsigned char*>(value);
  for (std::size_t i = 0; i < sizeof(T) / 2; ++i)
  {
    std::swap(p[i], p[sizeof(T) - 1 - i]);
  }
}

template <typename T>
void SwapBytesIfNeeded(T* values, std::size_t count)
{
  if (!kNeedsSwap)
  {
    return;
  }
  for (std::size_t i = 0; i < count; ++i)
  {
    SwapBytes(&values[i]);
  }
}

//-----------------------------------------------------------------------------
// A little-endian binary stream with omega_h's array framing:
// array: [I32 size][if compressed: I64 byte count + zlib stream | raw bytes].
// (Omega_h_file.cpp read_array / write_array)

class Stream
{
public:
  explicit Stream(vtkResourceStream* stream)
    : In(stream)
  {
  }

  template <typename T>
  bool Value(T& value)
  {
    if (this->In->Read(&value, sizeof(T)) != sizeof(T))
    {
      return false;
    }
    SwapBytesIfNeeded(&value, 1);
    return true;
  }

  bool Bytes(void* data, std::size_t count) { return this->In->Read(data, count) == count; }

  // Probe for trailing bytes: a read of zero means the stream is exhausted
  // (vtkResourceStream has no peek; a one-byte read is the equivalent probe).
  bool AtEnd()
  {
    char c;
    return this->In->Read(&c, 1) == 0;
  }

  bool Str(std::string& out, std::string& error)
  {
    std::int32_t length = 0;
    if (!this->Value(length) || length < 0)
    {
      error = "invalid string length in .osh stream";
      return false;
    }
    out.resize(static_cast<std::size_t>(length));
    if (length > 0 && !this->Bytes(&out[0], length))
    {
      error = "truncated string in .osh stream";
      return false;
    }
    return true;
  }

  template <typename T>
  bool Array(std::vector<T>& out, bool compressed, std::string& error)
  {
    std::int32_t size = 0;
    if (!this->Value(size) || size < 0)
    {
      error = "invalid array size in .osh stream";
      return false;
    }
    out.resize(static_cast<std::size_t>(size));
    if (size == 0)
    {
      return true;
    }
    if (compressed)
    {
      std::int64_t nbytes = 0;
      if (!this->Value(nbytes) || nbytes < 0)
      {
        error = "invalid compressed array size in .osh stream";
        return false;
      }
      std::vector<char> buffer(static_cast<std::size_t>(nbytes));
      if (nbytes > 0 && !this->Bytes(buffer.data(), nbytes))
      {
        error = "truncated compressed array in .osh stream";
        return false;
      }
      uLongf destLen = static_cast<uLongf>(size) * sizeof(T);
      const int ret = ::uncompress(reinterpret_cast<Bytef*>(out.data()), &destLen,
        reinterpret_cast<const Bytef*>(buffer.data()), static_cast<uLong>(nbytes));
      if (ret != Z_OK || destLen != static_cast<uLongf>(size) * sizeof(T))
      {
        error = "failed to decompress array in .osh stream (size " + std::to_string(size) + ", " +
          std::to_string(nbytes) + " compressed bytes, zlib status " + std::to_string(ret) + ")";
        return false;
      }
    }
    else if (!this->Bytes(out.data(), static_cast<std::size_t>(size) * sizeof(T)))
    {
      error = "truncated array in .osh stream";
      return false;
    }
    SwapBytesIfNeeded(out.data(), out.size());
    return true;
  }

private:
  vtkSmartPointer<vtkResourceStream> In;
};

//-----------------------------------------------------------------------------
// A single mesh part after parsing (see ReadPart).

struct Tag
{
  std::string Name;
  unsigned char NComps = 0;
  unsigned char Type = 0;
  std::vector<char> Data; // raw bytes (host order after byte swapping)
  std::size_t Count = 0;  // number of values (tuples * components)
};

struct Part
{
  unsigned char Family = kFamilySimplex;
  int Dim = 0;
  int CommSize = 1;
  int CommRank = 0;
  vtkIdType NVerts = 0;
  std::vector<std::vector<int>> Down;            // Down[d] = ab2b for d=1..dim
  std::vector<std::vector<unsigned char>> Codes; // Codes[d] for d=2..dim
  std::vector<std::vector<Tag>> Tags;            // Tags[d] for d=0..dim
  std::vector<std::vector<int>> OwnerRanks;      // per dim; empty if CommSize == 1
};

bool ReadPart(
  vtkResourceStream* input, int version, const std::string& source, Part& part, std::string& error)
{
  Stream stream(input);
  unsigned char magic[2] = { 0, 0 };
  if (!stream.Bytes(magic, sizeof(magic)) || magic[0] != kMagic[0] || magic[1] != kMagic[1])
  {
    error = "\"" + source + "\" is not an omega_h mesh part (bad magic)";
    return false;
  }
  unsigned char isCompressed = 0;
  if (!stream.Value(isCompressed))
  {
    error = "truncated .osh stream: " + source;
    return false;
  }

  // meta (Omega_h_file.cpp read_meta): family, dim, comm_size, comm_rank,
  // parting, nghost_layers, rib hints. Parting and hints are informational
  // for rendering and are skipped. Note: family and dim are stored as I8,
  // so they must be read into single-byte values.
  unsigned char dim = 0;
  unsigned char parting = 0;
  std::int32_t commSize = 0;
  std::int32_t commRank = 0;
  std::int32_t nghostLayers = 0;
  unsigned char haveHints = 0;
  if (!stream.Value(part.Family) || !stream.Value(dim) || !stream.Value(commSize) ||
    !stream.Value(commRank) || !stream.Value(parting) || !stream.Value(nghostLayers) ||
    !stream.Value(haveHints))
  {
    error = "truncated .osh meta: " + source;
    return false;
  }
  part.Dim = dim;
  part.CommSize = commSize;
  part.CommRank = commRank;
  if (haveHints)
  {
    std::int32_t naxes = 0;
    if (!stream.Value(naxes) || naxes < 0)
    {
      error = "invalid rib hint count in .osh stream: " + source;
      return false;
    }
    for (std::int32_t i = 0; i < naxes; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        std::int64_t axisValue = 0;
        if (!stream.Value(axisValue))
        {
          error = "truncated .osh rib hints: " + source;
          return false;
        }
      }
    }
  }

  std::int32_t nverts = 0;
  if (!stream.Value(nverts) || nverts < 0)
  {
    error = "invalid vertex count in .osh stream: " + source;
    return false;
  }
  part.NVerts = nverts;

  part.Down.assign(part.Dim + 1, {});
  part.Codes.assign(part.Dim + 1, {});
  for (int d = 1; d <= part.Dim; ++d)
  {
    if (!stream.Array(part.Down[d], isCompressed != 0, error))
    {
      return false;
    }
    if (d > 1 && !stream.Array(part.Codes[d], isCompressed != 0, error))
    {
      return false;
    }
  }

  part.Tags.assign(part.Dim + 1, {});
  part.OwnerRanks.assign(part.Dim + 1, {});
  for (int d = 0; d <= part.Dim; ++d)
  {
    std::int32_t ntags = 0;
    if (!stream.Value(ntags) || ntags < 0)
    {
      error = "invalid tag count in .osh stream: " + source;
      return false;
    }
    for (std::int32_t i = 0; i < ntags; ++i)
    {
      Tag tag;
      if (!stream.Str(tag.Name, error) || !stream.Value(tag.NComps) || !stream.Value(tag.Type))
      {
        return false;
      }
      if (tag.NComps == 0)
      {
        error = "tag \"" + tag.Name + "\" has zero components in .osh stream: " + source;
        return false;
      }
      if (version > 10)
      {
        std::string arrayTypeName; // e.g. "VectorND"; not needed for rendering
        if (!stream.Str(arrayTypeName, error))
        {
          return false;
        }
      }
      if (version > 9)
      {
        std::string marker;
        if (!stream.Str(marker, error))
        {
          return false;
        }
        if (marker != kClassIdsMarker)
        {
          error = "unexpected tag marker \"" + marker + "\" in .osh stream: " + source;
          return false;
        }
        std::int32_t nclassIds = 0;
        if (!stream.Value(nclassIds) || nclassIds < 0)
        {
          return false;
        }
        if (nclassIds > 0)
        {
          std::vector<std::int32_t> classIds; // not needed for rendering
          if (!stream.Array(classIds, isCompressed != 0, error))
          {
            return false;
          }
        }
      }
      // The data array must be read with the tag's element type: the stored
      // size counts values, and the decompressed byte count is
      // size * sizeof(element type).
      std::vector<char> data;
      switch (tag.Type)
      {
        case kTagI8:
        {
          std::vector<std::int8_t> v;
          if (!stream.Array(v, isCompressed != 0, error))
          {
            return false;
          }
          tag.Count = v.size();
          data.assign(reinterpret_cast<const char*>(v.data()),
            reinterpret_cast<const char*>(v.data()) + v.size());
          break;
        }
        case kTagI32:
        {
          std::vector<std::int32_t> v;
          if (!stream.Array(v, isCompressed != 0, error))
          {
            return false;
          }
          tag.Count = v.size();
          data.assign(reinterpret_cast<const char*>(v.data()),
            reinterpret_cast<const char*>(v.data()) + v.size() * sizeof(std::int32_t));
          break;
        }
        case kTagI64:
        {
          std::vector<std::int64_t> v;
          if (!stream.Array(v, isCompressed != 0, error))
          {
            return false;
          }
          tag.Count = v.size();
          data.assign(reinterpret_cast<const char*>(v.data()),
            reinterpret_cast<const char*>(v.data()) + v.size() * sizeof(std::int64_t));
          break;
        }
        case kTagF64:
        {
          std::vector<double> v;
          if (!stream.Array(v, isCompressed != 0, error))
          {
            return false;
          }
          tag.Count = v.size();
          data.assign(reinterpret_cast<const char*>(v.data()),
            reinterpret_cast<const char*>(v.data()) + v.size() * sizeof(double));
          break;
        }
        default:
          error = "unsupported tag type " + std::to_string(tag.Type) + " for tag \"" + tag.Name +
            "\" in " + source;
          return false;
      }
      tag.Data = std::move(data);
      part.Tags[d].push_back(std::move(tag));
    }
    if (part.CommSize > 1)
    {
      if (!stream.Array(part.OwnerRanks[d], isCompressed != 0, error))
      {
        return false;
      }
      // NOTE: in this omega_h fork Remotes::idxs is LOs (I32), unlike
      // upstream omega_h where it is GO (I64). Not needed for rendering.
      std::vector<std::int32_t> ownerIdxs;
      if (!stream.Array(ownerIdxs, isCompressed != 0, error))
      {
        return false;
      }
    }
  }

  if (version >= 8)
  {
    // class sets (Omega_h_file.cpp read_sets): parse and skip.
    std::int32_t nsets = 0;
    if (!stream.Value(nsets) || nsets < 0)
    {
      error = "invalid class set count in .osh stream: " + source;
      return false;
    }
    for (std::int32_t i = 0; i < nsets; ++i)
    {
      std::string name;
      std::int32_t npairs = 0;
      if (!stream.Str(name, error) || !stream.Value(npairs) || npairs < 0)
      {
        return false;
      }
      for (std::int32_t j = 0; j < npairs; ++j)
      {
        std::int32_t pairDim = 0;
        std::int32_t pairId = 0;
        if (!stream.Value(pairDim) || !stream.Value(pairId))
        {
          return false;
        }
      }
    }
  }
  if (version >= 9)
  {
    // parent elements: parse and skip.
    unsigned char hasParents = 0;
    if (!stream.Value(hasParents))
    {
      error = "truncated .osh parents: " + source;
      return false;
    }
    if (hasParents)
    {
      for (int d = 0; d <= part.Dim; ++d)
      {
        std::vector<std::int32_t> parentIdx;
        std::vector<unsigned char> parentCodes;
        if (!stream.Array(parentIdx, isCompressed != 0, error) ||
          !stream.Array(parentCodes, isCompressed != 0, error))
        {
          return false;
        }
      }
    }
  }
  if (!stream.AtEnd())
  {
    // trailing data usually means a format/version mismatch in this reader
    error = "unparsed trailing data in .osh stream: " + source;
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
// Element templates (dumped from omega_h's Omega_h_element.hpp against the
// omega_h installation; see Omega_h_simplex.hpp / Omega_h_hypercube.hpp).
// Type ids: 2=triangle, 3=quadrilateral, 4=tetrahedron, 5=hexahedron.

struct UpVert
{
  int Up;        // edge slot within the element
  int WhichDown; // vertex slot within that edge
};

constexpr UpVert kUpVertsTri[3] = { { 0, 0 }, { 1, 0 }, { 2, 0 } };
constexpr UpVert kUpVertsQuad[4] = { { 3, 1 }, { 0, 1 }, { 1, 1 }, { 2, 1 } };
constexpr UpVert kUpVertsTet[4] = { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 5, 1 } };
constexpr UpVert kUpVertsHex[8] = { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 }, { 8, 0 }, { 9, 0 },
  { 10, 0 }, { 11, 0 } };

struct UpEdge
{
  int Up;        // face slot within the element
  int WhichDown; // edge slot within that face
  int Flipped;   // whether the edge is flipped w.r.t. its canonical direction
};

constexpr UpEdge kUpEdgesTet[6] = { { 0, 2, 1 }, { 0, 1, 1 }, { 0, 0, 1 }, { 1, 2, 1 }, { 2, 2, 1 },
  { 3, 2, 1 } };
constexpr UpEdge kUpEdgesHex[12] = { { 0, 0, 1 }, { 0, 3, 1 }, { 0, 2, 1 }, { 0, 1, 1 },
  { 1, 3, 1 }, { 2, 3, 1 }, { 3, 3, 1 }, { 4, 3, 1 }, { 5, 0, 0 }, { 5, 1, 0 }, { 5, 2, 0 },
  { 5, 3, 0 } };

// Number of (dim-1)-entities per element, i.e. the constant ab2b degree per
// element used by the file layout (element_degree in omega_h).
int DownDegree(unsigned char family, int dim)
{
  if (dim == 1)
  {
    return 2;
  }
  if (family == kFamilySimplex)
  {
    return dim == 2 ? 3 : 4;
  }
  // hypercube (and best-effort MIXED): 2*d
  return dim == 2 ? 4 : 6;
}

// Number of vertices per element (element_degree(family, dim, 0)).
int NVertices(unsigned char family, int dim)
{
  if (dim == 0)
  {
    return 1;
  }
  if (dim == 1)
  {
    return 2;
  }
  if (family == kFamilySimplex)
  {
    return dim == 2 ? 3 : 4;
  }
  return dim == 2 ? 4 : 8;
}

// Uniform families only; MIXED is handled separately (best effort).
int TypeId(unsigned char family, int dim)
{
  if (dim == 2)
  {
    return family == kFamilySimplex ? 2 : 3;
  }
  if (dim == 3)
  {
    return family == kFamilySimplex ? 4 : 5;
  }
  return 0;
}

const UpVert* UpVerts(int type)
{
  switch (type)
  {
    case 2:
      return kUpVertsTri;
    case 3:
      return kUpVertsQuad;
    case 4:
      return kUpVertsTet;
    case 5:
      return kUpVertsHex;
    default:
      return nullptr;
  }
}

const UpEdge* UpEdges(int type)
{
  return type == 4 ? kUpEdgesTet : kUpEdgesHex;
}

//-----------------------------------------------------------------------------
// Element -> vertex composition. This is an exact replica of omega_h's
// Mesh::ask_verts_of (Omega_h_adj.cpp transit()), producing the vertices of
// every element in omega_h's canonical local order, which matches the VTK
// node order for LINE/TRIANGLE/QUAD/TETRA/HEXAHEDRON.

// For edges (n=2) flipping a vertex index is the identity and
// invert_alignment preserves the rotation, so align_vert_index collapses to
// a rotation (Omega_h_align.hpp).
int AlignVertIndex(int index, int rotation)
{
  return (index + rotation) & 1;
}

// align_edge_index(n, index, code) with invert_alignment(n, code) folded in.
int AlignEdgeIndex(int n, int index, unsigned char code)
{
  const int flipped = code & 1;
  const int rot = (code >> 1) & 3;
  const int effRot = flipped ? rot : ((n - rot) % n);
  const int r = (index + effRot) % n;
  return flipped ? (n - 1 - r) : r;
}

std::vector<vtkIdType> ElementVerts(const Part& part, vtkIdType e)
{
  const int dim = part.Dim;
  if (dim == 0)
  {
    return { e };
  }
  if (dim == 1)
  {
    return { part.Down[1][2 * e], part.Down[1][2 * e + 1] };
  }
  if (part.Family == kFamilyMixed)
  {
    // MIXED-family .osh files are not produced by omega_h's own writer (its
    // reader also assumes a constant degree per level), so this is best
    // effort: assume the hypercube degree layout, compose down the chain and
    // keep the first occurrence of every vertex.
    std::vector<vtkIdType> ids(1, e);
    for (int d = dim; d >= 1; --d)
    {
      const int deg = DownDegree(kFamilyHypercube, d);
      std::vector<vtkIdType> next;
      next.reserve(ids.size() * deg);
      for (const vtkIdType id : ids)
      {
        for (int u = 0; u < deg; ++u)
        {
          next.push_back(part.Down[d][id * deg + u]);
        }
      }
      ids.swap(next);
    }
    std::vector<vtkIdType> verts;
    for (const vtkIdType v : ids)
    {
      if (std::find(verts.begin(), verts.end(), v) == verts.end())
      {
        verts.push_back(v);
      }
    }
    return verts;
  }
  const int type = TypeId(part.Family, dim);
  const UpVert* upv = UpVerts(type);
  const int nv = NVertices(part.Family, dim);
  if (dim == 2)
  {
    // the file stores element->edge directly: single transit to vertices.
    std::vector<vtkIdType> verts(nv);
    for (int hl = 0; hl < nv; ++hl)
    {
      const vtkIdType m = part.Down[2][e * nv + upv[hl].Up];
      const int rot = (part.Codes[2][e * nv + upv[hl].Up] >> 1) & 3;
      verts[hl] = part.Down[1][m * 2 + AlignVertIndex(upv[hl].WhichDown, rot)];
    }
    return verts;
  }
  // dim == 3: transit(region->face, face->edge), then transit(edge->vert).
  // The intermediate region->edge codes carry the edge flip in their
  // rotation bits (make_code(false, flipped, 0)), which the second transit
  // applies to the vertex slot.
  const UpEdge* upe = UpEdges(type);
  const int nedges = (type == 4) ? 6 : 12;
  const int nfaces = (type == 4) ? 4 : 6;
  const int n = (type == 4) ? 3 : 4; // edges per face
  std::vector<vtkIdType> edges(nedges);
  std::vector<int> edgeRot(nedges);
  for (int hl = 0; hl < nedges; ++hl)
  {
    const vtkIdType face = part.Down[3][e * nfaces + upe[hl].Up];
    const unsigned char rfc = part.Codes[3][e * nfaces + upe[hl].Up];
    const int ml = AlignEdgeIndex(n, upe[hl].WhichDown, rfc);
    const unsigned char fec = part.Codes[2][face * n + ml];
    // region_edge_flipped = region_face_flipped ^ face_edge_flipped
    //                       ^ canon_flipped (Omega_h_adj.cpp transit)
    edgeRot[hl] = (rfc & 1) ^ (((fec >> 1) & 3) == 1) ^ upe[hl].Flipped;
    edges[hl] = part.Down[2][face * n + ml];
  }
  std::vector<vtkIdType> verts(nv);
  for (int hl = 0; hl < nv; ++hl)
  {
    verts[hl] =
      part.Down[1][edges[upv[hl].Up] * 2 + AlignVertIndex(upv[hl].WhichDown, edgeRot[upv[hl].Up])];
  }
  return verts;
}

//-----------------------------------------------------------------------------
// Conversion of one part to a vtkUnstructuredGrid.

const Tag* FindTag(const Part& part, int dim, const std::string& name)
{
  for (const Tag& tag : part.Tags[dim])
  {
    if (tag.Name == name)
    {
      return &tag;
    }
  }
  return nullptr;
}

vtkSmartPointer<vtkAbstractArray> TagToArray(const Tag& tag)
{
  vtkSmartPointer<vtkAbstractArray> array;
  switch (tag.Type)
  {
    case kTagI8:
      array = vtkSmartPointer<vtkSignedCharArray>::New();
      break;
    case kTagI32:
      array = vtkSmartPointer<vtkIntArray>::New();
      break;
    case kTagI64:
      array = vtkSmartPointer<vtkLongLongArray>::New();
      break;
    case kTagF64:
      array = vtkSmartPointer<vtkDoubleArray>::New();
      break;
    default:
      return nullptr;
  }
  array->SetName(tag.Name.c_str());
  array->SetNumberOfComponents(tag.NComps);
  array->SetNumberOfTuples(static_cast<vtkIdType>(tag.Count / tag.NComps));
  std::copy(tag.Data.begin(), tag.Data.end(), reinterpret_cast<char*>(array->GetVoidPointer(0)));
  return array;
}

vtkSmartPointer<vtkUnstructuredGrid> BuildGrid(const Part& part, std::string& error)
{
  const int dim = part.Dim;
  vtkNew<vtkUnstructuredGrid> grid;

  // points: the "coordinates" tag (F64, ncomps == dim) on dimension 0.
  // 1D/2D coordinates are padded with zeros to 3D.
  const Tag* coords = FindTag(part, 0, "coordinates");
  if (!coords || coords->Type != kTagF64 || coords->NComps != dim ||
    coords->Count != static_cast<std::size_t>(part.NVerts) * dim)
  {
    error = "mesh has no valid \"coordinates\" tag";
    return nullptr;
  }
  const double* coordsData = reinterpret_cast<const double*>(coords->Data.data());
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(part.NVerts);
  double xyz[3] = { 0.0, 0.0, 0.0 };
  for (vtkIdType v = 0; v < part.NVerts; ++v)
  {
    for (int c = 0; c < dim; ++c)
    {
      xyz[c] = coordsData[v * dim + c];
    }
    points->SetPoint(v, xyz);
  }
  grid->SetPoints(points);

  // cells
  int cellType = VTK_EMPTY_CELL;
  if (dim == 0)
  {
    cellType = VTK_VERTEX;
  }
  else if (part.Family == kFamilySimplex)
  {
    cellType = dim == 1 ? VTK_LINE : (dim == 2 ? VTK_TRIANGLE : VTK_TETRA);
  }
  else if (part.Family == kFamilyHypercube)
  {
    cellType = dim == 1 ? VTK_LINE : (dim == 2 ? VTK_QUAD : VTK_HEXAHEDRON);
  }
  // MIXED: per-cell type derived from the composed vertex count below.

  const int degree = DownDegree(part.Family, dim);
  if (dim > 0 && part.Down[dim].size() % degree != 0)
  {
    error = "inconsistent element connectivity in part";
    return nullptr;
  }
  const vtkIdType nelems =
    dim == 0 ? part.NVerts : static_cast<vtkIdType>(part.Down[dim].size() / degree);
  vtkNew<vtkCellArray> cells;
  for (vtkIdType e = 0; e < nelems; ++e)
  {
    const std::vector<vtkIdType> verts = ElementVerts(part, e);
    int thisCellType = cellType;
    if (part.Family == kFamilyMixed)
    {
      thisCellType = dim == 2
        ? (verts.size() == 3 ? VTK_TRIANGLE : (verts.size() == 4 ? VTK_QUAD : VTK_POLYGON))
        : (verts.size() == 4
              ? VTK_TETRA
              : (verts.size() == 5
                    ? VTK_PYRAMID
                    : (verts.size() == 6 ? VTK_WEDGE
                                         : (verts.size() == 8 ? VTK_HEXAHEDRON : VTK_POLYHEDRON))));
      if (thisCellType == VTK_POLYHEDRON)
      {
        error = "unsupported MIXED element with " + std::to_string(verts.size()) + " vertices";
        return nullptr;
      }
    }
    cells->InsertNextCell(static_cast<vtkIdType>(verts.size()), verts.data());
  }
  grid->SetCells(cellType, cells);

  // point data (dim 0 tags) and cell data (dim tags)
  for (const Tag& tag : part.Tags[0])
  {
    if (tag.Name == "coordinates")
    {
      continue;
    }
    grid->GetPointData()->AddArray(TagToArray(tag));
  }
  if (dim > 0)
  {
    for (const Tag& tag : part.Tags[dim])
    {
      grid->GetCellData()->AddArray(TagToArray(tag));
    }
  }

  // ghost marking: an entity whose owner rank differs from the part's writer
  // rank is a duplicate (vtkGhostType). Only present for multi-part meshes.
  if (!part.OwnerRanks[0].empty() &&
    static_cast<vtkIdType>(part.OwnerRanks[0].size()) == part.NVerts)
  {
    vtkNew<vtkUnsignedCharArray> ghostPoints;
    ghostPoints->SetName(vtkDataSetAttributes::GhostArrayName());
    ghostPoints->SetNumberOfTuples(part.NVerts);
    for (vtkIdType v = 0; v < part.NVerts; ++v)
    {
      ghostPoints->SetValue(
        v, part.OwnerRanks[0][v] != part.CommRank ? vtkDataSetAttributes::DUPLICATEPOINT : 0);
    }
    grid->GetPointData()->AddArray(ghostPoints);
  }
  if (dim > 0 && !part.OwnerRanks[dim].empty() &&
    static_cast<vtkIdType>(part.OwnerRanks[dim].size()) == nelems)
  {
    vtkNew<vtkUnsignedCharArray> ghostCells;
    ghostCells->SetName(vtkDataSetAttributes::GhostArrayName());
    ghostCells->SetNumberOfTuples(nelems);
    for (vtkIdType e = 0; e < nelems; ++e)
    {
      ghostCells->SetValue(
        e, part.OwnerRanks[dim][e] != part.CommRank ? vtkDataSetAttributes::DUPLICATECELL : 0);
    }
    grid->GetCellData()->AddArray(ghostCells);
  }

  return grid;
}

bool ReadIntFile(const std::string& path, int& value)
{
  vtkNew<vtkFileResourceStream> file;
  if (!file->Open(path.c_str()))
  {
    return false;
  }
  // The nparts/version sidecar files are tiny ASCII files holding one
  // integer each; vtkResourceStream is unformatted, so parse with sscanf.
  std::string text(64, '\0');
  const auto nbytes = file->Read(&text[0], text.size());
  if (nbytes == 0)
  {
    return false;
  }
  text.resize(nbytes);
  return std::sscanf(text.c_str(), "%d", &value) == 1;
}

} // namespace

//-----------------------------------------------------------------------------

vtkOmegaHReader::vtkOmegaHReader()
{
  // this is a reader, not a filter: it produces data without any input
  // (like vtkFidesReader).
  this->SetNumberOfInputPorts(0);
}

vtkOmegaHReader::~vtkOmegaHReader()
{
  this->SetFileName(nullptr);
  this->SetController(nullptr);
}

vtkMultiProcessController* vtkOmegaHReader::GetController()
{
  return this->Controller;
}

void vtkOmegaHReader::SetController(vtkMultiProcessController* controller)
{
  if (this->Controller != controller)
  {
    this->Controller = controller;
    this->Modified();
  }
}

void vtkOmegaHReader::SetStream(vtkResourceStream* stream)
{
  if (this->Stream != stream)
  {
    this->Stream = stream;
    this->Modified();
  }
}

vtkResourceStream* vtkOmegaHReader::GetStream()
{
  return this->Stream;
}

vtkTypeBool vtkOmegaHReader::CanReadFile(const char* name)
{
  // An omega_h mesh is a directory with ASCII "nparts" and "version"
  // sidecar files and at least one binary part file "<rank>.osh"; part 0
  // is checked for the omega_h magic bytes.
  if (!name || !vtksys::SystemTools::FileIsDirectory(name))
  {
    return 0;
  }
  const std::string directory(name);
  int nparts = 0;
  int version = 0;
  if (!ReadIntFile(directory + "/nparts", nparts) || nparts <= 0)
  {
    return 0;
  }
  if (!ReadIntFile(directory + "/version", version))
  {
    return 0;
  }
  vtkNew<vtkFileResourceStream> file;
  if (!file->Open((directory + "/0.osh").c_str()))
  {
    return 0;
  }
  unsigned char magic[2] = { 0, 0 };
  return (file->Read(magic, sizeof(magic)) == sizeof(magic) && magic[0] == kMagic[0] &&
           magic[1] == kMagic[1])
    ? 1
    : 0;
}

void vtkOmegaHReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

int vtkOmegaHReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  auto output = vtkPartitionedDataSet::GetData(outputVector, 0);
  if (!this->FileName && !this->Stream)
  {
    vtkErrorMacro(<< "Either a FileName or a Stream must be specified.");
    return 0;
  }

  if (this->Stream)
  {
    // Read a single part from the stream on every rank. The stream carries
    // no version sidecar, so parse with the latest format version.
    if (!this->Stream->SupportSeek())
    {
      vtkErrorMacro(<< "The stream must support seeking.");
      return 0;
    }
    this->Stream->Seek(0, vtkResourceStream::SeekDirection::Begin);
    Part part;
    std::string error;
    if (!ReadPart(this->Stream, kLatestVersion, "stream", part, error))
    {
      vtkErrorMacro(<< error);
      return 0;
    }
    auto grid = BuildGrid(part, error);
    if (!grid)
    {
      vtkErrorMacro(<< "Failed to convert the mesh part: " << error);
      return 0;
    }
    output->SetNumberOfPartitions(1);
    output->SetPartition(0, grid);
    return 1;
  }

  vtkMultiProcessController* controller = this->Controller;
  vtkNew<vtkDummyController> dummy;
  if (!controller)
  {
    controller = vtkMultiProcessController::GetGlobalController();
  }
  if (!controller)
  {
    controller = dummy;
  }
  const int rank = controller->GetLocalProcessId();
  const int nprocs = controller->GetNumberOfProcesses();

  const std::string directory(this->FileName);
  int nparts = 0;
  int version = -1;
  if (rank == 0)
  {
    if (!ReadIntFile(directory + "/nparts", nparts))
    {
      vtkErrorMacro(<< "Could not read \"" << directory << "/nparts\"; is this an omega_h mesh?");
      nparts = -1;
    }
    if (nparts >= 0 && !ReadIntFile(directory + "/version", version))
    {
      // omega_h treats a missing version file as version -1 (too old for us)
      version = -1;
    }
  }
  controller->Broadcast(&nparts, 1, 0);
  controller->Broadcast(&version, 1, 0);
  if (nparts <= 0)
  {
    vtkErrorMacro(<< "Invalid omega_h mesh directory: " << directory);
    return 0;
  }
  if (version < 9 || version > kLatestVersion)
  {
    vtkErrorMacro(<< "Unsupported omega_h format version " << version
                  << " (supported: 9, 10, 11).");
    return 0;
  }

  // Round robin: rank r reads parts r, r+P, r+2P, ... The partitioned data
  // set holds the full (global) number of partitions; every rank fills in
  // only the partitions it read (null elsewhere), like
  // vtkXMLPartitionedDataSetReader.
  output->SetNumberOfPartitions(nparts);
  for (int p = rank; p < nparts; p += nprocs)
  {
    const std::string filePath = directory + "/" + std::to_string(p) + ".osh";
    vtkNew<vtkFileResourceStream> file;
    if (!file->Open(filePath.c_str()))
    {
      vtkErrorMacro(<< "Could not open part file \"" << filePath << "\".");
      return 0;
    }
    Part part;
    std::string error;
    if (!ReadPart(file, version, filePath, part, error))
    {
      vtkErrorMacro(<< error);
      return 0;
    }
    auto grid = BuildGrid(part, error);
    if (!grid)
    {
      vtkErrorMacro(<< "Failed to convert part " << p << " of \"" << directory << "\": " << error);
      return 0;
    }
    output->SetPartition(p, grid);
  }
  return 1;
}
