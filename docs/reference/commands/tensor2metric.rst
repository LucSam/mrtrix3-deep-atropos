.. _tensor2metric:

tensor2metric
===================

Synopsis
--------

Generate maps of tensor-derived parameters

Usage
--------

::

    tensor2metric [ options ]  tensor

-  *tensor*: the input tensor image.

Options
-------

-  **-mask image** only perform computation within the specified binary brain mask image.

Diffusion Tensor Imaging
^^^^^^^^^^^^^^^^^^^^^^^^

-  **-adc image** compute the mean apparent diffusion coefficient (ADC) of the diffusion tensor. (sometimes also referred to as the mean diffusivity (MD))

-  **-fa image** compute the fractional anisotropy (FA) of the diffusion tensor.

-  **-ad image** compute the axial diffusivity (AD) of the diffusion tensor. (equivalent to the principal eigenvalue)

-  **-rd image** compute the radial diffusivity (RD) of the diffusion tensor. (equivalent to the mean of the two non-principal eigenvalues)

-  **-value image** compute the selected eigenvalue(s) of the diffusion tensor.

-  **-vector image** compute the selected eigenvector(s) of the diffusion tensor.

-  **-num sequence** specify the desired eigenvalue/eigenvector(s). Note that several eigenvalues can be specified as a number sequence. For example, '1,3' specifies the principal (1) and minor (3) eigenvalues/eigenvectors (default = 1).

-  **-modulate choice** specify how to modulate the magnitude of the eigenvectors. Valid choices are: none, FA, eigval (default = FA).

-  **-cl image** compute the linearity metric of the diffusion tensor. (one of the three Westin shape metrics)

-  **-cp image** compute the planarity metric of the diffusion tensor. (one of the three Westin shape metrics)

-  **-cs image** compute the sphericity metric of the diffusion tensor. (one of the three Westin shape metrics)

Diffusion Kurtosis Imaging
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-dkt image** input diffusion kurtosis tensor.

-  **-mk image** compute the mean kurtosis (MK) of the kurtosis tensor.

-  **-ak image** compute the axial kurtosis (AK) of the kurtosis tensor.

-  **-rk image** compute the radial kurtosis (RK) of the kurtosis tensor.

-  **-mk_dirs file** specify the directions used to numerically calculate mean kurtosis (by default, the built-in 300 direction set is used). These should be supplied as a text file containing [ az el ] pairs for the directions.

-  **-rk_ndirs integer** specify the number of directions used to numerically calculate radial kurtosis (by default, 300 directions are used).

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

Basser, P. J.; Mattiello, J. & Lebihan, D. MR diffusion tensor spectroscopy and imaging. Biophysical Journal, 1994, 66, 259-267

* If using -cl, -cp or -cs options:  |br|
  Westin, C. F.; Peled, S.; Gudbjartsson, H.; Kikinis, R. & Jolesz, F. A. Geometrical diffusion measures for MRI from tensor basis analysis. Proc Intl Soc Mag Reson Med, 1997, 5, 1742

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Ben Jeurissen (ben.jeurissen@uantwerpen.be) and Thijs Dhollander (thijs.dhollander@gmail.com) and J-Donald Tournier (jdtournier@gmail.com)

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


