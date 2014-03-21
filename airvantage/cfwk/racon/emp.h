/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

/**
 * @file
 * @brief Provides EMP API with some threading stuff.
 *
 *
 */

#ifndef INCLUSION_GUARD_EMP_H
#define INCLUSION_GUARD_EMP_H

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include "returncodes.h"
#include "swi_dset.h"

/**
 * Agent connection parameters
 */
#define SWI_IPC_SERVER_ADDR "127.0.0.1"
#define SWI_IPC_SERVER_PORT 9999


struct EmpParser_s;

typedef enum
{
  EMP_SENDDATA              = 1,
  EMP_REGISTER              = 2,
  EMP_UNREGISTER            = 3,
  EMP_CONNECTTOSERVER       = 4,
  //SMS
  EMP_REGISTERSMSLISTENER   = 7,
  EMP_NEWSMS                = 8,
  //TREE
  EMP_GETVARIABLE           = 9,
  EMP_SETVARIABLE           = 10,
  EMP_REGISTERVARIABLE      = 11,
  EMP_NOTIFYVARIABLES       = 12,
  EMP_DEREGISTERVARIABLE    = 13,
  //UPDATE
  EMP_SOFTWAREUPDATE        = 20,
  EMP_SOFTWAREUPDATERESULT  = 21,
  EMP_SOFTWAREUPDATESTATUS  = 22,
  EMP_SOFTWAREUPDATEREQUEST = 23,
  EMP_REGISTERUPDATELISTENER = 24,
  EMP_UNREGISTERUPDATELISTENER = 25,

  //UNSTRUCTURED DATA
  EMP_PDATA                 = 30,
  EMP_PFLUSH                = 32,
  EMP_PACKNOWLEDGE          = 33,
  //TABLE
  EMP_TABLENEW              = 40,
  EMP_TABLEROW              = 41,
  EMP_TABLESETMAXROWS       = 43,
  EMP_TABLERESET            = 44,
  EMP_CONSONEW              = 45,
  EMP_CONSOTRIGGER          = 46,
  EMP_SENDTRIGGER           = 47,
  //SYSTEM
  EMP_REBOOT                = 50,
  //SMS
  EMP_UNREGISTERSMSLISTENER = 51,
  EMP_SENDSMS               = 52,

  EMP_NB_OF_COMMANDS

} EmpCommand;

typedef int (*EmpMessageCB)(EmpCommand command,
    uint8_t type, char* payload, uint32_t payloadsize, void *udat);
typedef rc_ReturnCode_t (*EmpSendCB)(const char* payload, uint32_t payloadsize, void *udat);
typedef uint32_t (*EmpReadCB)(char* buffer, uint32_t size, void *udat);


typedef rc_ReturnCode_t (*emp_command_hdl_t)(  uint32_t payloadsize, char* payload );
typedef void (*emp_ipc_broken_hdl_t)(void);

#define EMP_MAX_CMD 64
#define EMP_MAX_IPC_HDLRS 8

typedef struct
{
  uint8_t status;
  sem_t respSem;
  rc_ReturnCode_t respStatus;
  char *respPayload;
  uint32_t respPayloadLen;
} emp_command_ctx_t;

typedef struct EmpParser_s
{
  emp_command_ctx_t commandInProgress[EMP_MAX_CMD];
  emp_command_hdl_t commandHdlrs [EMP_NB_OF_COMMANDS]; //indexed by EmpCommand,
  emp_ipc_broken_hdl_t ipcHdlrs[EMP_MAX_IPC_HDLRS];

  pthread_mutex_t sockLock; // lock used for atomic socket manipulation between the sender and the reader threads

  pthread_t readerThread;

  uint16_t cmdTimeout;
  int sockfd;
  int32_t ridBitfields[2];
} EmpParser;

#define SWI_EMP_INIT_NO_CMDS 0,NULL,NULL,NULL
#define SWI_EMP_DESTROY_NO_CMDS 0,NULL,NULL

rc_ReturnCode_t emp_parser_init(size_t nbCmds, EmpCommand* cmds, emp_command_hdl_t* cmdHdlrs, emp_ipc_broken_hdl_t ipcHdlr);
rc_ReturnCode_t emp_parser_destroy(size_t nbCmds, EmpCommand* cmds, emp_ipc_broken_hdl_t ipcHdlr);
rc_ReturnCode_t emp_send_and_wait_response(EmpCommand command, uint8_t type,
const char* payload, uint32_t payloadsize, char **respPayload, uint32_t* respPayloadLen);
void emp_freemessage(char* buffer);

#endif /* INCLUSION_GUARD_EMP_H */
