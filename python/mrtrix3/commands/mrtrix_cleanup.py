# Copyright (c) 2008-2024 the MRtrix3 contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Covered Software is provided under this License on an "as is"
# basis, without warranty of any kind, either expressed, implied, or
# statutory, including, without limitation, warranties that the
# Covered Software is free of defects, merchantable, fit for a
# particular purpose or non-infringing.
# See the Mozilla Public License v. 2.0 for more details.
#
# For more details, see http://www.mrtrix.org/.


import math, os, re, shutil


POSTFIXES = [ 'B', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB' ]


def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel

  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Clean up residual temporary files & scratch directories from MRtrix3 commands')
  cmdline.add_description('This script will search the file system at the specified location'
                          ' (and in sub-directories thereof)'
                          ' for any temporary files or directories that have been left behind'
                          ' by failed or terminated MRtrix3 commands,'
                          ' and attempt to delete them.')
  cmdline.add_description('Note that the script\'s search for temporary items'
                          ' will not extend beyond the user-specified filesystem location.'
                          ' This means that any built-in or user-specified default location'
                          ' for MRtrix3 piped data and scripts will not be automatically searched.'
                          ' Cleanup of such locations should instead be performed explicitly:'
                          ' e.g. "mrtrix_cleanup /tmp/" to remove residual piped images from /tmp/.')
  cmdline.add_description('This script should not be run while other MRtrix3 commands are being executed:'
                          ' it may delete temporary items during operation'
                          ' that may lead to unexpected behaviour.')
  cmdline.add_argument('path',
                       type=app.Parser.DirectoryIn(),
                       help='Directory from which to commence filesystem search')
  cmdline.add_argument('-test',
                       action='store_true',
                       default=None,
                       help='Run script in test mode:'
                            ' will list identified files / directories,'
                            ' but not attempt to delete them')
  cmdline.add_argument('-failed',
                       type=app.Parser.FileOut(),
                       help='Write list of items that the script failed to delete to a text file')
  cmdline.flag_mutually_exclusive_options([ 'test', 'failed' ])



def execute(): #pylint: disable=unused-variable
  from mrtrix3 import CONFIG #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel

  file_regex = re.compile(r"^mrtrix-tmp-[a-zA-Z0-9]{6}\..*$")
  file_config_regex = re.compile(r"^" + CONFIG['TmpFilePrefix'] + r"[a-zA-Z0-9]{6}\..*$") \
                      if 'TmpFilePrefix' in CONFIG and CONFIG['TmpFilePrefix'] != 'mrtrix-tmp-' \
                      else None
  dir_regex = re.compile(r"^\w+-tmp-[a-zA-Z0-9]{6}$")
  dir_config_regex = re.compile(r"^" + CONFIG['ScriptScratchPrefix'] + r"[a-zA-Z0-9]{6}$") \
                      if 'ScriptScratchPrefix' in CONFIG \
                      else None

  files_to_delete = [ ]
  dirs_to_delete = [ ]
  root_dir = os.path.abspath(app.ARGS.path)
  print_search_dir = ('' if os.path.abspath(os.getcwd()) == root_dir else ' from ' + root_dir)
  def file_search(regex):
    files_to_delete.extend([ os.path.join(dirname, filename) for filename in filter(regex.search, filelist) ])
  def dir_search(regex):
    items = set(filter(regex.search, subdirlist))
    if items:
      dirs_to_delete.extend([os.path.join(dirname, subdirname) for subdirname in items])
      subdirlist[:] = list(set(subdirlist)-items)
  def print_msg():
    return f'Searching{print_search_dir} (found {len(files_to_delete)} files, {len(dirs_to_delete)} directories)'
  progress = app.ProgressBar(print_msg)
  for dirname, subdirlist, filelist in os.walk(root_dir):
    file_search(file_regex)
    if file_config_regex:
      file_search(file_config_regex)
    dir_search(dir_regex)
    if dir_config_regex:
      dir_search(dir_config_regex)
    progress.increment()
  progress.done()

  if app.ARGS.test:
    if files_to_delete:
      app.console(f'Files identified ({len(files_to_delete)}):')
      for filepath in files_to_delete:
        app.console(f'  {filepath}')
    else:
      app.console(f'No files{"" if dirs_to_delete else " or directories"} found')
    if dirs_to_delete:
      app.console(f'Directories identified ({len(dirs_to_delete)}):')
      for dirpath in dirs_to_delete:
        app.console(f'  {dirpath}')
    elif files_to_delete:
      app.console('No directories identified')
  elif files_to_delete or dirs_to_delete:
    progress = app.ProgressBar(f'Deleting temporaries '
                               f'({len(files_to_delete)} files, {len(dirs_to_delete)} directories)',
                               len(files_to_delete) + len(dirs_to_delete))
    except_list = [ ]
    size_deleted = 0
    for filepath in files_to_delete:
      filesize = 0
      try:
        filesize = os.path.getsize(filepath)
        os.remove(filepath)
        size_deleted += filesize
      except OSError:
        except_list.append(filepath)
      progress.increment()
    for dirpath in dirs_to_delete:
      dirsize = 0
      try:
        for dirname, subdirlist, filelist in os.walk(dirpath):
          dirsize += sum(os.path.getsize(filename) for filename in filelist)
      except OSError:
        pass
      try:
        shutil.rmtree(dirpath)
        size_deleted += dirsize
      except OSError:
        except_list.append(dirpath)
      progress.increment()
    progress.done()
    postfix_index = int(math.floor(math.log(size_deleted, 1024))) if size_deleted else 0
    if postfix_index:
      size_deleted = round(size_deleted / math.pow(1024, postfix_index), 2)
    def print_freed():
      return f' ({size_deleted} {POSTFIXES[postfix_index]} freed)' if size_deleted else ''
    if except_list:
      app.console('%d of %d items erased%s' # pylint: disable=consider-using-f-string
                  % (len(files_to_delete) + len(dirs_to_delete) - len(except_list),
                     len(files_to_delete) + len(dirs_to_delete),
                     print_freed()))
      if app.ARGS.failed:
        with open(app.ARGS.failed, 'w', encoding='utf-8') as outfile:
          for item in except_list:
            outfile.write(item + '\n')
        app.console(f'List of items script failed to erase written to file "{app.ARGS.failed}"')
      else:
        app.console('Items that could not be erased:')
        for item in except_list:
          app.console(f'  {item}')
    else:
      app.console('All items deleted successfully' + print_freed())
  else:
    app.console('No files or directories found')
