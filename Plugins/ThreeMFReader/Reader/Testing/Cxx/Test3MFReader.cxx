// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// Writes small 3MF packages with an in-test ZIP writer, then checks the
// geometry, arrays and metadata produced by vtk3MFReader.

#include "vtk3MFReader.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"

#include <vtk_zlib.h>
#include <vtksys/FStream.hxx>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{
//------------------------------------------------------------------------------
// Minimal ZIP writer, enough to build 3MF packages.
struct ZipEntry
{
  std::string Name;
  std::string Data;
  bool Compress;
};

void Put16(std::string& s, uint32_t v)
{
  s += static_cast<char>(v & 0xFF);
  s += static_cast<char>((v >> 8) & 0xFF);
}

void Put32(std::string& s, uint32_t v)
{
  Put16(s, v & 0xFFFF);
  Put16(s, v >> 16);
}

std::string RawDeflate(const std::string& in)
{
  z_stream stream{};
  deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
  std::string out(deflateBound(&stream, static_cast<uLong>(in.size())), '\0');
  stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(in.data()));
  stream.avail_in = static_cast<uInt>(in.size());
  stream.next_out = reinterpret_cast<Bytef*>(&out[0]);
  stream.avail_out = static_cast<uInt>(out.size());
  deflate(&stream, Z_FINISH);
  out.resize(stream.total_out);
  deflateEnd(&stream);
  return out;
}

bool WriteZip(const std::string& fileName, const std::vector<ZipEntry>& entries)
{
  std::string archive, central;
  for (const ZipEntry& entry : entries)
  {
    const std::string payload = entry.Compress ? RawDeflate(entry.Data) : entry.Data;
    const uint32_t crc = static_cast<uint32_t>(crc32(crc32(0L, Z_NULL, 0),
      reinterpret_cast<const Bytef*>(entry.Data.data()), static_cast<uInt>(entry.Data.size())));
    const uint32_t offset = static_cast<uint32_t>(archive.size());
    const uint32_t method = entry.Compress ? 8 : 0;

    Put32(archive, 0x04034b50);
    Put16(archive, 20); // version needed
    Put16(archive, 0);  // flags
    Put16(archive, method);
    Put32(archive, 0); // time, date
    Put32(archive, crc);
    Put32(archive, static_cast<uint32_t>(payload.size()));
    Put32(archive, static_cast<uint32_t>(entry.Data.size()));
    Put16(archive, static_cast<uint32_t>(entry.Name.size()));
    Put16(archive, 0); // extra length
    archive += entry.Name;
    archive += payload;

    Put32(central, 0x02014b50);
    Put16(central, 20); // version made by
    Put16(central, 20); // version needed
    Put16(central, 0);  // flags
    Put16(central, method);
    Put32(central, 0); // time, date
    Put32(central, crc);
    Put32(central, static_cast<uint32_t>(payload.size()));
    Put32(central, static_cast<uint32_t>(entry.Data.size()));
    Put16(central, static_cast<uint32_t>(entry.Name.size()));
    Put16(central, 0); // extra length
    Put16(central, 0); // comment length
    Put16(central, 0); // disk number
    Put16(central, 0); // internal attributes
    Put32(central, 0); // external attributes
    Put32(central, offset);
    central += entry.Name;
  }

  const uint32_t centralOffset = static_cast<uint32_t>(archive.size());
  archive += central;
  Put32(archive, 0x06054b50);
  Put16(archive, 0);
  Put16(archive, 0);
  Put16(archive, static_cast<uint32_t>(entries.size()));
  Put16(archive, static_cast<uint32_t>(entries.size()));
  Put32(archive, static_cast<uint32_t>(central.size()));
  Put32(archive, centralOffset);
  Put16(archive, 0); // comment length

  vtksys::ofstream file(fileName.c_str(), std::ios::out | std::ios::binary);
  file.write(archive.data(), static_cast<std::streamsize>(archive.size()));
  return static_cast<bool>(file);
}

//------------------------------------------------------------------------------
const char* const ContentTypes = R"(<?xml version="1.0" encoding="UTF-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/>
</Types>)";

std::string Relationships(const std::string& target)
{
  return R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Target=")" +
    target + R"(" Id="rel0" Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/>
</Relationships>)";
}

// Unit cube with a single base material color.
const char* const CubeModel = R"(<?xml version="1.0" encoding="UTF-8"?>
<model unit="millimeter" xml:lang="en-US"
       xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02">
  <metadata name="Title">Test cube</metadata>
  <resources>
    <basematerials id="1">
      <base name="Red" displaycolor="#FF0000"/>
    </basematerials>
    <object id="2" name="Cube" type="model" pid="1" pindex="0">
      <mesh>
        <vertices>
          <vertex x="0" y="0" z="0"/> <vertex x="1" y="0" z="0"/>
          <vertex x="1" y="1" z="0"/> <vertex x="0" y="1" z="0"/>
          <vertex x="0" y="0" z="1"/> <vertex x="1" y="0" z="1"/>
          <vertex x="1" y="1" z="1"/> <vertex x="0" y="1" z="1"/>
        </vertices>
        <triangles>
          <triangle v1="0" v2="2" v3="1"/> <triangle v1="0" v2="3" v3="2"/>
          <triangle v1="4" v2="5" v3="6"/> <triangle v1="4" v2="6" v3="7"/>
          <triangle v1="0" v2="1" v3="5"/> <triangle v1="0" v2="5" v3="4"/>
          <triangle v1="1" v2="2" v3="6"/> <triangle v1="1" v2="6" v3="5"/>
          <triangle v1="2" v2="3" v3="7"/> <triangle v1="2" v2="7" v3="6"/>
          <triangle v1="3" v2="0" v3="4"/> <triangle v1="3" v2="4" v3="7"/>
        </triangles>
      </mesh>
    </object>
  </resources>
  <build>
    <item objectid="2"/>
  </build>
</model>)";

// One triangle instanced twice through components (one instance mirrored),
// with per-triangle colors from a color group, in centimeters.
const char* const ComponentsModel = R"(<?xml version="1.0" encoding="UTF-8"?>
<model unit="centimeter" xml:lang="en-US"
       xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02"
       xmlns:m="http://schemas.microsoft.com/3dmanufacturing/material/2015/02">
  <resources>
    <m:colorgroup id="1">
      <m:color color="#00FF0080"/>
      <m:color color="#0000FF"/>
    </m:colorgroup>
    <object id="2" type="model">
      <mesh>
        <vertices>
          <vertex x="0" y="0" z="0"/> <vertex x="1" y="0" z="0"/> <vertex x="0" y="1" z="0"/>
        </vertices>
        <triangles>
          <triangle v1="0" v2="1" v3="2" pid="1" p1="1"/>
        </triangles>
      </mesh>
    </object>
    <object id="3" name="Assembly" type="model">
      <components>
        <component objectid="2" transform="1 0 0 0 1 0 0 0 1 10 0 0"/>
        <component objectid="2" transform="-1 0 0 0 1 0 0 0 1 0 0 0"/>
      </components>
    </object>
  </resources>
  <build>
    <item objectid="3" transform="1 0 0 0 1 0 0 0 1 0 0 5"/>
    <item objectid="3" transform="1 0 0 0 1 0 0 0 1 0 0 -5"/>
  </build>
</model>)";

// Production extension: the root model references an object of another part.
const char* const ProductionRootModel = R"(<?xml version="1.0" encoding="UTF-8"?>
<model unit="millimeter" xml:lang="en-US" requiredextensions="p"
       xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02"
       xmlns:p="http://schemas.microsoft.com/3dmanufacturing/production/2015/06">
  <resources>
    <object id="1" name="Wrapper" type="model">
      <components>
        <component p:path="/3D/Objects/part.model" objectid="7"
                   transform="2 0 0 0 2 0 0 0 2 0 0 0"/>
      </components>
    </object>
  </resources>
  <build>
    <item objectid="1"/>
  </build>
</model>)";

const char* const ProductionPartModel = R"(<?xml version="1.0" encoding="UTF-8"?>
<model unit="millimeter" xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02">
  <resources>
    <object id="7" type="model">
      <mesh>
        <vertices>
          <vertex x="0" y="0" z="0"/> <vertex x="1" y="0" z="0"/> <vertex x="0" y="0" z="1"/>
        </vertices>
        <triangles><triangle v1="0" v2="1" v3="2"/></triangles>
      </mesh>
    </object>
  </resources>
  <build/>
</model>)";

const char* const BadIndexModel = R"(<?xml version="1.0" encoding="UTF-8"?>
<model xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02">
  <resources>
    <object id="1" type="model">
      <mesh>
        <vertices><vertex x="0" y="0" z="0"/></vertices>
        <triangles><triangle v1="0" v2="1" v3="2"/></triangles>
      </mesh>
    </object>
  </resources>
  <build><item objectid="1"/></build>
</model>)";

std::vector<ZipEntry> Package(const char* model, bool compress, bool withRels = true)
{
  std::vector<ZipEntry> entries = { { "[Content_Types].xml", ContentTypes, compress } };
  if (withRels)
  {
    entries.push_back({ "_rels/.rels", Relationships("/3D/3dmodel.model"), compress });
  }
  entries.push_back({ "3D/3dmodel.model", model, compress });
  return entries;
}

//------------------------------------------------------------------------------
#define CHECK(condition)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(condition))                                                                              \
    {                                                                                              \
      vtkLog(ERROR, "Check failed: " #condition);                                                  \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

bool Near(double a, double b)
{
  return std::abs(a - b) < 1e-9;
}

vtkPolyData* GetMesh(vtkPartitionedDataSetCollection* output, unsigned int index)
{
  return vtkPolyData::SafeDownCast(output->GetPartition(index, 0));
}

std::string GetFieldString(vtkPartitionedDataSetCollection* output, const char* name)
{
  auto array = vtkStringArray::SafeDownCast(output->GetFieldData()->GetAbstractArray(name));
  return array && array->GetNumberOfValues() > 0 ? array->GetValue(0) : std::string();
}

bool TestCube(const std::string& dir, bool compress)
{
  const std::string fileName = dir + (compress ? "/cube_deflate.3mf" : "/cube_stored.3mf");
  CHECK(WriteZip(fileName, Package(CubeModel, compress)));

  vtkNew<vtk3MFReader> reader;
  CHECK(reader->CanReadFile(fileName.c_str()) == 1);
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkPartitionedDataSetCollection* output = reader->GetOutput();

  CHECK(output->GetNumberOfPartitionedDataSets() == 1);
  CHECK(std::string(output->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME())) == "Cube");
  vtkPolyData* mesh = GetMesh(output, 0);
  CHECK(mesh);
  CHECK(mesh->GetNumberOfPoints() == 8);
  CHECK(mesh->GetNumberOfPolys() == 12);
  double bounds[6];
  mesh->GetBounds(bounds);
  CHECK(Near(bounds[0], 0) && Near(bounds[1], 1) && Near(bounds[4], 0) && Near(bounds[5], 1));

  auto colors = vtkUnsignedCharArray::SafeDownCast(mesh->GetCellData()->GetArray("Color"));
  CHECK(colors && colors->GetNumberOfComponents() == 4);
  CHECK(colors->GetValue(0) == 255 && colors->GetValue(1) == 0 && colors->GetValue(2) == 0 &&
    colors->GetValue(3) == 255);
  vtkDataArray* ids = mesh->GetCellData()->GetArray("ObjectId");
  CHECK(ids && ids->GetTuple1(0) == 2);

  CHECK(GetFieldString(output, "Unit") == "millimeter");
  CHECK(GetFieldString(output, "Title") == "Test cube");
  return true;
}

bool TestComponents(const std::string& dir)
{
  const std::string fileName = dir + "/components.3mf";
  // No relationships part: the reader must fall back to the default model path.
  CHECK(WriteZip(fileName, Package(ComponentsModel, true, false)));

  vtkNew<vtk3MFReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkPartitionedDataSetCollection* output = reader->GetOutput();

  CHECK(output->GetNumberOfPartitionedDataSets() == 2);
  CHECK(std::string(output->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME())) == "Assembly");
  CHECK(std::string(output->GetMetaData(1u)->Get(vtkCompositeDataSet::NAME())) == "Assembly (2)");
  CHECK(GetFieldString(output, "Unit") == "centimeter");

  vtkPolyData* mesh = GetMesh(output, 0);
  CHECK(mesh && mesh->GetNumberOfPoints() == 6 && mesh->GetNumberOfPolys() == 2);
  double bounds[6];
  mesh->GetBounds(bounds);
  CHECK(Near(bounds[0], -1) && Near(bounds[1], 11) && Near(bounds[4], 5) && Near(bounds[5], 5));

  // The mirrored instance must keep the same (+z) orientation.
  for (vtkIdType cell = 0; cell < 2; ++cell)
  {
    const vtkIdType* pts;
    vtkIdType npts;
    mesh->GetCellPoints(cell, npts, pts);
    double a[3], b[3], c[3];
    mesh->GetPoint(pts[0], a);
    mesh->GetPoint(pts[1], b);
    mesh->GetPoint(pts[2], c);
    const double nz = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
    CHECK(nz > 0);
  }

  auto colors = vtkUnsignedCharArray::SafeDownCast(mesh->GetCellData()->GetArray("Color"));
  CHECK(colors);
  CHECK(colors->GetValue(0) == 0 && colors->GetValue(1) == 0 && colors->GetValue(2) == 255);

  reader->ConvertToMillimetersOn();
  reader->Update();
  output = reader->GetOutput();
  GetMesh(output, 1)->GetBounds(bounds);
  CHECK(Near(bounds[0], -10) && Near(bounds[1], 110) && Near(bounds[4], -50));
  CHECK(GetFieldString(output, "Unit") == "millimeter");
  return true;
}

bool TestProduction(const std::string& dir)
{
  const std::string fileName = dir + "/production.3mf";
  std::vector<ZipEntry> entries = Package(ProductionRootModel, true);
  entries.push_back({ "3D/Objects/part.model", ProductionPartModel, true });
  CHECK(WriteZip(fileName, entries));

  vtkNew<vtk3MFReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkPartitionedDataSetCollection* output = reader->GetOutput();

  CHECK(output->GetNumberOfPartitionedDataSets() == 1);
  vtkPolyData* mesh = GetMesh(output, 0);
  CHECK(mesh && mesh->GetNumberOfPoints() == 3 && mesh->GetNumberOfPolys() == 1);
  double bounds[6];
  mesh->GetBounds(bounds);
  CHECK(Near(bounds[1], 2) && Near(bounds[5], 2));
  CHECK(mesh->GetCellData()->GetArray("ObjectId")->GetTuple1(0) == 7);
  // No colors in this file.
  CHECK(mesh->GetCellData()->GetArray("Color") == nullptr);
  return true;
}

bool TestInvalid(const std::string& dir)
{
  const std::string notZip = dir + "/not_a_zip.3mf";
  {
    vtksys::ofstream file(notZip.c_str(), std::ios::out | std::ios::binary);
    file << "solid this is not a 3MF file\n";
  }
  vtkNew<vtk3MFReader> reader;
  CHECK(reader->CanReadFile(notZip.c_str()) == 0);

  const std::string badIndex = dir + "/bad_index.3mf";
  CHECK(WriteZip(badIndex, Package(BadIndexModel, false)));
  CHECK(reader->CanReadFile(badIndex.c_str()) == 1);

  // Errors are expected here: silence them.
  vtkObject::GlobalWarningDisplayOff();
  reader->SetFileName(badIndex.c_str());
  reader->Update();
  vtkObject::GlobalWarningDisplayOn();
  CHECK(reader->GetOutput()->GetNumberOfPartitionedDataSets() == 0);
  return true;
}
}

//------------------------------------------------------------------------------
int Test3MFReader(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  const std::string dir = tempDir;
  delete[] tempDir;

  bool success = true;
  success &= TestCube(dir, false);
  success &= TestCube(dir, true);
  success &= TestComponents(dir);
  success &= TestProduction(dir);
  success &= TestInvalid(dir);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
