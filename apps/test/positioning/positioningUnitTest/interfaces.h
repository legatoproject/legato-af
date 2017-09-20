#ifndef _INTERFACES_H
#define _INTERFACES_H

#include "le_gnss_simu.h"
#include "le_pos_interface.h"
#include "le_gnss_interface.h"
#include "le_posCtrl_interface.h"
#include "le_cfg_interface.h"

#undef LE_KILL_CLIENT
#define LE_KILL_CLIENT LE_DEBUG

le_msg_ServiceRef_t le_posCtrl_GetServiceRef(void);
le_msg_SessionRef_t le_posCtrl_GetClientSessionRef(void);
le_msg_ServiceRef_t le_pos_GetServiceRef(void);
le_msg_SessionRef_t le_pos_GetClientSessionRef(void);
le_cfg_ChangeHandlerRef_t le_cfg_AddChangeHandler(const char *newPath,
                                le_cfg_ChangeHandlerFunc_t handlerPtr,
                                void *contextPtr);
void le_cfg_CancelTxn(le_cfg_IteratorRef_t iteratorRef);
void le_cfg_CommitTxn(le_cfg_IteratorRef_t iteratorRef);
le_cfg_IteratorRef_t le_cfg_CreateReadTxn(const char *basePath);
le_cfg_IteratorRef_t le_cfg_CreateWriteTxn(const char *basePath);
int32_t le_cfg_GetInt(le_cfg_IteratorRef_t iteratorRef, const char *path,
                int32_t defaultValue);
void le_cfg_SetInt(le_cfg_IteratorRef_t iteratorRef, const char *path,
                int32_t value);

#endif /* interfaces.h */
