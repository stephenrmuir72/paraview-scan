/*=========================================================================

  Program:   ParaView
  Module:    vtkCLIOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCLIOptions.h"

#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <vtk_cli11.h>
#include <vtksys/SystemTools.hxx>

namespace
{
// This is fairly naive. It's not smart enough to detect 'quoted words'
// and treat them as a single word. However, works reasonably okay for our
// use-case.
std::string FormatParagraph(size_t indent, const std::string& text)
{
  static const size_t width = vtksys::SystemTools::GetTerminalWidth();
  if (width < (indent + 10))
  {
    // too narrow; don't bother.
    return text;
  }

  std::ostringstream str;
  const size_t target = width - indent;
  size_t count = 0;
  for (const auto& word : vtksys::SystemTools::SplitString(text, ' '))
  {
    if ((count + word.length() + 5) > target)
    {
      str << "\n";
      count = 0;
    }
    str << word << " ";
    count += word.length() + 1;
  }
  return str.str();
}

class PVFormatter : public CLI::Formatter
{
  using Superclass = CLI::Formatter;

public:
  // overridden to add a new line before each group.
  std::string make_expanded(const CLI::App* sub) const override
  {
    return "\n" + Superclass::make_expanded(sub);
  }

  // overridden to ensure good word wrapping for description text.
  std::string make_option_desc(const CLI::Option* option) const override
  {
    auto txt = this->Superclass::make_option_desc(option);
    const size_t indent = this->get_column_width();
    return FormatParagraph(indent, txt);
  }
};

}

class vtkCLIOptions::vtkInternals
{
public:
  std::shared_ptr<CLI::App> App;
  vtkInternals()
    : App(new CLI::App())
  {
    this->App->formatter(std::make_shared<PVFormatter>());
  }
};

vtkStandardNewMacro(vtkCLIOptions);
//----------------------------------------------------------------------------
vtkCLIOptions::vtkCLIOptions()
  : Internals(new vtkCLIOptions::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkCLIOptions::~vtkCLIOptions() {}

//----------------------------------------------------------------------------
vtkCLIOptions* vtkCLIOptions::New(const std::shared_ptr<CLI::App>& app)
{
  auto result = vtkCLIOptions::New();
  if (app)
  {
    result->Internals->App = app;
  }
  return result;
}

//----------------------------------------------------------------------------
void vtkCLIOptions::SetAllowExtras(bool val)
{
  if (auto app = this->GetCLI11App())
  {
    app->allow_extras(val);
  }
}

//----------------------------------------------------------------------------
CLI::App* vtkCLIOptions::GetCLI11App() const
{
  auto& internals = (*this->Internals);
  return internals.App.get();
}

//----------------------------------------------------------------------------
std::string vtkCLIOptions::GetHelp() const
{
  auto& internals = (*this->Internals);
  return internals.App->help();
}

//----------------------------------------------------------------------------
void vtkCLIOptions::SetName(const char* name)
{
  auto& internals = (*this->Internals);
  internals.App->name(name ? name : "");
}

//----------------------------------------------------------------------------
void vtkCLIOptions::SetDescription(const char* desc)
{
  auto& internals = (*this->Internals);
  internals.App->description(desc ? desc : "");
}

//----------------------------------------------------------------------------
vtkCLIOptions::StatusFlag vtkCLIOptions::Parse(int& argc, char**& argv)
{
  this->ExtraArguments.clear();
  this->LastErrorMessage.clear();
  this->LastParseStatusFlag = vtkCLIOptions::Unknown;

  auto controller = vtkMultiProcessController::GetGlobalController();
  const bool generateWarnings =
    this->GenerateWarnings && controller && (controller->GetLocalProcessId() == 0);

  auto& internals = (*this->Internals);
  auto& app = (*internals.App);
  if (app.get_name().empty())
  {
    app.name(argv[0]);
  }

  bool skipArgs = false;
  bool legacyArgumentSeperator = false;
  std::vector<std::string> args;
  args.reserve(argc - 1);
  for (int cc = 1; cc < argc; ++cc)
  {
    auto arg = std::string(argv[cc]);
    if (skipArgs)
    {
      this->ExtraArguments.emplace_back(std::move(arg));
      continue;
    }
    else if (arg == "--")
    {
      skipArgs = true;
      continue;
    }
    else if (arg[0] == '-')
    {
      auto idx = arg.find('=');
      if (idx != std::string::npos)
      {
        args.emplace_back(arg.substr(0, idx));
        args.emplace_back(arg.substr(idx + 1));
        legacyArgumentSeperator = true;
        continue;
      }
    }
    args.emplace_back(std::move(arg));
  }

  // TODO: warn on root node only.
  // Previously, we supported args like '-dr'. These are now converted to
  // '--dr'. This piece of code handles that automatically.
  for (auto& arg : args)
  {
    if (arg.length() > 2 && arg[0] == '-' && arg[1] != '-')
    {
      if (generateWarnings)
      {
        vtkLogF(WARNING, "'%s' is deprecated. Use '-%s' instead.", arg.c_str(), arg.c_str());
      }
      arg.insert(arg.begin(), '-');
    }
  }

  if (legacyArgumentSeperator && generateWarnings)
  {
    // For now, let's not warn about this.
    // vtkLogF(WARNING,
    //   "'=' is no longer needed for separating command line argument names and their values.");
  }

  try
  {
    std::reverse(args.begin(), args.end());
    internals.App->parse(args);
    std::reverse(args.begin(), args.end());
    std::copy(args.begin(), args.end(), std::back_inserter(this->ExtraArguments));
    this->LastParseStatusFlag = vtkCLIOptions::Success;
  }
  catch (const CLI::CallForHelp&)
  {
    this->LastParseStatusFlag = vtkCLIOptions::HelpRequested;
  }
  catch (const CLI::CallForAllHelp&)
  {
    this->LastParseStatusFlag = vtkCLIOptions::HelpRequested;
  }
  catch (const CLI::ParseError& error)
  {
    this->LastErrorMessage = error.what();
    this->LastParseStatusFlag = vtkCLIOptions::Fail;
  }

  return this->LastParseStatusFlag;
}

//----------------------------------------------------------------------------
void vtkCLIOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GenerateWarnings: " << this->GenerateWarnings << endl;
}
