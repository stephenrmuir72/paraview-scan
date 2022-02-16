/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFileInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVFileInformation
 * @brief   Information object that can
 * be used to obtain information about a file/directory.
 *
 * vtkPVFileInformation can be used to collect information about file
 * or directory. vtkPVFileInformation can collect information
 * from a vtkPVFileInformationHelper object alone.
 * @sa
 * vtkPVFileInformationHelper
 */

#ifndef vtkPVFileInformation_h
#define vtkPVFileInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSetGet.h"

#include <string> // Needed for std::string

class vtkCollection;
class vtkPVFileInformationSet;
class vtkFileSequenceParser;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVFileInformation : public vtkPVInformation
{
public:
  static vtkPVFileInformation* New();
  vtkTypeMacro(vtkPVFileInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   * The object must be a vtkPVFileInformationHelper.
   */
  void CopyFromObject(vtkObject* object);

  //@{
  /**
   * vtkPVInformation API implementation.
   */

  bool LoadInformation(const vtkJson& state) override;
  vtkJson SaveInformation() const override;
  void AddInformation(vtkPVInformation* oinfo) override;
  //@}

  ///@{
  /**
   * Save/load state of the information object. This is not the meta-data
   * gather, but the options that dictate how the meta-data is gathered,
   * if any.
   */
  vtkJson SaveState() const override;
  bool LoadState(const vtkJson& state) override;

  /**
   * Gather information from the target.
   */
  bool GatherInformation(vtkObject* target) override;

  /**
   * We want all vtkPVFileInformation objects to be gathered on the RPC thread.
   */
  bool CanRunOnRPC() const override { return true; }

  enum FileTypes
  {
    INVALID = 0,
    SINGLE_FILE,
    SINGLE_FILE_LINK,
    DIRECTORY,
    DIRECTORY_LINK,
    FILE_GROUP,
    DRIVE,
    NETWORK_ROOT,
    NETWORK_DOMAIN,
    NETWORK_SERVER,
    NETWORK_SHARE,
    DIRECTORY_GROUP
  };

  //@{
  /**
   * Helper that returns whether a FileType is a
   * directory (DIRECTORY, DRIVE, NETWORK_ROOT, etc...)
   * Or in other words, a type that we can do a DirectoryListing on.
   */
  static bool IsDirectory(int t);
  bool IsDirectory() const { return vtkPVFileInformation::IsDirectory(this->Type); }
  //@}

  //@{
  /**
   * Helper that returns true if file-type is a group i.e.
   * either a FILE_GROUP or a DIRECTORY_GROUP.
   */
  static bool IsGroup(int type)
  {
    switch (type)
    {
      case FILE_GROUP:
      case DIRECTORY_GROUP:
        return true;
      default:
        return false;
    }
  }
  bool IsGroup() const { return vtkPVFileInformation::IsGroup(this->Type); }
  //@}

  /**
   * Initializes the information object.
   */
  void Initialize();

  //@{
  /**
   * Get the name of the file/directory whose information is
   * represented by this object.
   */
  vtkGetStringMacro(Name);
  //@}

  //@{
  /**
   * Get the full path of the file/directory whose information is
   * represented by this object.
   */
  vtkGetStringMacro(FullPath);
  //@}

  //@{
  /**
   * Get the type of this file object.
   */
  vtkGetMacro(Type, int);
  //@}

  //@{
  /**
   * Get the state of the hidden flag for the file/directory.
   */
  vtkGetMacro(Hidden, bool);
  //@}

  ///@{
  /**
   * Groups file sequences when found. A file sequence could be [foo1.png, foo2.png, foo6.png].
   * When turned on, the series of files is grouped and our sequence example is named foo..png
   * and original files are discarded from the listing.
   * By default, this flag is set to true.
   */
  vtkGetMacro(GroupFileSequences, bool);
  vtkSetMacro(GroupFileSequences, bool);
  vtkBooleanMacro(GroupFileSequences, bool);
  ///@}

  vtkSetMacro(FastFileTypeDetection, bool);
  vtkSetMacro(ReadDetailedFileInformation, bool);
  vtkGetMacro(ReadDetailedFileInformation, bool);
  vtkSetMacro(ListDirectories, bool);
  vtkSetMacro(ListSpecialDirectories, bool);
  void SetPath(const std::string& path)
  {
    this->Path = path;
    this->Modified();
  };
  void SetWorkingDirectory(const std::string& path)
  {
    this->WorkingDirectory = path;
    this->Modified();
  };

  //@{
  /**
   * Get the Contents for this directory.
   * Returns a collection with vtkPVFileInformation objects
   * for the contents of this directory if Type == DIRECTORY
   * or the contents of this file group if Type == FILE_GROUP
   * or the contents of this directory group if Type == DIRECTORY_GROUP.
   */
  vtkGetObjectMacro(Contents, vtkCollection);
  vtkGetStringMacro(Extension);
  vtkGetMacro(Size, long long);
  vtkGetMacro(ModificationTime, time_t);
  //@}

  std::string RetrieveAbsolutePath(const std::string& path);
  bool FileExists(const std::string& path, std::string& fullpath);
  bool DirectoryExists(const std::string& path, std::string& fullpath);

  vtkGetMacro(PathSeparator, char);

protected:
  vtkPVFileInformation();
  ~vtkPVFileInformation() override;

  vtkCollection* Contents;
  vtkFileSequenceParser* SequenceParser;

  char* Name;              // Name of this file/directory.
  char* FullPath;          // Full path for this file/directory.
  int Type;                // Type i.e. File/Directory/FileGroup.
  bool Hidden;             // If file/directory is hidden
  char* Extension;         // File extension
  long long Size;          // File size
  time_t ModificationTime; // File modification time
  char PathSeparator;      // Character used as path separator for the server.

  vtkSetStringMacro(Extension);
  vtkSetStringMacro(Name);
  vtkSetStringMacro(FullPath);

  void GetWindowsDirectoryListing();
  void GetDirectoryListing();

  // Goes thru the collection of vtkPVFileInformation objects
  // are creates file groups, if possible.
  void OrganizeCollection(vtkPVFileInformationSet& vector);

  bool DetectType();
  void GetSpecialDirectories();
  void SetHiddenFlag();

  // These flags describe how to get the data.
  //@{
  bool FastFileTypeDetection;
  bool ReadDetailedFileInformation;
  bool GroupFileSequences;
  bool ListSpecialDirectories;
  bool ListDirectories;
  std::string Path;
  std::string WorkingDirectory;
  //@}

private:
  vtkPVFileInformation(const vtkPVFileInformation&) = delete;
  void operator=(const vtkPVFileInformation&) = delete;

  struct vtkInfo;
};

#endif
