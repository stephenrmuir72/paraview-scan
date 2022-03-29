/*=========================================================================

  Program:   ParaView
  Module:    vtkSMReaderFactory.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMReaderFactory
 * @brief   is a factory for creating a reader
 * proxy based on the filename/extension.
 *
 * vtkSMReaderFactory is a factory for creating a reader that reads a particular
 * file. The reader factory needs to be configured to register the reader
 * prototypes supported by the application. This is done automatically when
 * the reader's proxy definition is registered AND if it has the extensions
 * specified in the Hints section of the XML proxy definition. It is done
 * with the following format:
 * \verbatim
 * <ReaderFactory extensions="[list of expected extensions]"
 *     file_description="[description of the file]" />
 * \endverbatim
 *
 * Once the factory has been configured, the API to create readers, get
 * available readers etc. can be used.
 */

#ifndef vtkSMReaderFactory_h
#define vtkSMReaderFactory_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
// ideally, we include rx-lite.hpp here.
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

#include <string>
#include <vector>

struct FileTypeDetailed
{
  std::string Description;
  std::vector<std::string> FilenamePatterns;
  std::string Group;
  std::string Name;
};

class vtkClientSession;
class vtkSMSessionProxyManager;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMReaderFactory : public vtkObject
{
public:
  static vtkSMReaderFactory* New();
  vtkTypeMacro(vtkSMReaderFactory, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the session to which this reader factory belongs.
   */
  void SetSession(vtkClientSession* session);
  vtkClientSession* GetSession() const;
  ///@}

  /**
   * Returns the session proxy manager. Same as calling
   * `vtkSMReaderFactory::GetSession()->GetProxyManager()`.
   */
  vtkSMSessionProxyManager* GetSessionProxyManager() const;

  /**
   * Resets the factory to its initial state. All registered reader prototypes
   * information will be cleared.
   */
  void Reset();

  ///@{
  /**
   * For the reader factory to work, one must register reader proxy prototypes
   * with the factory. These methods provides different ways of registering
   * prototypes.
   */
  void RegisterPrototype(const std::string& xmlgroup, const std::string& xmlname);
  void RegisterPrototypesFromGroup(const std::string& xmlgroup);
  ///@}

  /**
   * Returns the list of supported file types with details on the corresponding
   * readers and their supported patterns. The two first values of this vector are
   * "Supported Types" and "All Files" to simplify its use with the `pqFileDialog`.
   * They do not have a name or group fields.
   */
  std::vector<FileTypeDetailed> GetSupportedFileTypes() const;

  ///@{
  /**
   * Finds a reader type for reading the given file (or directory).
   */
  std::vector<std::pair<std::string, std::string>> FindReadersForFile(
    const std::string& filename) const;
  std::vector<std::pair<std::string, std::string>> FindReadersForDirectory(
    const std::string& filename) const;
  ///@}

  /**
   * Description of the file type used for all supported types
   */
  static const std::string SUPPORTED_TYPES_DESCRIPTION;

  /**
   * Description of the file type used for all files
   */
  static const std::string ALL_FILES_DESCRIPTION;

protected:
  vtkSMReaderFactory();
  ~vtkSMReaderFactory() override;

private:
  vtkSMReaderFactory(const vtkSMReaderFactory&) = delete;
  void operator=(const vtkSMReaderFactory&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
