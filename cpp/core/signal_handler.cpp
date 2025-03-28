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

#include "signal_handler.h"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <signal.h>
#include <vector>

#include "app.h"
#include "file/path.h"

#ifdef MRTRIX_WINDOWS
#define STDERR_FILENO 2
#endif

namespace MR::SignalHandler {

namespace {
std::vector<cleanup_function_type> cleanup_operations;

std::vector<std::string> marked_files;
std::atomic_flag flag = ATOMIC_FLAG_INIT;

void delete_temporary_files() {
  for (const auto &i : marked_files)
    std::remove(i.c_str());
  marked_files.clear();
}

void handler(int i) noexcept {
  // Only process this once if using multi-threading:
  if (!flag.test_and_set()) {

    // Try to do a tempfile cleanup before printing the error, since the latter's not guaranteed to work...
    // Don't use File::remove: may throw an exception
    for (auto func : cleanup_operations)
      func();

    const char *sig = nullptr;
    const char *msg = nullptr;
    switch (i) {

#define __SIGNAL(SIG, MSG)                                                                                             \
  case SIG:                                                                                                            \
    sig = #SIG;                                                                                                        \
    msg = MSG;                                                                                                         \
    break;
#include "signals.h"
#undef __SIGNAL

    default:
      sig = "UNKNOWN";
      msg = "Unknown fatal system signal";
      break;
    }

    // Don't use std::cerr << here: Use basic C string-handling functions and a write() call to STDERR_FILENO
    // Don't attempt to use any terminal colouring
    char str[256];
    str[255] = '\0';
    snprintf(str, 255, "\n%s: [SYSTEM FATAL CODE: %s (%d)] %s\n", App::NAME.c_str(), sig, i, msg);
    if (write(STDERR_FILENO, str, strnlen(str, 256)) == 0)
      std::_Exit(i);
    else
      std::_Exit(i);
  }
}

} // namespace

void init() {
  on_signal(delete_temporary_files);

  // ENVVAR name: MRTRIX_NOSIGNALS
  // ENVVAR If this variable is set to any value, disable MRtrix3's custom
  // ENVVAR signal handlers. This may sometimes be useful when debugging.
  // ENVVAR Note however that this prevents the
  // ENVVAR deletion of temporary files when the command terminates
  // ENVVAR abnormally.
  if (getenv("MRTRIX_NOSIGNALS"))
    return;

#ifdef MRTRIX_WINDOWS
    // Use signal() rather than sigaction() for Windows, as the latter is not supported
#define __SIGNAL(SIG, MSG) signal(SIG, handler)
#else
  // Construct the signal structure
  struct sigaction act;
  act.sa_handler = &handler;
  // Since we're _Exit()-ing for any of these signals, block them all
  sigfillset(&act.sa_mask);
  act.sa_flags = 0;
#define __SIGNAL(SIG, MSG) sigaction(SIG, &act, nullptr)
#endif

#include "signals.h"
}

void on_signal(cleanup_function_type func) {
  cleanup_operations.push_back(func);
  std::atexit(func);
}

void mark_file_for_deletion(const std::string &filename) {
  while (!flag.test_and_set())
    ;
  marked_files.push_back(filename);
  flag.clear();
}

void unmark_file_for_deletion(const std::string &filename) {
  while (!flag.test_and_set())
    ;
  auto i = marked_files.begin();
  while (i != marked_files.end()) {
    if (*i == filename)
      i = marked_files.erase(i);
    else
      ++i;
  }
  flag.clear();
}

} // namespace MR::SignalHandler
