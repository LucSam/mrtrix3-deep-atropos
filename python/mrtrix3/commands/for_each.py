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


import os, re, sys, threading



# Since we're going to capture everything after the colon character and "hide" it from argparse,
#   we need to store the contents from there in a global so as for it to be accessible from execute()
CMDSPLIT = [ ]



def usage(cmdline): #pylint: disable=unused-variable
  global CMDSPLIT
  from mrtrix3 import version #pylint: disable=no-name-in-module, import-outside-toplevel
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)')
  cmdline.set_synopsis('Perform some arbitrary processing step for each of a set of inputs')
  cmdline.add_description('This script greatly simplifies various forms of batch processing'
                          ' by enabling the execution of a command'
                          ' (or set of commands)'
                          ' independently for each of a set of inputs.')
  cmdline.add_description('More information on use of the for_each command can be found at the following link: \n'
                          f'https://mrtrix.readthedocs.io/en/{version.TAG}/tips_and_tricks/batch_processing_with_foreach.html')
  cmdline.add_description('The way that this batch processing capability is achieved'
                          ' is by providing basic text substitutions,'
                          ' which simplify the formation of valid command strings'
                          ' based on the unique components of the input strings'
                          ' on which the script is instructed to execute. '
                          'This does however mean that the items to be passed as inputs to the for_each command'
                          ' (e.g. file / directory names)'
                          ' MUST NOT contain any instances of these substitution strings,'
                          ' as otherwise those paths will be corrupted during the course of the substitution.')
  cmdline.add_description('The available substitutions are listed below '
                          '(note that the -test command-line option can be used'
                          ' to ensure correct command string formation prior to actually executing the commands):')
  cmdline.add_description('   - IN:   '
                          'The full matching pattern, including leading folders. '
                          'For example, if the target list contains a file "folder/image.mif", '
                          'any occurrence of "IN" will be substituted with "folder/image.mif".')
  cmdline.add_description('   - NAME: '
                          'The basename of the matching pattern. '
                          'For example, if the target list contains a file "folder/image.mif", '
                          'any occurrence of "NAME" will be substituted with "image.mif".')
  cmdline.add_description('   - PRE:  '
                          'The prefix of the input pattern '
                          '(the basename stripped of its extension). '
                          'For example, if the target list contains a file "folder/my.image.mif.gz", '
                          'any occurrence of "PRE" will be substituted with "my.image".')
  cmdline.add_description('   - UNI:  '
                          'The unique part of the input after removing any common prefix and common suffix. '
                          'For example, if the target list contains files: "folder/001dwi.mif", "folder/002dwi.mif", "folder/003dwi.mif", '
                          'any occurrence of "UNI" will be substituted with "001", "002", "003".')
  cmdline.add_description('Note that due to a limitation of the Python "argparse" module,'
                          ' any command-line OPTIONS that the user intends to provide specifically to the for_each script'
                          ' must appear BEFORE providing the list of inputs on which for_each is intended to operate.'
                          ' While command-line options provided as such will be interpreted specifically by the for_each script,'
                          ' any command-line options that are provided AFTER the COLON separator will form part of the executed COMMAND,'
                          ' and will therefore be interpreted as command-line options having been provided to that underlying command.')
  cmdline.add_example_usage('Demonstration of basic usage syntax',
                            'for_each folder/*.mif : mrinfo IN',
                            'This will run the "mrinfo" command for every .mif file present in "folder/".'
                            ' Note that the compulsory colon symbol is used'
                            ' to separate the list of items on which for_each is being instructed to operate'
                            ' from the command that is intended to be run for each input.')
  cmdline.add_example_usage('Multi-threaded use of for_each',
                            'for_each -nthreads 4 freesurfer/subjects/* : recon-all -subjid NAME -all',
                            'In this example, '
                            'for_each is instructed to run the FreeSurfer command "recon-all"'
                            ' for all subjects within the "subjects" directory,'
                            ' with four subjects being processed in parallel at any one time.'
                            ' Whenever processing of one subject is completed,'
                            ' processing for a new unprocessed subject will commence.'
                            ' This technique is useful for improving the efficiency'
                            ' of running single-threaded commands on multi-core systems,'
                            ' as long as the system possesses enough memory to support such parallel processing.'
                            ' Note that in the case of multi-threaded commands'
                            ' (which includes many MRtrix3 commands),'
                            ' it is generally preferable to permit multi-threaded execution'
                            ' of the command on a single input at a time,'
                            ' rather than processing multiple inputs in parallel.')
  cmdline.add_example_usage('Excluding specific inputs from execution',
                            'for_each *.nii -exclude 001.nii : mrconvert IN PRE.mif',
                            'Particularly when a wildcard is used to define the list of inputs for for_each,'
                            ' it is possible in some instances that this list will include one or more strings'
                            ' for which execution should in fact not be performed;'
                            ' for instance, if a command has already been executed for one or more files,'
                            ' and then for_each is being used to execute the same command for all other files.'
                            ' In this case,'
                            ' the -exclude option can be used to effectively remove an item'
                            ' from the list of inputs that would otherwise be included due to the use of a wildcard'
                            ' (and can be used more than once to exclude more than one string).'
                            ' In this particular example,'
                            ' mrconvert is instructed to perform conversions from NIfTI to MRtrix image formats,'
                            ' for all except the first image in the directory.'
                            ' Note that any usages of this option must appear AFTER the list of inputs.'
                            ' Note also that the argument following the -exclude option'
                            ' can alternatively be a regular expression,'
                            ' in which case any inputs for which a match to the expression is found'
                            ' will be excluded from processing.')
  cmdline.add_example_usage('Testing the command string substitution',
                            'for_each -test * : mrconvert IN PRE.mif',
                            'By specifying the -test option,'
                            ' the script will print to the terminal the results of text substitutions'
                            ' for all of the specified inputs,'
                            ' but will not actually execute those commands.'
                            ' It can therefore be used to verify that the script is receiving the intended set of inputs,'
                            ' and that the text substitutions on those inputs lead to the intended command strings.')
  cmdline.add_argument('inputs',
                       nargs='+',
                       help='Each of the inputs for which processing should be run')
  cmdline.add_argument('colon',
                       type=str,
                       choices=[':'],
                       help='Colon symbol (":") delimiting the for_each inputs & command-line options from the actual command to be executed')
  cmdline.add_argument('command',
                       type=str,
                       help='The command string to run for each input, '
                            'containing any number of substitutions listed in the Description section')
  cmdline.add_argument('-exclude',
                       action='append',
                       metavar='"regex"',
                       nargs=1,
                       help='Exclude one specific input string / all strings matching a regular expression from being processed '
                            '(see Example Usage)')
  cmdline.add_argument('-test',
                       action='store_true',
                       default=None,
                       help='Test the operation of the for_each script,'
                            ' by printing the command strings following string substitution but not actually executing them')

  # Usage of for_each needs to be handled slightly differently here:
  # We want argparse to parse only the contents of the command-line before the colon symbol,
  #   as these are the items that pertain to the invocation of the for_each script;
  #   anything after the colon should instead form a part of the command that
  #   for_each is responsible for executing
  try:
    index = next(i for i,s in enumerate(sys.argv) if s == ':')
    try:
      CMDSPLIT = sys.argv[index+1:]
      sys.argv = sys.argv[:index+1]
      sys.argv.append(' '.join(CMDSPLIT))
    except IndexError:
      sys.stderr.write('Erroneous usage: No command specified (colon separator cannot be the last entry provided)\n')
      sys.exit(0)
  except StopIteration:
    if len(sys.argv) > 2:
      sys.stderr.write('Erroneous usage: A colon must be used to separate for_each inputs from the command to be executed\n')
      sys.exit(0)








# These need to be globals in order to be accessible from execute_parallel()
class Shared:
  def __init__(self):
    self._job_index = 0
    self.lock = threading.Lock()
    self.stop = False
  def next(self, jobs):
    job = None
    with self.lock:
      if self._job_index < len(jobs):
        job = jobs[self._job_index]
        self._job_index += 1
        self.stop = self._job_index == len(jobs)
    return job

shared = Shared() #pylint: disable=invalid-name



KEYLIST = [ 'IN', 'NAME', 'PRE', 'UNI' ]



def execute(): #pylint: disable=unused-variable
  from mrtrix3 import ANSI, MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, run #pylint: disable=no-name-in-module, import-outside-toplevel

  inputs = app.ARGS.inputs
  app.debug(f'All inputs: {inputs}')
  app.debug(f'Command: {app.ARGS.command}')
  app.debug(f'CMDSPLIT: {CMDSPLIT}')

  if app.ARGS.exclude:
    app.ARGS.exclude = [ exclude[0] for exclude in app.ARGS.exclude ] # To deal with argparse's action=append. Always guaranteed to be only one argument since nargs=1
    app.debug(f'To exclude: {app.ARGS.exclude}')
    exclude_unmatched = [ ]
    to_exclude = [ ]
    for exclude in app.ARGS.exclude:
      if exclude in inputs:
        to_exclude.append(exclude)
      else:
        try:
          re_object = re.compile(exclude)
          regex_hits = [ ]
          for arg in inputs:
            search_result = re_object.search(arg)
            if search_result and search_result.group():
              regex_hits.append(arg)
          if regex_hits:
            app.debug(f'Inputs excluded via regex "{exclude}": {regex_hits}')
            to_exclude.extend(regex_hits)
          else:
            app.debug(f'Compiled exclude regex "{exclude}" had no hits')
            exclude_unmatched.append(exclude)
        except re.error:
          app.debug(f'Exclude string "{exclude}" did not compile as regex')
          exclude_unmatched.append(exclude)
    if exclude_unmatched:

      app.warn(f'{"Items" if len(exclude_unmatched) > 1 else "Item"} '
               'specified via -exclude did not result in item exclusion, '
               'whether by direct match or compilation as regex: '
               + (f'"{exclude_unmatched[0]}"' if len(exclude_unmatched) == 1 else str(exclude_unmatched)))
    inputs = [ arg for arg in inputs if arg not in to_exclude ]
    if not inputs:
      raise MRtrixError(f'No inputs remaining after application of exclusion {"criterion" if len(app.ARGS.exclude) == 1 else "criteria"}')
    app.debug(f'Inputs after exclusion: {inputs}')

  common_prefix = os.path.commonprefix(inputs)
  common_suffix = os.path.commonprefix([i[::-1] for i in inputs])[::-1]
  app.debug(f'Common prefix: {common_prefix}' if common_prefix else 'No common prefix')
  app.debug(f'Common suffix: {common_suffix}' if common_suffix else 'No common suffix')

  for entry in CMDSPLIT:
    if os.path.exists(entry):
      keys_present = [ key for key in KEYLIST if key in entry ]
      if keys_present:
        app.warn(f'Performing text substitution of {keys_present} within command: "{entry}"; '
                 f'but the original text exists as a path on the file system... '
                 f'is this a problematic filesystem path?')

  try:
    next(entry for entry in CMDSPLIT if any(key for key in KEYLIST if key in entry))
  except StopIteration as exception:
    raise MRtrixError(f'None of the unique for_each keys {KEYLIST} appear in command string "{app.ARGS.command}"; '
                      f'no substitution can occur') from exception

  class Entry:
    def __init__(self, input_text):
      self.input_text = input_text
      self.sub_in = input_text
      self.sub_name = os.path.basename(input_text.rstrip('/'))
      self.sub_pre = os.path.splitext(self.sub_name.rstrip('.gz'))[0]
      if common_suffix:
        self.sub_uni = input_text[len(common_prefix):-len(common_suffix)]
      else:
        self.sub_uni = input_text[len(common_prefix):]

      self.substitutions = { 'IN': self.sub_in, 'NAME': self.sub_name, 'PRE': self.sub_pre, 'UNI': self.sub_uni }
      app.debug(f'Input text: {input_text}')
      app.debug(f'Substitutions: {self.substitutions}')

      self.cmd = [ ]
      for entry in CMDSPLIT:
        for (key, value) in self.substitutions.items():
          entry = entry.replace(key, value)
        if ' ' in entry:
          entry = '"' + entry + '"'
        self.cmd.append(entry)
      app.debug(f'Resulting command: {self.cmd}')

      self.outputtext = None
      self.returncode = None

  jobs = [ ]
  for i in inputs:
    jobs.append(Entry(i))

  if app.ARGS.test:
    app.console(f'Command strings for {len(jobs)} jobs:')
    for job in jobs:
      sys.stderr.write(ANSI.execute + 'Input:' + ANSI.clear + f'   "{job.input_text}"\n')
      sys.stderr.write(ANSI.execute + 'Command:' + ANSI.clear + f' {" ".join(job.cmd)}\n')
    return

  parallel = app.NUM_THREADS is not None and app.NUM_THREADS > 1

  def progress_string():
    success_count = sum(1 if job.returncode is not None else 0 for job in jobs)
    fail_count = sum(1 if job.returncode else 0 for job in jobs)
    threading_message = f'across {app.NUM_THREADS} threads' if parallel else 'sequentially'
    fail_message = f' ({fail_count} errors)' if fail_count else ''
    return f'{success_count}/{len(jobs)} jobs completed {threading_message}{fail_message}'

  progress = app.ProgressBar(progress_string(), len(jobs))

  def execute_parallel():
    while not shared.stop:
      my_job = shared.next(jobs)
      if not my_job:
        return
      try:
        result = run.command(' '.join(my_job.cmd), shell=True)
        my_job.outputtext = result.stdout + result.stderr
        my_job.returncode = 0
      except run.MRtrixCmdError as exception:
        my_job.outputtext = str(exception)
        my_job.returncode = exception.returncode
      except Exception as exception: # pylint: disable=broad-except
        my_job.outputtext = str(exception)
        my_job.returncode = 1
      with shared.lock:
        progress.increment(progress_string())

  if parallel:
    threads = [ ]
    for i in range (1, app.NUM_THREADS):
      thread = threading.Thread(target=execute_parallel)
      thread.start()
      threads.append(thread)
    execute_parallel()
    for thread in threads:
      thread.join()
  else:
    for job in jobs:
      try:
        result = run.command(' '.join(job.cmd), shell=True)
        job.outputtext = result.stdout + result.stderr
        job.returncode = 0
      except run.MRtrixCmdError as exception:
        job.outputtext = str(exception)
        job.returncode = exception.returncode
      except Exception as exception: # pylint: disable=broad-except
        job.outputtext = str(exception)
        job.returncode = 1
      progress.increment(progress_string())

  progress.done()

  assert all(job.returncode is not None for job in jobs)
  fail_count = sum(1 if job.returncode else 0 for job in jobs)
  if fail_count:
    app.warn(f'{fail_count} of {len(jobs)} jobs did not complete successfully')
    if fail_count > 1:
      app.warn('Outputs from failed commands:')
      sys.stderr.write(f'{app.EXEC_NAME}:\n')
    else:
      app.warn('Output from failed command:')
    for job in jobs:
      if job.returncode:
        if job.outputtext:
          app.warn(f'For input "{job.sub_in}" (returncode = {job.returncode}):')
          for line in job.outputtext.splitlines():
            sys.stderr.write(f'{" " * (len(app.EXEC_NAME)+2)}{line}\n')
        else:
          app.warn(f'No output from command for input "{job.sub_in}" (return code = {job.returncode})')
        if fail_count > 1:
          sys.stderr.write(f'{app.EXEC_NAME}:\n')
    raise MRtrixError(f'{fail_count} of {len(jobs)} jobs did not complete successfully: '
                      f'{[job.input_text for job in jobs if job.returncode]}')

  if app.VERBOSITY > 1:
    if any(job.outputtext for job in jobs):
      sys.stderr.write('{app.EXE_NAME}:\n')
      for job in jobs:
        if job.outputtext:
          app.console(f'Output of command for input "{job.sub_in}":')
          for line in job.outputtext.splitlines():
            sys.stderr.write(f'{" " * (len(app.EXEC_NAME)+2)}{line}\n')
        else:
          app.console(f'No output from command for input "{job.sub_in}"')
        sys.stderr.write('{app.EXE_NAME}:\n')
    else:
      app.console('No output from command for any inputs')

  app.console('Script reported successful completion for all inputs')
