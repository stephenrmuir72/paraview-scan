/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplicationOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVApplicationOptions.h"

#include "vtkObjectFactory.h"

#include <vector>
#include <vtk_cli11.h>
#include <vtksys/SystemInformation.hxx>
#include <vtksys/SystemTools.hxx>

namespace
{
// Redefined from vtkRenderWindow.h
static const int VTK_STEREO_CRYSTAL_EYES = 1;
static const int VTK_STEREO_RED_BLUE = 2;
static const int VTK_STEREO_INTERLACED = 3;
static const int VTK_STEREO_LEFT = 4;
static const int VTK_STEREO_RIGHT = 5;
static const int VTK_STEREO_DRESDEN = 6;
static const int VTK_STEREO_ANAGLYPH = 7;
static const int VTK_STEREO_CHECKERBOARD = 8;
static const int VTK_STEREO_SPLITVIEWPORT_HORIZONTAL = 9;
static const int VTK_STEREO_FAKE = 10;
static const int VTK_STEREO_EMULATE = 11;

int ParseStereoType(const std::string& value)
{
  if (value == "Crystal Eyes")
  {
    return VTK_STEREO_CRYSTAL_EYES;
  }
  else if (value == "Red-Blue")
  {
    return VTK_STEREO_RED_BLUE;
  }
  else if (value == "Interlaced")
  {
    return VTK_STEREO_INTERLACED;
  }
  else if (value == "Dresden")
  {
    return VTK_STEREO_DRESDEN;
  }
  else if (value == "Anaglyph")
  {
    return VTK_STEREO_ANAGLYPH;
  }
  else if (value == "Checkerboard")
  {
    return VTK_STEREO_CHECKERBOARD;
  }
  else if (value == "SplitViewportHorizontal")
  {
    return VTK_STEREO_SPLITVIEWPORT_HORIZONTAL;
  }
  throw CLI::ValidationError("Invalid stereo-type specified.");
}

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
  for (const auto& line : vtksys::SystemTools::SplitString(text, '\n'))
  {
    size_t count = 0;
    for (const auto& word : vtksys::SystemTools::SplitString(line, ' '))
    {
      if ((count + word.length() + 5) > target)
      {
        str << "\n";
        count = 0;
      }
      str << word << " ";
      count += word.length() + 1;
    }
    str << "\n";
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

  std::string make_description(const CLI::App* app) const override
  {
    auto txt = this->Superclass::make_description(app);
    return FormatParagraph(0, txt);
  }

  std::string make_usage(const CLI::App* app, std::string name) const override
  {
    auto usage = this->Superclass::make_usage(app, name);
    if (app->get_parent() == nullptr)
    {
      usage +=
        "\n[General Guidelines]\n\n"
        "Values for options can be specified either with a space (' ') or an equal-to sign ('='). "
        "Thus, '--option=value' and '--option value' are equivalent.\n\n"
        "Multi-valued options can be specified by providing the option multiple times or "
        "separating the values by a comma (','). Thus, '--moption=foo,bar' and '--moption=foo "
        "--moption=bar' are equivalent.";
    }
    return FormatParagraph(0, usage);
  }
};
}

class vtkPVApplicationOptions::vtkInternals
{
public:
  bool ReadOnly{ false };
  vtkLogger::Verbosity LogStdErrVerbosity{ vtkLogger::VERBOSITY_INVALID };
  bool EnableStackTrace{ false };
  bool DisableRegistry{ false };
  bool DisableXDisplayTests{ false };
  bool ForceOffscreenRendering{ false };
  bool ForceOnscreenRendering{ false };
  bool UseStereoRendering{ false };
  int StereoType{ 0 };
  double EyeSeparation{ 0.06 };
  std::vector<std::pair<std::string, vtkLogger::Verbosity>> LogFiles;
  std::string LocalHostname;
  int ConnectID{ 0 };
};

#define CHECK_READ_ONLY(x)                                                                         \
  vtkLogIfF(ERROR, x.ReadOnly, "Options is read-only!");                                           \
  if (x.ReadOnly)                                                                                  \
  {                                                                                                \
    return;                                                                                        \
  }

vtkStandardNewMacro(vtkPVApplicationOptions);
//----------------------------------------------------------------------------
vtkPVApplicationOptions::vtkPVApplicationOptions()
  : Internals(new vtkPVApplicationOptions::vtkInternals())
{
  auto& internals = (*this->Internals);
  // initialize host names
  vtksys::SystemInformation sys_info;
  sys_info.RunOSCheck();
  if (auto hostname = sys_info.GetHostname())
  {
    internals.LocalHostname = hostname;
  }
}

//----------------------------------------------------------------------------
vtkPVApplicationOptions::~vtkPVApplicationOptions() = default;

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetLogStdErrVerbosity(vtkLogger::Verbosity verbosity)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);
  internals.LogStdErrVerbosity = verbosity;
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkPVApplicationOptions::GetLogStdErrVerbosity() const
{
  const auto& internals = (*this->Internals);
  return internals.LogStdErrVerbosity;
}

//----------------------------------------------------------------------------
const std::vector<std::pair<std::string, vtkLogger::Verbosity>>&
vtkPVApplicationOptions::GetLogFiles() const
{
  const auto& internals = (*this->Internals);
  return internals.LogFiles;
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetLogFiles(
  const std::vector<std::pair<std::string, vtkLogger::Verbosity>>& files)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);
  internals.LogFiles = files;
}

//----------------------------------------------------------------------------
bool vtkPVApplicationOptions::GetDisableRegistry() const
{
  const auto& internals = (*this->Internals);
  return internals.DisableRegistry;
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetDisableRegistry(bool value)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);
  internals.DisableRegistry = value;
}

//----------------------------------------------------------------------------
const std::string& vtkPVApplicationOptions::GetLocalHostname() const
{
  const auto& internals = (*this->Internals);
  return internals.LocalHostname;
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetLocalHostname(const std::string& value)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);
  internals.LocalHostname = value;
}

//----------------------------------------------------------------------------
bool vtkPVApplicationOptions::GetReadOnly() const
{
  const auto& internals = (*this->Internals);
  return internals.ReadOnly;
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetReadOnly(bool value)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);
  internals.ReadOnly = value;
}

//----------------------------------------------------------------------------
bool vtkPVApplicationOptions::GetDisableXDisplayTests() const
{
  const auto& internals = (*this->Internals);
  return internals.DisableXDisplayTests;
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetDisableXDisplayTests(bool value)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);
  internals.DisableXDisplayTests = value;
}

//----------------------------------------------------------------------------
bool vtkPVApplicationOptions::GetForceOnscreenRendering() const
{
  const auto& internals = (*this->Internals);
  return internals.ForceOnscreenRendering;
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetForceOnscreenRendering(bool value)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);
  internals.ForceOnscreenRendering = value;
}

//----------------------------------------------------------------------------
bool vtkPVApplicationOptions::GetForceOffscreenRendering() const
{
  const auto& internals = (*this->Internals);
  return internals.ForceOffscreenRendering;
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetForceOffscreenRendering(bool value)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);
  internals.ForceOffscreenRendering = value;
}

//----------------------------------------------------------------------------
bool vtkPVApplicationOptions::GetUseStereoRendering() const
{
  const auto& internals = (*this->Internals);
  return internals.UseStereoRendering;
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetUseStereoRendering(bool value)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);
  internals.UseStereoRendering = value;
}

//----------------------------------------------------------------------------
int vtkPVApplicationOptions::GetStereoType() const
{
  const auto& internals = (*this->Internals);
  return internals.StereoType;
}

//----------------------------------------------------------------------------
const char* vtkPVApplicationOptions::GetStereoTypeAsString() const
{
  switch (this->GetStereoType())
  {
    case VTK_STEREO_CRYSTAL_EYES:
      return "Crystal Eyes";
    case VTK_STEREO_RED_BLUE:
      return "Red-Blue";
    case VTK_STEREO_INTERLACED:
      return "Interlaced";
    case VTK_STEREO_LEFT:
      return "Left";
    case VTK_STEREO_RIGHT:
      return "Right";
    case VTK_STEREO_DRESDEN:
      return "Dresden";
    case VTK_STEREO_ANAGLYPH:
      return "Anaglyph";
    case VTK_STEREO_CHECKERBOARD:
      return "Checkerboard";
    case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
      return "SplitViewportHorizontal";
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetStereoType(int value)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);
  internals.StereoType = value;
}

//----------------------------------------------------------------------------
double vtkPVApplicationOptions::GetEyeSeparation() const
{
  const auto& internals = (*this->Internals);
  return internals.EyeSeparation;
}
//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetEyeSeparator(double value)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);
  internals.EyeSeparation = value;
}

//----------------------------------------------------------------------------
int vtkPVApplicationOptions::GetConnectID() const
{
  auto& internals = (*this->Internals);
  return internals.ConnectID;
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetConnectID(int value)
{
  auto& internals = (*this->Internals);
  // for now, we allow changing connect id even when ReadOnly is true.
  // CHECK_READ_ONLY(internals);
  internals.ConnectID = value;
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::Populate(CLI::App& app)
{
  auto& internals = (*this->Internals);
  CHECK_READ_ONLY(internals);

  app.add_flag("-d,--dr,--disable-registry", internals.DisableRegistry,
    "Skip user-specific applications settings and configuration options.");

  app
    .add_option("-v,--verbosity", internals.LogStdErrVerbosity,
      "Log verbosity on stderr as an integer in range [-9, 9] "
      "or INFO, WARNING, ERROR, or OFF. Defaults to INFO(0).")
    ->transform(
      [](const std::string& value) {
        auto xformedValue = vtkLogger::ConvertToVerbosity(value.c_str());
        if (xformedValue == vtkLogger::VERBOSITY_INVALID)
        {
          throw CLI::ValidationError("Invalid verbosity specified!");
        }
        return std::to_string(xformedValue);
      },
      "verbosity");

  //---------------------------------------------------------------------------
  // Debugging options
  //---------------------------------------------------------------------------
  auto* groupLogging = app.add_option_group("Debugging / Logging", "Logging and debugging options");
  groupLogging->add_flag(
    "--enable-bt", internals.EnableStackTrace, "Generate stack-trace on crash, if possible.");

  groupLogging
    ->add_option(
      "-l,--log",
      [&internals](const CLI::results_t& results) {
        for (const auto& value : results)
        {
          const auto separator = value.find_last_of(',');
          if (separator != std::string::npos)
          {
            const auto verbosityString = value.substr(separator + 1);
            const auto verbosity = vtkLogger::ConvertToVerbosity(verbosityString.c_str());
            if (verbosity == vtkLogger::VERBOSITY_INVALID)
            {
              vtkLogF(ERROR, "Invalid verbosity specified '%s'", verbosityString.c_str());
              // invalid verbosity specified.
              return false;
            }
            // remove the ",..." part from filename.
            internals.LogFiles.emplace_back(value.substr(0, separator), verbosity);
          }
          else
          {
            internals.LogFiles.emplace_back(value, vtkLogger::VERBOSITY_INFO);
          }
        }
        return true;
      },
      "Additional log files to generate. Can be specified multiple times. "
      "By default, log verbosity is set to INFO(0) and may be "
      "overridden per file by adding suffix `,verbosity` where verbosity values "
      "are same as those accepted for `--verbosity` argument.")
    ->delimiter('+') // reset delimiter. For log files ',' is used to separate verbosity.
    ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll)
    ->type_name("TEXT:filename[,ENUM:verbosity] ...");

  //---------------------------------------------------------------------------
  // Connection specific options
  //---------------------------------------------------------------------------
  auto groupConnection = app.add_option_group(
    "Connection options", "Options affecting connections between client/server processes");

  groupConnection
    ->add_option("--connect-id", internals.ConnectID,
      "An identifier used to match client-server connections. When non-zero, the client and server "
      "processes must used identical identifier for the connection to succeed.")
#if PARAVIEW_ALWAYS_SECURE_CONNECTION
    ->required();
#else
    ->default_val(internals.ConnectID);
#endif

  groupConnection
    ->add_option("--hostname", internals.LocalHostname,
      "Override the hostname to be used to connect to this process. "
      "By default, the hostname is determined using appropriate system calls.")
    ->default_val(internals.LocalHostname);

  auto renderingGroup = app.add_option_group("Rendering", "Rendering specific options");
  auto ofnr =
    renderingGroup
      ->add_flag("--force-offscreen-rendering", internals.ForceOffscreenRendering,
        "If supported by the build and platform, create headless (offscreen) render windows "
        "for rendering results.")
      ->default_val(internals.ForceOffscreenRendering);

  renderingGroup
    ->add_flag("--force-onscreen-rendering", internals.ForceOnscreenRendering,
      "If supported by the build and platform, create on-screen render windows "
      "for rendering results.")
    ->excludes(ofnr)
    ->default_val(internals.ForceOnscreenRendering)
    ->envname("PV_DEBUG_REMOTE_RENDERING");

  //---------------------------------------------------------------------------
  // Display specific options
  //---------------------------------------------------------------------------
  auto displayGroup =
    app.add_option_group("Display Environment", "Display/Device specific settings.");

  displayGroup
    ->add_flag("--disable-xdisplay-test", internals.DisableXDisplayTests,
      "Skip all X-display tests and OpenGL version checks. Use this option if "
      "you are getting remote-rendering disabled errors and you are positive that "
      "the X environment is set up properly and your OpenGL support is adequate (experimental).")
    ->default_val(internals.DisableXDisplayTests);

  //---------------------------------------------------------------------------
  // Stereo specific options
  //---------------------------------------------------------------------------
  auto* stereoGroup = app.add_option_group("Stereo", "Stereo rendering options");
  stereoGroup->add_flag("--stereo", internals.UseStereoRendering, "Enable stereo rendering.");
  stereoGroup
    ->add_option("--stereo-type", internals.StereoType,
      "Specify the stereo type to use. Possible values are 'Crystal Eyes', "
      "'Red-Blue', 'Interlaced', 'Dresden', 'Anaglyph', 'Checkerboard', or "
      "'SplitViewportHorizontal'.")
    ->needs("--stereo")
    ->transform([](const std::string& value) { return std::to_string(::ParseStereoType(value)); });

  stereoGroup
    ->add_option("--eye-separation", internals.EyeSeparation, "Specify eye separation distance.")
    ->needs("--stereo");
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::SetupDefaults(CLI::App& app)
{
  app.formatter(std::make_shared<PVFormatter>());
  app.prefix_command(true);
  // ensures that `,` is treated as multi-option separator.
  app.option_defaults()->delimiter(',');
}

//----------------------------------------------------------------------------
void vtkPVApplicationOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
