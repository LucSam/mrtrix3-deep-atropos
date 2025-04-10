find_program(BASH bash)

include(DirsToUnix)
set(EXEC_DIRS ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND EXEC_DIRS ${PROJECT_BINARY_DIR}/bin)
list(APPEND EXEC_DIRS ${PROJECT_BINARY_DIR}/testing/tools)
list(APPEND EXEC_DIRS "$ENV{PATH}")
dirs_to_unix(EXEC_DIRS "${EXEC_DIRS}")

include(BashTests)
function(add_bash_script_test file_path)
    set(extra_labels "")
    foreach(label ${ARGN})
        list(APPEND extra_labels ${label})
    endforeach()

    # Set folder name
    string(REPLACE "/" ";" folder_list ${file_path})
    list(GET folder_list 0 folder_name)

    add_bash_test(
        FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests/${file_path}"
        PREFIX "${folder_name}"
        WORKING_DIRECTORY ${SCRIPT_DATA_DIR}
        EXEC_DIRECTORIES "${EXEC_DIRS}"
        LABELS "script;${folder_name};${extra_labels}"
    )
endfunction()

add_bash_script_test(5ttgen/deepatropos_fromseg_default "pythonci")
add_bash_script_test(5ttgen/deepatropos_fromseg_whitestem)
add_bash_script_test(5ttgen/deepatropos_fromprob_default "pythonci")
add_bash_script_test(5ttgen/deepatropos_fromprob_whitestem)
add_bash_script_test(5ttgen/freesurfer_default "pythonci")
add_bash_script_test(5ttgen/freesurfer_nocrop)
add_bash_script_test(5ttgen/freesurfer_piping)
add_bash_script_test(5ttgen/freesurfer_sgmamyghipp)
add_bash_script_test(5ttgen/freesurfer_whitespace)
add_bash_script_test(5ttgen/fsl_default)
add_bash_script_test(5ttgen/fsl_mask)
add_bash_script_test(5ttgen/fsl_nocrop)
add_bash_script_test(5ttgen/fsl_piping)
add_bash_script_test(5ttgen/fsl_premasked)
add_bash_script_test(5ttgen/fsl_sgmamyghipp)
add_bash_script_test(5ttgen/fsl_whitespace)
add_bash_script_test(5ttgen/hsvs_aseg)
add_bash_script_test(5ttgen/hsvs_default)
add_bash_script_test(5ttgen/hsvs_first)
add_bash_script_test(5ttgen/hsvs_modules)
add_bash_script_test(5ttgen/hsvs_piping)
add_bash_script_test(5ttgen/hsvs_template)
add_bash_script_test(5ttgen/hsvs_whitespace)
add_bash_script_test(5ttgen/hsvs_whitestem)

add_bash_script_test(dwi2mask/3dautomask_default)
add_bash_script_test(dwi2mask/3dautomask_options)
add_bash_script_test(dwi2mask/3dautomask_piping)
add_bash_script_test(dwi2mask/3dautomask_whitespace)
add_bash_script_test(dwi2mask/ants_config)
add_bash_script_test(dwi2mask/ants_piping)
add_bash_script_test(dwi2mask/ants_template)
add_bash_script_test(dwi2mask/ants_whitespace)
add_bash_script_test(dwi2mask/b02template_antsfull_clioptions)
add_bash_script_test(dwi2mask/b02template_antsfull_configfile)
add_bash_script_test(dwi2mask/b02template_antsfull_configoptions)
add_bash_script_test(dwi2mask/b02template_antsfull_template)
add_bash_script_test(dwi2mask/b02template_antsquick_config)
add_bash_script_test(dwi2mask/b02template_antsquick_template)
add_bash_script_test(dwi2mask/b02template_fsl_default)
add_bash_script_test(dwi2mask/b02template_fsl_flirtconfig)
add_bash_script_test(dwi2mask/b02template_fsl_fnirtcnf)
add_bash_script_test(dwi2mask/b02template_fsl_fnirtconfig)
add_bash_script_test(dwi2mask/b02template_piping)
add_bash_script_test(dwi2mask/b02template_whitespace)
add_bash_script_test(dwi2mask/consensus_default "pythonci")
add_bash_script_test(dwi2mask/consensus_piping)
add_bash_script_test(dwi2mask/consensus_template)
add_bash_script_test(dwi2mask/consensus_whitespace)
add_bash_script_test(dwi2mask/fslbet_default)
add_bash_script_test(dwi2mask/fslbet_options)
add_bash_script_test(dwi2mask/fslbet_piping)
add_bash_script_test(dwi2mask/fslbet_rescale)
add_bash_script_test(dwi2mask/fslbet_whitespace)
add_bash_script_test(dwi2mask/hdbet_default)
add_bash_script_test(dwi2mask/hdbet_piping)
add_bash_script_test(dwi2mask/hdbet_whitespace)
add_bash_script_test(dwi2mask/legacy_default "pythonci")
add_bash_script_test(dwi2mask/legacy_piping)
add_bash_script_test(dwi2mask/legacy_whitespace)
add_bash_script_test(dwi2mask/mean_default "pythonci")
add_bash_script_test(dwi2mask/mean_piping)
add_bash_script_test(dwi2mask/mean_whitespace)
add_bash_script_test(dwi2mask/mtnorm_default "pythonci")
add_bash_script_test(dwi2mask/mtnorm_initmask)
add_bash_script_test(dwi2mask/mtnorm_lmax)
add_bash_script_test(dwi2mask/mtnorm_piping)
add_bash_script_test(dwi2mask/mtnorm_whitespace)
add_bash_script_test(dwi2mask/synthstrip_default)
add_bash_script_test(dwi2mask/synthstrip_options)
add_bash_script_test(dwi2mask/synthstrip_piping)
add_bash_script_test(dwi2mask/synthstrip_whitespace)
add_bash_script_test(dwi2mask/trace_default "pythonci")
add_bash_script_test(dwi2mask/trace_iterative)
add_bash_script_test(dwi2mask/trace_piping)
add_bash_script_test(dwi2mask/trace_whitespace)

add_bash_script_test(dwi2response/dhollander_default)
add_bash_script_test(dwi2response/dhollander_fslgrad "pythonci")
add_bash_script_test(dwi2response/dhollander_grad)
add_bash_script_test(dwi2response/dhollander_lmax)
add_bash_script_test(dwi2response/dhollander_mask)
add_bash_script_test(dwi2response/dhollander_piping)
add_bash_script_test(dwi2response/dhollander_shells)
add_bash_script_test(dwi2response/dhollander_whitespace)
add_bash_script_test(dwi2response/dhollander_wmalgofa)
add_bash_script_test(dwi2response/dhollander_wmalgotax)
add_bash_script_test(dwi2response/dhollander_wmalgotournier)
add_bash_script_test(dwi2response/fa_default)
add_bash_script_test(dwi2response/fa_fslgrad "pythonci")
add_bash_script_test(dwi2response/fa_grad)
add_bash_script_test(dwi2response/fa_lmax)
add_bash_script_test(dwi2response/fa_mask)
add_bash_script_test(dwi2response/fa_piping)
add_bash_script_test(dwi2response/fa_shells)
add_bash_script_test(dwi2response/fa_threshold)
add_bash_script_test(dwi2response/fa_whitespace)
add_bash_script_test(dwi2response/manual_default)
add_bash_script_test(dwi2response/manual_dirs)
add_bash_script_test(dwi2response/manual_fslgrad "pythonci")
add_bash_script_test(dwi2response/manual_grad)
add_bash_script_test(dwi2response/manual_lmax)
add_bash_script_test(dwi2response/manual_piping)
add_bash_script_test(dwi2response/manual_shells)
add_bash_script_test(dwi2response/manual_whitespace)
add_bash_script_test(dwi2response/msmt5tt_default)
add_bash_script_test(dwi2response/msmt5tt_dirs)
add_bash_script_test(dwi2response/msmt5tt_fslgrad "pythonci")
add_bash_script_test(dwi2response/msmt5tt_grad)
add_bash_script_test(dwi2response/msmt5tt_lmax)
add_bash_script_test(dwi2response/msmt5tt_mask)
add_bash_script_test(dwi2response/msmt5tt_piping)
add_bash_script_test(dwi2response/msmt5tt_sfwmfa)
add_bash_script_test(dwi2response/msmt5tt_shells)
add_bash_script_test(dwi2response/msmt5tt_whitespace)
add_bash_script_test(dwi2response/msmt5tt_wmalgotax)
add_bash_script_test(dwi2response/tax_default)
add_bash_script_test(dwi2response/tax_fslgrad "pythonci")
add_bash_script_test(dwi2response/tax_grad)
add_bash_script_test(dwi2response/tax_lmax)
add_bash_script_test(dwi2response/tax_mask)
add_bash_script_test(dwi2response/tax_piping)
add_bash_script_test(dwi2response/tax_shell)
add_bash_script_test(dwi2response/tax_whitespace)
add_bash_script_test(dwi2response/tournier_default)
add_bash_script_test(dwi2response/tournier_fslgrad "pythonci")
add_bash_script_test(dwi2response/tournier_grad)
add_bash_script_test(dwi2response/tournier_lmax)
add_bash_script_test(dwi2response/tournier_mask)
add_bash_script_test(dwi2response/tournier_piping)
add_bash_script_test(dwi2response/tournier_shell)
add_bash_script_test(dwi2response/tournier_whitespace)

add_bash_script_test(dwibiascorrect/ants_default)
add_bash_script_test(dwibiascorrect/ants_mask)
add_bash_script_test(dwibiascorrect/ants_piping)
add_bash_script_test(dwibiascorrect/ants_whitespace)
add_bash_script_test(dwibiascorrect/fsl_default)
add_bash_script_test(dwibiascorrect/fsl_masked)
add_bash_script_test(dwibiascorrect/fsl_piping)
add_bash_script_test(dwibiascorrect/fsl_whitespace)
add_bash_script_test(dwibiascorrect/mtnorm_default "pythonci")
add_bash_script_test(dwibiascorrect/mtnorm_lmax)
add_bash_script_test(dwibiascorrect/mtnorm_masked)
add_bash_script_test(dwibiascorrect/mtnorm_piping)
add_bash_script_test(dwibiascorrect/mtnorm_whitespace)

add_bash_script_test(dwibiasnormmask/default "pythonci")
add_bash_script_test(dwibiasnormmask/lmax)
add_bash_script_test(dwibiasnormmask/maxiters)
add_bash_script_test(dwibiasnormmask/piping)
add_bash_script_test(dwibiasnormmask/reference)
add_bash_script_test(dwibiasnormmask/scaled)
add_bash_script_test(dwibiasnormmask/whitespace)

add_bash_script_test(dwicat/3dimage)
add_bash_script_test(dwicat/default_ownbzero "pythonci")
add_bash_script_test(dwicat/default_sharedbzero)
add_bash_script_test(dwicat/nointensity)
add_bash_script_test(dwicat/piping)
add_bash_script_test(dwicat/rigidbody)
add_bash_script_test(dwicat/whitespace)

add_bash_script_test(dwifslpreproc/axis_padding)
add_bash_script_test(dwifslpreproc/eddyqc)
add_bash_script_test(dwifslpreproc/permuted_volumes)
add_bash_script_test(dwifslpreproc/piping)
add_bash_script_test(dwifslpreproc/rpeall)
add_bash_script_test(dwifslpreproc/rpeheader_none)
add_bash_script_test(dwifslpreproc/rpeheader_onerevbzero)
add_bash_script_test(dwifslpreproc/rpeheader_rpepair)
add_bash_script_test(dwifslpreproc/rpenone_default)
add_bash_script_test(dwifslpreproc/rpepair_alignseepi)
add_bash_script_test(dwifslpreproc/rpepair_default)
add_bash_script_test(dwifslpreproc/whitespace)

add_bash_script_test(dwigradcheck/default)
add_bash_script_test(dwigradcheck/piping)
add_bash_script_test(dwigradcheck/whitespace)

add_bash_script_test(dwinormalise/group_default "pythonci")
add_bash_script_test(dwinormalise/group_piping)
add_bash_script_test(dwinormalise/group_whitespace)
add_bash_script_test(dwinormalise/manual_default "pythonci")
add_bash_script_test(dwinormalise/manual_percentile)
add_bash_script_test(dwinormalise/manual_piping)
add_bash_script_test(dwinormalise/manual_whitespace)
add_bash_script_test(dwinormalise/mtnorm_default "pythonci")
add_bash_script_test(dwinormalise/mtnorm_lmax)
add_bash_script_test(dwinormalise/mtnorm_masked)
add_bash_script_test(dwinormalise/mtnorm_piping)
add_bash_script_test(dwinormalise/mtnorm_reference)
add_bash_script_test(dwinormalise/mtnorm_scaled)
add_bash_script_test(dwinormalise/mtnorm_whitespace)

add_bash_script_test(dwishellmath/default "pythonci")
add_bash_script_test(dwishellmath/oneshell)
add_bash_script_test(dwishellmath/piping)
add_bash_script_test(dwishellmath/whitespace)

add_bash_script_test(for_each/echo "pythonci")
add_bash_script_test(for_each/exclude "pythonci")
add_bash_script_test(for_each/parallel "pythonci")

add_bash_script_test(labelsgmfirst/default)
add_bash_script_test(labelsgmfirst/freesurfer)
add_bash_script_test(labelsgmfirst/piping)
add_bash_script_test(labelsgmfirst/premasked)
add_bash_script_test(labelsgmfirst/sgm_amyg_hipp)
add_bash_script_test(labelsgmfirst/whitespace)

add_bash_script_test(mask2glass/default "pythonci")
add_bash_script_test(mask2glass/no4dseries "pythonci")
add_bash_script_test(mask2glass/piping)
add_bash_script_test(mask2glass/whitespace)

add_bash_script_test(mrtrix_cleanup/default "pythonci")
add_bash_script_test(mrtrix_cleanup/test "pythonci")
add_bash_script_test(mrtrix_cleanup/whitespace)

add_bash_script_test(population_template/fa_affine)
add_bash_script_test(population_template/fa_affinenonlinear)
add_bash_script_test(population_template/fa_default "pythonci")
add_bash_script_test(population_template/fa_initaligngeometric)
add_bash_script_test(population_template/fa_initalignnone)
add_bash_script_test(population_template/fa_masked)
add_bash_script_test(population_template/fa_nonlinear)
add_bash_script_test(population_template/fa_rigid)
add_bash_script_test(population_template/fa_rigidaffine)
add_bash_script_test(population_template/fa_rigidnonlinear)
add_bash_script_test(population_template/fa_voxelsize)
add_bash_script_test(population_template/fod_default "pythonci")
add_bash_script_test(population_template/fod_options)
add_bash_script_test(population_template/piping)
add_bash_script_test(population_template/whitespace)

add_bash_script_test(responsemean/default "pythonci")
add_bash_script_test(responsemean/legacy "pythonci")
add_bash_script_test(responsemean/whitespace "pythonci")

