//--------------------------------------------------------------------------------------------------
/**
 * @file instStat.h
 *
 * The instStat functions are used to let interested third parties know if an application has been
 * installed or removed.  These applications may not be directly involved in the install process,
 * but may just need to know that the system has changed.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_INST_STAT_H_INCLUDE_GUARD
#define LEGATO_INST_STAT_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the instStat subsystem so that it is ready to report install and uninstall activity.
 */
//--------------------------------------------------------------------------------------------------
void instStat_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Report to anyone who may be listening that an application has just been installed in the system.
 */
//--------------------------------------------------------------------------------------------------
void instStat_ReportAppInstall
(
    const char* appNamePtr  ///< [IN] The name of the application to report.
);


//--------------------------------------------------------------------------------------------------
/**
 * Report that an application has been removed from the system.
 */
//--------------------------------------------------------------------------------------------------
void instStat_ReportAppUninstall
(
    const char* appNamePtr  ///< [IN] The name of the application to report.
);


#endif  // LEGATO_INST_STAT_H_INCLUDE_GUARD
