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

import math, os



def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('In a FreeSurfer parcellation image,'
                       ' replace the sub-cortical grey matter structure delineations using FSL FIRST')
  cmdline.add_citation('Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. '
                       'A Bayesian model of shape and appearance for subcortical brain segmentation. '
                       'NeuroImage, 2011, 56, 907-922',
                       is_external=True)
  cmdline.add_citation('Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. '
                       'Advances in functional and structural MR image analysis and implementation as FSL. '
                       'NeuroImage, 2004, 23, S208-S219',
                       is_external=True)
  cmdline.add_citation('Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. '
                       'The effects of SIFT on the reproducibility and biological accuracy of the structural connectome. '
                       'NeuroImage, 2015, 104, 253-265')
  cmdline.add_argument('parc',
                       type=app.Parser.ImageIn(),
                       help='The input FreeSurfer parcellation image')
  cmdline.add_argument('t1',
                       type=app.Parser.ImageIn(),
                       help='The T1 image to be provided to FIRST')
  cmdline.add_argument('lut',
                       type=app.Parser.FileIn(),
                       help='The lookup table file that the parcellated image is based on')
  cmdline.add_argument('output',
                       type=app.Parser.ImageOut(),
                       help='The output parcellation image')
  cmdline.add_argument('-premasked',
                       action='store_true',
                       default=None,
                       help='Indicate that brain masking has been applied to the T1 input image')
  cmdline.add_argument('-sgm_amyg_hipp',
                       action='store_true',
                       default=None,
                       help='Consider the amygdalae and hippocampi as sub-cortical grey matter structures,'
                            ' and also replace their estimates with those from FIRST')






def execute(): #pylint: disable=unused-variable
  from mrtrix3 import MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, fsl, image, path, run, utils #pylint: disable=no-name-in-module, import-outside-toplevel

  if utils.is_windows():
    raise MRtrixError('Script cannot run on Windows due to FSL dependency')

  image.check_3d_nonunity(app.ARGS.t1)

  fsl_path = os.environ.get('FSLDIR', '')
  if not fsl_path:
    raise MRtrixError('Environment variable FSLDIR is not set; '
                      'please run appropriate FSL configuration script')

  first_cmd = fsl.exe_name('run_first_all')

  first_atlas_path = os.path.join(fsl_path, 'data', 'first', 'models_336_bin')
  if not os.path.isdir(first_atlas_path):
    raise MRtrixError('Atlases required for FSL\'s FIRST program not installed; '
                      'please install fsl-first-data using your relevant package manager')

  # Want a mapping between FreeSurfer node names and FIRST structure names
  # Just deal with the 5 that are used in ACT; FreeSurfer's hippocampus / amygdala segmentations look good enough.
  structure_map = { 'L_Accu':'Left-Accumbens-area',  'R_Accu':'Right-Accumbens-area',
                    'L_Caud':'Left-Caudate',         'R_Caud':'Right-Caudate',
                    'L_Pall':'Left-Pallidum',        'R_Pall':'Right-Pallidum',
                    'L_Puta':'Left-Putamen',         'R_Puta':'Right-Putamen',
                    'L_Thal':'Left-Thalamus-Proper', 'R_Thal':'Right-Thalamus-Proper' }
  if app.ARGS.sgm_amyg_hipp:
    structure_map.update({ 'L_Amyg':'Left-Amygdala',    'R_Amyg':'Right-Amygdala',
                           'L_Hipp':'Left-Hippocampus', 'R_Hipp':'Right-Hippocampus' })

  t1_spacing = image.Header(app.ARGS.t1).spacing()
  upsample_for_first = False
  # If voxel size is 1.25mm or larger, make a guess that the user has erroneously re-gridded their data
  if math.pow(t1_spacing[0] * t1_spacing[1] * t1_spacing[2], 1.0/3.0) > 1.225:
    app.warn(f'Voxel size of input T1 image larger than expected for T1-weighted images ({t1_spacing}); '
             f'image will be resampled to 1mm isotropic in order to maximise chance of '
             f'FSL FIRST script succeeding')
    upsample_for_first = True

  app.activate_scratch_dir()
  # Get the parcellation and T1 images into the scratch directory, with conversion of the T1 into the correct format for FSL
  run.command(['mrconvert', app.ARGS.parc, 'parc.mif'],
              preserve_pipes=True)
  if upsample_for_first:
    run.command(f'mrgrid {app.ARGS.t1} regrid - -voxel 1.0 -interp sinc | '
                'mrcalc - 0.0 -max - | '
                'mrconvert - T1.nii -strides -1,+2,+3',
                preserve_pipes=True)
  else:
    run.command(f'mrconvert {app.ARGS.t1} T1.nii -strides -1,+2,+3',
                preserve_pipes=True)

  # Run FIRST
  first_input_is_brain_extracted = ''
  if app.ARGS.premasked:
    first_input_is_brain_extracted = ' -b'
  structures_string = ','.join(structure_map.keys())
  run.command(f'{first_cmd} -m none -s {structures_string} -i T1.nii {first_input_is_brain_extracted} -o first')
  fsl.check_first('first', structure_map.keys())

  # Generate an empty image that will be used to construct the new SGM nodes
  run.command('mrcalc parc.mif 0 -min sgm.mif')

  # Read the local connectome LUT file
  # This will map a structure name to an index
  sgm_lut = {}
  sgm_lut_file_name = 'FreeSurferSGM.txt'
  sgm_lut_file_path = os.path.join(path.shared_data_path(), 'labelsgmfirst', sgm_lut_file_name)
  with open(sgm_lut_file_path, encoding='utf-8') as sgm_lut_file:
    for line in sgm_lut_file:
      line = line.rstrip()
      if line and line[0]!='#':
        line = line.split()
        sgm_lut[line[1]] = line[0] # This can remain as a string

  # Convert FIRST meshes to node masks
  # In this use case, don't want the PVE images; want to threshold at 0.5
  mask_list = [ ]
  progress = app.ProgressBar('Generating mask images for SGM structures', len(structure_map))
  for key, value in structure_map.items():
    image_path = f'{key}_mask.mif'
    mask_list.append(image_path)
    vtk_in_path = f'first-{key}_first.vtk'
    run.command(f'meshconvert {vtk_in_path} first-{key}_transformed.vtk -transform first2real T1.nii')
    run.command(f'mesh2voxel first-{key}_transformed.vtk parc.mif - | '
                f'mrthreshold - {image_path} -abs 0.5')
    # Add to the SGM image; don't worry about overlap for now
    node_index = sgm_lut[value]
    run.command(f'mrcalc {image_path} {node_index} sgm.mif -if sgm_new.mif')
    if not app.CONTINUE_OPTION:
      run.function(os.remove, 'sgm.mif')
      run.function(os.rename, 'sgm_new.mif', 'sgm.mif')
    progress.increment()
  progress.done()

  # Detect any overlapping voxels between the SGM masks, and set to zero
  run.command(['mrmath', mask_list, 'sum', '-', '|', \
               'mrcalc', '-', '1', '-gt', 'sgm_overlap_mask.mif'])
  run.command('mrcalc sgm_overlap_mask.mif 0 sgm.mif -if sgm_masked.mif')

  # Convert the SGM label image to the indices that are required based on the user-provided LUT file
  run.command(['labelconvert', 'sgm_masked.mif', sgm_lut_file_path, app.ARGS.lut, 'sgm_new_labels.mif'])

  # For each SGM structure:
  # * Figure out what index the structure has been mapped to; this can only be done using mrstats
  # * Strip that index from the parcellation image
  # * Insert the new delineation of that structure
  progress = app.ProgressBar('Replacing SGM parcellations', len(structure_map))
  for struct in structure_map:
    image_path = f'{struct}_mask.mif'
    index = int(image.statistics('sgm_new_labels.mif', mask=image_path).median)
    run.command(f'mrcalc parc.mif {index} -eq 0 parc.mif -if parc_removed.mif')
    run.function(os.remove, 'parc.mif')
    run.function(os.rename, 'parc_removed.mif', 'parc.mif')
    progress.increment()
  progress.done()

  # Insert the new delineations of all SGM structures in a single call
  # Enforce unsigned integer datatype of output image
  run.command('mrcalc sgm_new_labels.mif 0.5 -gt sgm_new_labels.mif parc.mif -if result.mif -datatype uint32')
  run.command(['mrconvert', 'result.mif', app.ARGS.output],
              mrconvert_keyval=app.ARGS.parc,
              force=app.FORCE_OVERWRITE,
              preserve_pipes=True)
