//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's header file for the defines used in dcs_utils.c
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DCS_UTILS_H_INCLUDE_GUARD
#define LEGATO_DCS_UTILS_H_INCLUDE_GUARD


#if LE_CONFIG_ENABLE_CONFIG_TREE
//--------------------------------------------------------------------------------------------------
/**
 * The following are the config tree defines for session cleanup filtering. They are paths & nodes
 * for specifying whether a client app session's data connection should be allowed to stay or be
 * closed upon its exit. If to stay, both the data channel is left open and its client app's
 * previously added channel event handler registered. In the present, this filtering is solely used
 * by Legato's app tool "cm data [connect | disconnect]", and not advertised for external apps'
 * use. This mechanism is necessary for the cm tool because each "cm data [connect | disconnect]"
 * command is executed transactionally with its process exited upon execution completion while
 * needing the opened data connection to remain active.
 *
 * However, this filtering mechanism's isn't custom-made in its design & implementation for the cm
 * tool and can be used by other tools & component as well, as illustrated below.
 *
 * The paths are:
 *   dataConnectionService:/sessionCleanup/<appName>
 *   dataConnectionService:/sessionCleanup/<tech>/<channelName>/<reqRef>
 *   dataConnectionService:/sessionCleanup/<tech>/<channelName>/<handlerRef>
 * where <appName> is the app's name using this mechanism, <tech> is of le_dcs_Technology_t type
 * which is LE_DCS_TECH_CELLULAR, the only type supported in the present, <channelName> the name
 * of the channel, reqRef the start request reference.
 *
 * The use of this mechanism is all within a client app's responsibility, including when to set
 * & clear the parameters set on these config tree paths.
 *
 * Once these configs are deleted from these paths, upon the client app's exit, typical connection
 * closure, if not yet stopped, and event handler removal will be carried out as cleanup as usual.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_PATH_SESSION_CLEANUP    "sessionCleanup"
#define CFG_SESSION_CLEANUP_REQREF  "reqRef"
#define CFG_SESSION_CLEANUP_HANDLERREF "handlerRef"
#endif

#endif
