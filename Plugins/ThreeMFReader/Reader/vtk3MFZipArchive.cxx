// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtk3MFZipArchive.h"

#include <vtk_zlib.h>
#include <vtksys/FStream.hxx>

#include <algorithm>
#include <cctype>
#include <limits>
#include <new>

namespace
{
constexpr uint32_t LocalHeaderSignature = 0x04034b50;
constexpr uint32_t CentralHeaderSignature = 0x02014b50;
constexpr uint32_t EndOfCentralDirSignature = 0x06054b50;
constexpr uint32_t Zip64EndOfCentralDirSignature = 0x06064b50;
constexpr uint32_t Zip64LocatorSignature = 0x07064b50;

constexpr uint64_t EndOfCentralDirSize = 22;
constexpr uint64_t Zip64LocatorSize = 20;
constexpr uint64_t Zip64EndOfCentralDirSize = 56;
constexpr uint64_t CentralHeaderSize = 46;
constexpr uint64_t LocalHeaderSize = 30;
constexpr uint64_t MaxCommentSize = 0xFFFF;

constexpr uint16_t MethodStored = 0;
constexpr uint16_t MethodDeflate = 8;
constexpr uint16_t FlagEncrypted = 0x0001;

// Chunk size used when feeding zlib, whose lengths are 32-bit.
constexpr uint64_t ZlibChunk = 1u << 30;

uint16_t Read16(const unsigned char* p)
{
  return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

uint32_t Read32(const unsigned char* p)
{
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
    (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

uint64_t Read64(const unsigned char* p)
{
  return static_cast<uint64_t>(Read32(p)) | (static_cast<uint64_t>(Read32(p + 4)) << 32);
}

const unsigned char* Bytes(const std::string& s, std::size_t offset = 0)
{
  return reinterpret_cast<const unsigned char*>(s.data()) + offset;
}

int HexValue(char c)
{
  if (c >= '0' && c <= '9')
  {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f')
  {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F')
  {
    return c - 'A' + 10;
  }
  return -1;
}

uint32_t ComputeCRC(const std::string& data)
{
  uLong crc = crc32(0L, Z_NULL, 0);
  const Bytef* ptr = reinterpret_cast<const Bytef*>(data.data());
  uint64_t remaining = data.size();
  while (remaining > 0)
  {
    const uInt len = static_cast<uInt>(std::min(remaining, ZlibChunk));
    crc = crc32(crc, ptr, len);
    ptr += len;
    remaining -= len;
  }
  return static_cast<uint32_t>(crc);
}

bool Inflate(const std::string& in, std::string& out, std::string& error)
{
  z_stream stream{};
  // Negative window bits: raw deflate stream, as stored in ZIP archives.
  if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
  {
    error = "Cannot initialize zlib.";
    return false;
  }

  Bytef* inPtr = reinterpret_cast<Bytef*>(const_cast<char*>(in.data()));
  uint64_t inRemaining = in.size();
  Bytef* outPtr = reinterpret_cast<Bytef*>(&out[0]);
  uint64_t outRemaining = out.size();

  int status = Z_OK;
  while (status == Z_OK)
  {
    if (stream.avail_in == 0 && inRemaining > 0)
    {
      stream.next_in = inPtr;
      stream.avail_in = static_cast<uInt>(std::min(inRemaining, ZlibChunk));
      inPtr += stream.avail_in;
      inRemaining -= stream.avail_in;
    }
    if (stream.avail_out == 0 && outRemaining > 0)
    {
      stream.next_out = outPtr;
      stream.avail_out = static_cast<uInt>(std::min(outRemaining, ZlibChunk));
      outPtr += stream.avail_out;
      outRemaining -= stream.avail_out;
    }
    if (stream.avail_out == 0)
    {
      // Output is full but the stream has not ended: it is larger than declared.
      // Probe for one more byte to tell a truncated declaration from a complete stream.
      Bytef probe;
      stream.next_out = &probe;
      stream.avail_out = 1;
      status = inflate(&stream, Z_NO_FLUSH);
      if (status != Z_STREAM_END || stream.avail_out == 0)
      {
        inflateEnd(&stream);
        error = "Compressed entry is larger than its declared size.";
        return false;
      }
      break;
    }
    status = inflate(&stream, Z_NO_FLUSH);
    if (status == Z_BUF_ERROR && stream.avail_in == 0 && inRemaining == 0)
    {
      break; // ran out of input
    }
  }
  const bool complete = (status == Z_STREAM_END) && stream.total_out == out.size();
  inflateEnd(&stream);
  if (!complete)
  {
    error = "Compressed entry is corrupt or truncated.";
    return false;
  }
  return true;
}
}

//------------------------------------------------------------------------------
std::string vtk3MFZipArchive::NormalizeName(const std::string& name)
{
  std::string result;
  result.reserve(name.size());
  for (std::size_t i = 0; i < name.size(); ++i)
  {
    char c = name[i];
    if (c == '%' && i + 2 < name.size())
    {
      const int hi = HexValue(name[i + 1]);
      const int lo = HexValue(name[i + 2]);
      if (hi >= 0 && lo >= 0)
      {
        c = static_cast<char>((hi << 4) | lo);
        i += 2;
      }
    }
    else if (c == '\\')
    {
      c = '/';
    }
    result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  const std::size_t start = result.find_first_not_of('/');
  return start == std::string::npos ? std::string() : result.substr(start);
}

//------------------------------------------------------------------------------
bool vtk3MFZipArchive::ReadBytes(uint64_t offset, uint64_t size, std::string& out) const
{
  if (offset > this->FileSize || size > this->FileSize - offset)
  {
    return false;
  }
  vtksys::ifstream file(this->FileName.c_str(), std::ios::in | std::ios::binary);
  if (!file)
  {
    return false;
  }
  try
  {
    out.resize(static_cast<std::size_t>(size));
  }
  catch (const std::bad_alloc&)
  {
    return false;
  }
  file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  file.read(&out[0], static_cast<std::streamsize>(size));
  return static_cast<uint64_t>(file.gcount()) == size;
}

//------------------------------------------------------------------------------
bool vtk3MFZipArchive::Open(const std::string& fileName, std::string& error)
{
  this->FileName = fileName;
  this->Entries.clear();
  this->FileSize = 0;

  {
    vtksys::ifstream file(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!file)
    {
      error = "Cannot open file.";
      return false;
    }
    file.seekg(0, std::ios::end);
    const std::streamoff end = file.tellg();
    if (end < 0)
    {
      error = "Cannot determine file size.";
      return false;
    }
    this->FileSize = static_cast<uint64_t>(end);
  }

  if (this->FileSize < EndOfCentralDirSize)
  {
    error = "File is too small to be a ZIP archive.";
    return false;
  }

  // The end of central directory record sits at the end of the file, followed
  // by an optional comment of up to 64 KiB.
  const uint64_t tailSize = std::min(this->FileSize, EndOfCentralDirSize + MaxCommentSize);
  const uint64_t tailOffset = this->FileSize - tailSize;
  std::string tail;
  if (!this->ReadBytes(tailOffset, tailSize, tail))
  {
    error = "Cannot read file.";
    return false;
  }

  std::size_t eocd = std::string::npos;
  for (std::size_t pos = tail.size() - EndOfCentralDirSize + 1; pos-- > 0;)
  {
    if (Read32(Bytes(tail, pos)) == EndOfCentralDirSignature)
    {
      eocd = pos;
      break;
    }
  }
  if (eocd == std::string::npos)
  {
    error = "Not a ZIP archive (end of central directory not found).";
    return false;
  }

  const unsigned char* rec = Bytes(tail, eocd);
  const uint16_t diskNumber = Read16(rec + 4);
  const uint16_t cdDisk = Read16(rec + 6);
  uint64_t numEntries = Read16(rec + 10);
  uint64_t cdSize = Read32(rec + 12);
  uint64_t cdOffset = Read32(rec + 16);

  if (numEntries == 0xFFFF || cdSize == 0xFFFFFFFF || cdOffset == 0xFFFFFFFF)
  {
    // ZIP64: the locator immediately precedes the end of central directory record.
    const uint64_t eocdOffset = tailOffset + eocd;
    std::string locator;
    if (eocdOffset < Zip64LocatorSize ||
      !this->ReadBytes(eocdOffset - Zip64LocatorSize, Zip64LocatorSize, locator) ||
      Read32(Bytes(locator)) != Zip64LocatorSignature)
    {
      error = "ZIP64 end of central directory locator not found.";
      return false;
    }
    std::string record;
    if (!this->ReadBytes(Read64(Bytes(locator, 8)), Zip64EndOfCentralDirSize, record) ||
      Read32(Bytes(record)) != Zip64EndOfCentralDirSignature)
    {
      error = "ZIP64 end of central directory record not found.";
      return false;
    }
    numEntries = Read64(Bytes(record, 32));
    cdSize = Read64(Bytes(record, 40));
    cdOffset = Read64(Bytes(record, 48));
  }
  else if (diskNumber != 0 || cdDisk != 0)
  {
    error = "Multi-disk ZIP archives are not supported.";
    return false;
  }

  std::string cd;
  if (!this->ReadBytes(cdOffset, cdSize, cd))
  {
    error = "Cannot read the ZIP central directory.";
    return false;
  }

  std::size_t pos = 0;
  for (uint64_t i = 0; i < numEntries; ++i)
  {
    if (pos + CentralHeaderSize > cd.size() || Read32(Bytes(cd, pos)) != CentralHeaderSignature)
    {
      error = "Corrupt ZIP central directory.";
      return false;
    }
    const unsigned char* h = Bytes(cd, pos);
    Entry entry;
    entry.Flags = Read16(h + 8);
    entry.Method = Read16(h + 10);
    entry.CRC = Read32(h + 16);
    entry.CompressedSize = Read32(h + 20);
    entry.UncompressedSize = Read32(h + 24);
    const uint16_t nameLength = Read16(h + 28);
    const uint16_t extraLength = Read16(h + 30);
    const uint16_t commentLength = Read16(h + 32);
    entry.LocalHeaderOffset = Read32(h + 42);

    const std::size_t next = pos + CentralHeaderSize + nameLength + extraLength + commentLength;
    if (next > cd.size())
    {
      error = "Corrupt ZIP central directory.";
      return false;
    }
    entry.Name = cd.substr(pos + CentralHeaderSize, nameLength);

    // ZIP64 extended information: 64-bit values for the fields saturated above, in order.
    std::size_t extra = pos + CentralHeaderSize + nameLength;
    const std::size_t extraEnd = extra + extraLength;
    while (extra + 4 <= extraEnd)
    {
      const uint16_t id = Read16(Bytes(cd, extra));
      const uint16_t size = Read16(Bytes(cd, extra + 2));
      std::size_t field = extra + 4;
      const std::size_t fieldEnd = std::min(field + size, extraEnd);
      if (id == 0x0001)
      {
        for (uint64_t* value :
          { &entry.UncompressedSize, &entry.CompressedSize, &entry.LocalHeaderOffset })
        {
          if (*value == 0xFFFFFFFF && field + 8 <= fieldEnd)
          {
            *value = Read64(Bytes(cd, field));
            field += 8;
          }
        }
      }
      extra += 4 + size;
    }

    // Directories have no content; skip them.
    if (!entry.Name.empty() && entry.Name.back() != '/')
    {
      this->Entries[NormalizeName(entry.Name)] = entry;
    }
    pos = next;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtk3MFZipArchive::HasEntry(const std::string& name) const
{
  return this->Entries.count(NormalizeName(name)) > 0;
}

//------------------------------------------------------------------------------
std::vector<std::string> vtk3MFZipArchive::GetEntryNames() const
{
  std::vector<std::string> names;
  names.reserve(this->Entries.size());
  for (const auto& item : this->Entries)
  {
    names.push_back(item.second.Name);
  }
  return names;
}

//------------------------------------------------------------------------------
bool vtk3MFZipArchive::ReadEntry(
  const std::string& name, std::string& data, std::string& error) const
{
  data.clear();
  const auto it = this->Entries.find(NormalizeName(name));
  if (it == this->Entries.end())
  {
    error = "Entry '" + name + "' not found in archive.";
    return false;
  }
  const Entry& entry = it->second;
  if (entry.Flags & FlagEncrypted)
  {
    error = "Entry '" + name + "' is encrypted, which is not supported.";
    return false;
  }
  if (entry.Method != MethodStored && entry.Method != MethodDeflate)
  {
    error = "Entry '" + name + "' uses unsupported compression method " +
      std::to_string(entry.Method) + ".";
    return false;
  }
  if (entry.UncompressedSize > std::numeric_limits<std::size_t>::max())
  {
    error = "Entry '" + name + "' is too large.";
    return false;
  }

  // The local header has its own variable-length fields, which may differ
  // from the central directory ones.
  std::string header;
  if (!this->ReadBytes(entry.LocalHeaderOffset, LocalHeaderSize, header) ||
    Read32(Bytes(header)) != LocalHeaderSignature)
  {
    error = "Corrupt local header for entry '" + name + "'.";
    return false;
  }
  const uint64_t dataOffset = entry.LocalHeaderOffset + LocalHeaderSize +
    Read16(Bytes(header, 26)) + Read16(Bytes(header, 28));

  std::string compressed;
  if (!this->ReadBytes(dataOffset, entry.CompressedSize, compressed))
  {
    error = "Cannot read data for entry '" + name + "'.";
    return false;
  }

  if (entry.Method == MethodStored)
  {
    if (entry.CompressedSize != entry.UncompressedSize)
    {
      error = "Corrupt sizes for stored entry '" + name + "'.";
      return false;
    }
    data.swap(compressed);
  }
  else
  {
    try
    {
      data.resize(static_cast<std::size_t>(entry.UncompressedSize));
    }
    catch (const std::bad_alloc&)
    {
      error = "Not enough memory to extract entry '" + name + "'.";
      return false;
    }
    std::string inflateError;
    if (!Inflate(compressed, data, inflateError))
    {
      data.clear();
      error = "Entry '" + name + "': " + inflateError;
      return false;
    }
  }

  if (ComputeCRC(data) != entry.CRC)
  {
    data.clear();
    error = "CRC mismatch for entry '" + name + "'.";
    return false;
  }
  return true;
}
