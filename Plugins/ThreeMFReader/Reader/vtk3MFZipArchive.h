// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtk3MFZipArchive
 * @brief   Minimal read-only ZIP archive access for 3MF packages.
 *
 * 3MF files are Open Packaging Convention (OPC) packages, which are ZIP
 * archives. This class implements just enough of the ZIP format to locate and
 * extract parts of such a package: the central directory is parsed (including
 * ZIP64 extensions) and entries can be extracted when they are either stored
 * or compressed with deflate, using VTK's zlib. Encrypted and multi-disk
 * archives are not supported.
 *
 * Part names are looked up case-insensitively, with any leading '/' ignored and
 * percent-encoded characters decoded, as OPC part names are case-insensitive
 * URIs relative to the package root.
 *
 * This is a private helper of vtk3MFReader and is not part of its public API.
 */

#ifndef vtk3MFZipArchive_h
#define vtk3MFZipArchive_h

#include <cstdint>
#include <map>
#include <string>
#include <vector>

class vtk3MFZipArchive
{
public:
  /**
   * Open the archive and read its central directory.
   * On failure, returns false and fills `error`.
   */
  bool Open(const std::string& fileName, std::string& error);

  /**
   * Return true if the archive contains an entry with the given part name.
   */
  bool HasEntry(const std::string& name) const;

  /**
   * Extract the entry with the given part name into `data`.
   * On failure, returns false and fills `error`.
   */
  bool ReadEntry(const std::string& name, std::string& data, std::string& error) const;

  /**
   * Return the names of all entries, as stored in the archive.
   */
  std::vector<std::string> GetEntryNames() const;

  /**
   * Normalize an OPC part name for lookup: drop leading '/', convert '\\' to
   * '/', decode percent-encoded characters and convert to lower case.
   */
  static std::string NormalizeName(const std::string& name);

private:
  struct Entry
  {
    std::string Name;
    uint16_t Flags = 0;
    uint16_t Method = 0;
    uint32_t CRC = 0;
    uint64_t CompressedSize = 0;
    uint64_t UncompressedSize = 0;
    uint64_t LocalHeaderOffset = 0;
  };

  bool ReadBytes(uint64_t offset, uint64_t size, std::string& out) const;

  std::string FileName;
  uint64_t FileSize = 0;
  std::map<std::string, Entry> Entries;
};

#endif
// VTK-HeaderTest-Exclude: vtk3MFZipArchive.h
