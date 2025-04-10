/* Copyright (c) 2008-2024 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#pragma once

#ifdef FLUSH_TO_ZERO
#include <xmmintrin.h>
#endif

#include "app.h"
#include "executable_version.h"
#include "mrtrix.h"
#include "mrtrix_version.h"
#ifdef MRTRIX_PROJECT
namespace MR {
namespace App {
void set_project_version();
}
} // namespace MR
#endif

#define MRTRIX_UPDATED_API

#ifdef MRTRIX_AS_R_LIBRARY

extern "C" void R_main(int *cmdline_argc, char **cmdline_argv) {
#ifdef MRTRIX_PROJECT
  ::MR::App::set_project_version();
#endif
  ::MR::App::DESCRIPTION.clear();
  ::MR::App::ARGUMENTS.clear();
  ::MR::App::OPTIONS.clear();
  try {
    usage();
    ::MR::App::verify_usage();
    ::MR::App::init(*cmdline_argc, cmdline_argv);
    ::MR::App::parse();
    run();
  } catch (MR::Exception &E) {
    E.display();
    return;
  } catch (int retval) {
    return;
  }
}

extern "C" void R_usage(char **output) {
  ::MR::App::DESCRIPTION.clear();
  ::MR::App::ARGUMENTS.clear();
  ::MR::App::OPTIONS.clear();
  usage();
  std::string s = MR::App::full_usage();
  *output = new char[s.size() + 1];
  strncpy(*output, s.c_str(), s.size() + 1);
}

#else

int main(int cmdline_argc, char **cmdline_argv) {
  if (MR::App::mrtrix_version != MR::App::mrtrix_executable_version) {
    MR::Exception E("executable was compiled for a different version of the MRtrix3 library!");
    E.push_back(std::string("  ") + MR::App::NAME + " version: " + MR::App::mrtrix_executable_version);
    E.push_back(std::string("  library version: ") + MR::App::mrtrix_version);
    E.push_back("You may need to erase files left over from prior MRtrix3 versions;");
    E.push_back("eg. core/version.cpp; src/exec_version.cpp");
    E.push_back(", and re-configure cmake");
    throw E;
  }

#ifdef FLUSH_TO_ZERO
  // use gcc switches: -msse -mfpmath=sse -ffast-math
  int mxcsr = _mm_getcsr();
  // Sets denormal results from floating-point calculations to zero:
  mxcsr |= (1 << 15) | (1 << 11); // flush-to-zero
  // Treats denormal values used as input to floating-point instructions as zero:
  mxcsr |= (1 << 6); // denormals-are-zero
  _mm_setcsr(mxcsr);
#endif
#ifdef MRTRIX_PROJECT
  ::MR::App::set_project_version();
#endif
  try {
    ::MR::App::init(cmdline_argc, cmdline_argv);
    usage();
    ::MR::App::verify_usage();
    ::MR::App::parse_special_options();
#ifdef GUI_APP_H
    ::MR::GUI::App app(cmdline_argc, cmdline_argv);
#endif
    ::MR::App::parse();

    // ENVVAR name: MRTRIX_CLI_PARSE_ONLY
    // ENVVAR Set the command to parse the provided inputs and then quit
    // ENVVAR if it is set. This can be used in the CI of wrapping code,
    // ENVVAR such as the automatically generated Pydra interfaces.
    // ENVVAR Note that it will have no effect for R interfaces
    char *parse_only = std::getenv("MRTRIX_CLI_PARSE_ONLY");
    if (parse_only && ::MR::to<bool>(parse_only)) {
      CONSOLE("Quitting after parsing command-line arguments successfully due to environment variable "
              "'MRTRIX_CLI_PARSE_ONLY'");
      return 0;
    }
    run();
  } catch (::MR::Exception &E) {
    E.display();
    return 1;
  } catch (int retval) {
    return retval;
  }
  return ::MR::App::exit_error_code;
}

#endif
