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

import math
from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import app, image, matrix, run


REFERENCE_INTENSITY = 1000

LMAXES_MULTI = [4, 0, 0]
LMAXES_SINGLE = [4, 0]


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('mtnorm', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)'
                    ' and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)')
  parser.set_synopsis('Normalise a DWI series to the estimated b=0 CSF intensity')
  parser.add_description('This algorithm determines an appropriate global scaling factor to apply to a DWI series '
                         'such that after the scaling is applied, '
                         'the b=0 CSF intensity corresponds to some reference value '
                         f'({REFERENCE_INTENSITY} by default).')
  parser.add_description('The operation of this script is a subset of that performed by the script "dwibiasnormmask". '
                         'Many users may find that comprehensive solution preferable; '
                         'this dwinormalise algorithm is nevertheless provided to demonstrate '
                         'specifically the global intensituy normalisation portion of that command.')
  parser.add_description('The ODFs estimated within this optimisation procedure are by default of lower maximal '
                         'spherical harmonic degree than what would be advised for analysis. '
                         'This is done for computational efficiency. '
                         'This behaviour can be modified through the -lmax command-line option.')
  parser.add_citation('Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. '
                      'Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. '
                      'NeuroImage, 2014, 103, 411-426')
  parser.add_citation('Raffelt, D.; Dhollander, T.; Tournier, J.-D.; Tabbara, R.; Smith, R. E.; Pierre, E. & Connelly, A. '
                      'Bias Field Correction and Intensity Normalisation for Quantitative Analysis of Apparent Fibre Density. '
                      'In Proc. ISMRM, 2017, 26, 3541')
  parser.add_citation('Dhollander, T.; Tabbara, R.; Rosnarho-Tornstrand, J.; Tournier, J.-D.; Raffelt, D. & Connelly, A. '
                      'Multi-tissue log-domain intensity and inhomogeneity normalisation for quantitative apparent fibre density. '
                      'In Proc. ISMRM, 2021, 29, 2472')
  parser.add_citation('Dhollander, T.; Raffelt, D. & Connelly, A. '
                      'Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. '
                      'ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5')
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input DWI series')
  parser.add_argument('output',
                      type=app.Parser.ImageOut(),
                      help='The normalised DWI series')
  options = parser.add_argument_group('Options specific to the "mtnorm" algorithm')
  options.add_argument('-lmax',
                       type=app.Parser.SequenceInt(),
                       help='The maximum spherical harmonic degree for the estimated FODs (see Description); '
                            f'defaults are "{",".join(map(str, LMAXES_MULTI))}" for multi-shell '
                            f'and "{",".join(map(str, LMAXES_SINGLE))}" for single-shell data)')
  options.add_argument('-mask',
                       type=app.Parser.ImageIn(),
                       help='Provide a mask image for relevant calculations '
                            '(if not provided, the default dwi2mask algorithm will be used)')
  options.add_argument('-reference',
                       type=app.Parser.Float(0.0),
                       default=REFERENCE_INTENSITY,
                       help='Set the target CSF b=0 intensity in the output DWI series '
                            f'(default: {REFERENCE_INTENSITY})')
  options.add_argument('-scale',
                       type=app.Parser.FileOut(),
                       help='Write the scaling factor applied to the DWI series to a text file')
  app.add_dwgrad_import_options(parser)



def execute(): #pylint: disable=unused-variable

  # Verify user inputs
  lmax = None
  if app.ARGS.lmax:
    lmax = app.ARGS.lmax
    if any(value < 0 or value % 2 for value in lmax):
      raise MRtrixError('lmax values must be non-negative even integers')
    if len(lmax) not in [2, 3]:
      raise MRtrixError('Length of lmax vector expected to be either 2 or 3')

  # Get input data into the scratch directory
  app.activate_scratch_dir()
  run.command(['mrconvert', app.ARGS.input, 'input.mif']
               + app.dwgrad_import_options(),
               preserve_pipes=True)
  if app.ARGS.mask:
    run.command(['mrconvert', app.ARGS.mask, 'mask.mif', '-datatype', 'bit'],
                preserve_pipes=True)

  # Make sure we have a valid mask available
  if app.ARGS.mask:
    if not image.match('input.mif', 'mask.mif', up_to_dim=3):
      raise MRtrixError('Provided mask image does not match input DWI')
  else:
    run.command(['dwi2mask', CONFIG['Dwi2maskAlgorithm'], 'input.mif', 'mask.mif'])

  # Determine whether we are working with single-shell or multi-shell data
  bvalues = [
      int(round(float(value)))
      for value in image.mrinfo('input.mif', 'shell_bvalues') \
                               .strip().split()]
  multishell = len(bvalues) > 2
  if lmax is None:
    lmax = LMAXES_MULTI if multishell else LMAXES_SINGLE
  elif len(lmax) == 3 and not multishell:
    raise MRtrixError('User specified 3 lmax values for three-tissue decomposition, '
                      'but input DWI is not multi-shell')

  # RF estimation and multi-tissue CSD
  class Tissue(object): #pylint: disable=useless-object-inheritance
    def __init__(self, name):
      self.name = name
      self.tissue_rf = f'response_{name}.txt'
      self.fod = f'FOD_{name}.mif'
      self.fod_norm = f'FODnorm_{name}.mif'

  tissues = [Tissue('WM'), Tissue('GM'), Tissue('CSF')]

  run.command('dwi2response dhollander input.mif -mask mask.mif '
              f'{" ".join(tissue.tissue_rf for tissue in tissues)}')

  # Immediately remove GM if we can't deal with it
  if not multishell:
    app.cleanup(tissues[1].tissue_rf)
    tissues = tissues[::2]

  run.command('dwi2fod msmt_csd input.mif '
              f'-lmax {",".join(map(str, lmax))} '
              + ' '.join(f'{tissue.tissue_rf} {tissue.fod}'
                         for tissue in tissues))

  # Normalisation in brain mask
  run.command('maskfilter mask.mif erode - | '
              'mtnormalise -mask - -balanced '
              '-check_factors factors.txt '
              + ' '.join(f'{tissue.fod} {tissue.fod_norm}'
                         for tissue in tissues))
  app.cleanup([tissue.fod for tissue in tissues])
  app.cleanup([tissue.fod_norm for tissue in tissues])

  csf_rf = matrix.load_matrix(tissues[-1].tissue_rf)
  app.cleanup([tissue.tissue_rf for tissue in tissues])
  csf_rf_bzero_lzero = csf_rf[0][0]
  balance_factors = matrix.load_vector('factors.txt')
  app.cleanup('factors.txt')
  csf_balance_factor = balance_factors[-1]
  scale_multiplier = (app.ARGS.reference * math.sqrt(4.0*math.pi)) / (csf_rf_bzero_lzero / csf_balance_factor)

  run.command(f'mrcalc input.mif {scale_multiplier} -mult - | '
              f'mrconvert - {app.ARGS.output}',
              mrconvert_keyval=app.ARGS.input,
              force=app.FORCE_OVERWRITE)

  if app.ARGS.scale:
    matrix.save_vector(app.ARGS.scale,
                       [scale_multiplier],
                       force=app.FORCE_OVERWRITE)
