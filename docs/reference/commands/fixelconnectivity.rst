.. _fixelconnectivity:

fixelconnectivity
===================

Synopsis
--------

Generate a fixel-fixel connectivity matrix

Usage
--------

::

    fixelconnectivity [ options ]  fixel_directory tracks matrix

-  *fixel_directory*: the directory containing the fixels between which connectivity will be quantified
-  *tracks*: the tracks used to determine fixel-fixel connectivity
-  *matrix*: the output fixel-fixel connectivity matrix directory path

Description
-----------

This command will generate a directory containing three images, which encodes the fixel-fixel connectivity matrix. Documentation regarding this format and how to use it will come in the future.

Fixel data are stored utilising the fixel directory format described in the main documentation, which can be found at the following link:  |br|
https://mrtrix.readthedocs.io/en/3.0.4/fixel_based_analysis/fixel_directory_format.html

Options
-------

Options that influence generation of the connectivity matrix / matrices
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-threshold value** a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: 0.01)

-  **-angle value** the max angle threshold for assigning streamline tangents to fixels (Default: 45 degrees)

-  **-mask file** provide a fixel data file containing a mask of those fixels to be computed; fixels outside the mask will be empty in the output matrix

-  **-tck_weights_in path** specify a text scalar file containing the streamline weights

Options for additional outputs to be generated
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-count path** export a fixel data file encoding the number of connections for each fixel

-  **-extent path** export a fixel data file encoding the extent of connectivity (sum of weights) for each fixel

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2024 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Covered Software is provided under this License on an "as is"
basis, without warranty of any kind, either expressed, implied, or
statutory, including, without limitation, warranties that the
Covered Software is free of defects, merchantable, fit for a
particular purpose or non-infringing.
See the Mozilla Public License v. 2.0 for more details.

For more details, see http://www.mrtrix.org/.


