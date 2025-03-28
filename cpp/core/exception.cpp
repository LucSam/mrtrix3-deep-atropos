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

#include "exception.h"
#include "app.h"
#include "debug.h"
#include "file/config.h"

#ifdef MRTRIX_AS_R_LIBRARY
#include "wrap_r.h"
#endif

namespace MR {

void display_exception_cmdline(const Exception &E, int log_level) {
  if (App::log_level >= log_level)
    for (size_t n = 0; n < E.description.size(); ++n)
      report_to_user_func(E.description[n], log_level);
}

bool __need_newline = false;

namespace {

inline const char *console_prefix(int type) {
  switch (type) {
  case 0:
    return "[ERROR] ";
  case 1:
    return "[WARNING] ";
  case 2:
    return "[INFO] ";
  case 3:
    return "[DEBUG] ";
  default:
    return "";
  }
}

} // namespace

void cmdline_report_to_user_func(const std::string &msg, int type) {
  static constexpr const char *colour_format_strings[] = {"%s: %s%s\n",
                                                          "%s: \033[01;31m%s%s\033[0m\n",
                                                          "%s: \033[00;31m%s%s\033[0m\n",
                                                          "%s: \033[00;32m%s%s\033[0m\n",
                                                          "%s: \033[00;34m%s%s\033[0m\n"};

  if (__need_newline) {
    __print_stderr("\n");
    __need_newline = false;
  }

  auto clamp = [](int t) {
    if (t < -1 || t > 3)
      t = -1;
    return t + 1;
  };

  __print_stderr(printf(colour_format_strings[App::terminal_use_colour ? clamp(type) : 0],
                        App::NAME.c_str(),
                        console_prefix(type),
                        msg.c_str()));
  if (type == 1 && App::fail_on_warn)
    throw Exception("terminating due to request to fail on warning");
}

void cmdline_print_func(const std::string &msg) {
#ifdef MRTRIX_AS_R_LIBRARY
  Rprintf(msg.c_str());
#else
  std::cout << msg;
#endif
}

void (*print)(const std::string &msg) = cmdline_print_func;
void (*report_to_user_func)(const std::string &msg, int type) = cmdline_report_to_user_func;
void (*Exception::display_func)(const Exception &E, int log_level) = display_exception_cmdline;

void check_app_exit_code() {
  if (App::exit_error_code)
    throw Exception("Command performing delayed termination due to prior critical error");
}

} // namespace MR
