/** @file pa_mrc_simu.h
 *
 * Legato @ref pa_mrc_simu include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef PA_MRC_SIMU_H_INCLUDE_GUARD
#define PA_MRC_SIMU_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * This function set the current Radio Access Technology in use.
 */
//--------------------------------------------------------------------------------------------------
void pa_mrcSimu_SetRadioAccessTechInUse
(
    le_mrc_Rat_t   rat  ///< [IN] The Radio Access Technology.
);

#endif // PA_MRC_SIMU_H_INCLUDE_GUARD

