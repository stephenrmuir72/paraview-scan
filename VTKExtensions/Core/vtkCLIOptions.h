/*=========================================================================

  Program:   ParaView
  Module:    vtkCLIOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkCLIOptions
 * @brief command line options manager
 *
 * vtkCLIOptions handles command line arguments processing in ParaView. To use,
 * create a `vtkCLIOptions` instance, populate it with available arguments and
 * then call `Parse`. Various modules in ParaView offer ways of populating
 * vtkCLIOptions instance with supported command line arguments. For example:
 *
 * @code
 *
 * vtkNew<vtkCLIOptions> options;
 * vtkRemotingCoreConfiguration::GetInstance()->PopulateOptions(options);
 * vtkRemotingServerManagerConfiguration::GetInstance()->PopulateOptions(options);
 *
 * @endcode
 *
 * Applications can add custom configuration classes that expose new command
 * line options and configuration parameters.
 *
 * ParaView uses CLI11 for the processing on command line arguments. The CLI11
 * App can be accessed using `GetCLI11App`. vtkCLIOptions automatically creates
 * a `CLI::App` on instantiation. One can also pass it an existing app using
 * `vtkCLIOptions::New(app)`.
 */

#ifndef vtkCLIOptions_h
#define vtkCLIOptions_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro

#include <memory>              // for std::unique_ptr
#include <string>              // for std::string
#include <vector>              // for std::vector
#include <vtk_cli11_forward.h> // for CLI::App

// forward declare CLI::App
namespace CLI
{
class App;
}

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkCLIOptions : public vtkObject
{
public:
  static vtkCLIOptions* New();
  vtkTypeMacro(vtkCLIOptions, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Use this overload of `New` to use an existing CLI::App instance. Note,
   * when using this API, since CLI::API
   */
  static vtkCLIOptions* New(const std::shared_ptr<CLI::App>& app);

  /**
   * Returns the `CLI::App` reference that can be used to populate command line
   * options.
   */
  CLI::App* GetCLI11App() const;

  /**
   * Returns the help string.
   */
  std::string GetHelp() const;

  void SetName(const char* name);
  void SetDescription(const char* desc);

  enum StatusFlag
  {
    Unknown = -1,
    Fail = 0,
    Success,
    HelpRequested,
  };

  vtkCLIOptions::StatusFlag Parse(int& argc, char**& argv);

  /**
   * Indicate whether extra arguments should be allowed/ignored. Unparsed
   * arguments can be obtained using `GetExtraArguments`.
   */
  void SetAllowExtras(bool val);

  ///@{
  /**
   * Returns unparsed / extra arguments left over from the most reset `Parse`
   * call.
   */
  const std::vector<std::string> GetExtraArguments() const { return this->ExtraArguments; }
  ///@}

  ///@{
  /**
   * When set to true (default), warnings messages are generated with
   * incorrectly named short arguments are automatically converted to long
   * arguments.
   */
  vtkSetMacro(GenerateWarnings, bool);
  vtkGetMacro(GenerateWarnings, bool);
  vtkBooleanMacro(GenerateWarnings, bool);
  ///@}

  /**
   * Returns error message from the most recent call to `Parse`, if any.
   */
  const std::string& GetLastErrorMessage() const { return this->LastErrorMessage; }

  /**
   * Returns the status from the most reset `Parse` call. Defaults to Unknown.
   */
  vtkGetMacro(LastParseStatusFlag, vtkCLIOptions::StatusFlag);

protected:
  vtkCLIOptions();
  ~vtkCLIOptions();

private:
  vtkCLIOptions(const vtkCLIOptions&) = delete;
  void operator=(const vtkCLIOptions&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  bool GenerateWarnings = true;
  std::vector<std::string> ExtraArguments;
  std::string LastErrorMessage;
  vtkCLIOptions::StatusFlag LastParseStatusFlag = vtkCLIOptions::Unknown;
};

#endif
