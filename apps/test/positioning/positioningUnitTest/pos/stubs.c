#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference stub for le_posCtrl
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_posCtrl_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference stub for le_posCtrl
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_posCtrl_GetClientSessionRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference stub for le_pos
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_pos_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference stub for le_pos
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_pos_GetClientSessionRef
(
    void
)
{
    return NULL;
}

le_cfg_ChangeHandlerRef_t le_cfg_AddChangeHandler
(
    const char *newPath,
    le_cfg_ChangeHandlerFunc_t handlerPtr,
    void *contextPtr
)
{
    return NULL;
}

void le_cfg_CancelTxn
(
    le_cfg_IteratorRef_t iteratorRef
)
{
}

void le_cfg_CommitTxn
(
    le_cfg_IteratorRef_t iteratorRef
)
{
}

le_cfg_IteratorRef_t le_cfg_CreateReadTxn
(
    const char *basePath
)
{
    return NULL;
}

le_cfg_IteratorRef_t le_cfg_CreateWriteTxn
(
    const char *basePath
)
{
    return NULL;
}

int32_t le_cfg_GetInt
(
    le_cfg_IteratorRef_t iteratorRef,
    const char *path,
    int32_t defaultValue
)
{
    return 0;
}

void le_cfg_SetInt
(
    le_cfg_IteratorRef_t iteratorRef,
    const char *path,
    int32_t value
)
{
}
