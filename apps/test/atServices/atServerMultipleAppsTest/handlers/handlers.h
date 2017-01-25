/**
 * This is the header file for handlers functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#ifndef _HANDLERS_H
#define _HANDLERS_H

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of commands per session
 */
//--------------------------------------------------------------------------------------------------
#define COMMANDS_MAX    50

//------------------------------------------------------------------------------
/**
 * struct AtCmd defintion
 *
 */
//------------------------------------------------------------------------------
typedef struct
{
    const char* atCmdPtr;                           ///< at command pointer
    le_atServer_CmdRef_t cmdRef;                    ///< command reference
    le_atServer_CommandHandlerFunc_t handlerPtr;    ///< handler func pointer
}
atCmd_t;

//--------------------------------------------------------------------------------------------------
/**
 * struct AtSession definition
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_atServer_DeviceRef_t devRef; ///< device reference
    int cmdsCount;                  ///< number of registred at commands
    atCmd_t atCmds[COMMANDS_MAX];   ///< at commands array
}
atSession_t;

void GenericCmdHandler
(
    le_atServer_CmdRef_t commandRef,    ///< command reference
    le_atServer_Type_t type,            ///< command type
    uint32_t parametersNumber,          ///< number of command parameters
    void* contextPtr                    ///< context pointer
);

void AtCmdHandler
(
    le_atServer_CmdRef_t commandRef,    ///< command reference
    le_atServer_Type_t type,            ///< command type
    uint32_t parametersNumber,          ///< number of command parameters
    void* contextPtr                    ///< context pointer
);

void DelCmdHandler
(
    le_atServer_CmdRef_t commandRef,    ///< command reference
    le_atServer_Type_t type,            ///< command type
    uint32_t parametersNumber,          ///< number of command parameters
    void* contextPtr                    ///< context pointer
);

void CloseCmdHandler
(
    le_atServer_CmdRef_t commandRef,    ///< command reference
    le_atServer_Type_t type,            ///< command type
    uint32_t parametersNumber,          ///< number of command parameters
    void* contextPtr                    ///< context pointer
);

#endif /* handlers.h */
