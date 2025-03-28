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

import importlib



def usage(cmdline): #pylint: disable=unused-variable

  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Generate a 5TT image suitable for ACT')
  cmdline.add_citation('Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. '
                       'Anatomically-constrained tractography:'
                       ' Improved diffusion MRI streamlines tractography through effective use of anatomical information. '
                       'NeuroImage, 2012, 62, 1924-1938')
  cmdline.add_description('5ttgen acts as a "master" script'
                          ' for generating a five-tissue-type (5TT) segmented tissue image'
                          ' suitable for use in Anatomically-Constrained Tractography (ACT).'
                          ' A range of different algorithms are available for completing this task.'
                          ' When using this script,'
                          ' the name of the algorithm to be used must appear'
                          ' as the first argument on the command-line after "5ttgen".'
                          ' The subsequent compulsory arguments and options available'
                          ' depend on the particular algorithm being invoked.')
  cmdline.add_description('Each algorithm available also has its own help page,'
                          ' including necessary references;'
                          ' e.g. to see the help page of the "fsl" algorithm, type "5ttgen fsl".')

  common_options = cmdline.add_argument_group('Options common to all 5ttgen algorithms')
  common_options.add_argument('-nocrop',
                              action='store_true',
                              default=None,
                              help='Do NOT crop the resulting 5TT image to reduce its size '
                                   '(keep the same dimensions as the input image)')
  common_options.add_argument('-sgm_amyg_hipp',
                              action='store_true',
                              default=None,
                              help='Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image')

  # Import the command-line settings for all algorithms found in the relevant directory
  cmdline.add_subparsers()



def execute(): #pylint: disable=unused-variable
  from mrtrix3 import app, run #pylint: disable=no-name-in-module, import-outside-toplevel

  # Load module for the user-requested algorithm
  alg = importlib.import_module(f'.{app.ARGS.algorithm}', 'mrtrix3.commands.5ttgen')

  app.activate_scratch_dir()

  result_path = alg.execute()
  if result_path:
    stderr = run.command(['5ttcheck', result_path]).stderr
    if '[WARNING]' in stderr:
      app.warn('Generated image does not perfectly conform to 5TT format:')
      for line in stderr.splitlines():
        app.warn(line)
