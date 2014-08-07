/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *   http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include "emp.h"
#include "swi_log.h"

/*
 * EMP is the Embedded Micro Protocol, it is used to communicate to the agent
 * through a TCP socket.
 *
 * This module is responsible to communicate to agent by exchanging commands with optional payloads.
 * To do so when the module is initialized, it creates a "reader thread", this one runs in background and waits
 * messages coming from the agent. Commands are sent by calling emp_send_and_wait_response, when the command has been
 * sent, the calling thread is blocked and then awake by the reader thread once the corresponding acknowledgement is received.
 * EMP can also handle command handlers, these callbacks are called when the agent explicity sends a command to an application,
 * typically a notification. The reader thread supports responses associated to a command and real commands coming from the agent.
 *
 *
 * emp_send_and_wait_response:
 *
 * This function sends a command with a specified payload to the agent.
 * Once the command has been sent, the calling thread is blocked on a condition until the corresponding
 * response (with ou without payload) is received. For each new command, EMP associates each caller thread to a request identifier (rid).
 * When a message coming from the agent is received EMP wakes up the thread associated with received rid ,
 * if the awakened thread was waiting for this rid, then it quits the function, otherwise it waits on the condition until a new response is received or until
 * the timer is triggered (for example, if the reply is never received).
 *
 * reader_emp_parse:
 *
 * This function is executed in a separated thread, called the reader thread, and waits for message coming from the agent.
 * When a new message is received, its header is completly read and parsed, the command and the payload are then retrieved. The message
 * is dispatched to the application by calling the function reader_dispatch_message.
 *
 * reader_dispatch_message:
 *
 * This functions is executed in the reader thread and dispatches the message received from the agent to the corresponding component of the running application.
 * This component can be either the caller thread  previously blocked on a condition or a software component registered with a command handler to EMP.
 * This functions parses the flags of the last received message to determine if this one is an explicit command sent from the agent to the application,
 * or just a response to a command coming from the application.
 * When the message is a response to a command sent from the application, the associated blocked thread is awake, then the reader thread continue to process new messages.
 * When the message is an explicit command sent by the agent to the application, the associated callback is invoked in a new thread and then detached.
 */

#define EMP_RID_ERROR 0xff
#define EMP_RID_AVAILABLE 0
#define EMP_RID_ALLOCATED 1
#define EMP_RID_TIMEDOUT 2

static EmpParser *parser;

#define min(a,b) (a) < (b) ? (a) : (b)

static rc_ReturnCode_t reader_dispatch_message(EmpCommand command, uint32_t rid, uint8_t type, char* payload,
    uint32_t payloadsize);
static rc_ReturnCode_t ipc_send(const char* payload, uint32_t payloadsize);
static uint32_t ipc_read(char* buffer, uint32_t size);
static void reader_emp_parse();
static rc_ReturnCode_t emp_sendmessage(EmpCommand command, uint8_t type, uint8_t* rid, const char* payload,
    uint32_t payloadsize);

static rc_ReturnCode_t emp_addCmdHandler(EmpCommand cmd, emp_command_hdl_t h);
static rc_ReturnCode_t emp_removeCmdHandler(EmpCommand cmd);
static struct sockaddr_in agent_addr;

#ifdef __ARMEL__

typedef int (__kuser_cmpxchg_t)(int oldval, int newval, volatile int *ptr);
#define __kuser_cmpxchg (*(__kuser_cmpxchg_t *)0xffff0fc0)
static inline int compare_and_swap(volatile int *ptr, int oldval, int newval)
{
  int ret;
  ret = __kuser_cmpxchg(oldval, newval, ptr);
  return ret == 0 ? oldval : newval;
}

#else
#define compare_and_swap(ptr, oldval, newval) __sync_val_compare_and_swap(ptr, oldval, newval)
#endif

// Allocating atomically a bit fields representing available slots (lockfree)
static uint8_t atomic_rid_lookup()
{
  uint8_t i = 0, j = 0, idx = 0;
  for (i = 0; i < 64; i++)
  {
    idx = (i >= 32);
    j = i % 32;
    if ( ( ( compare_and_swap(&parser->ridBitfields[idx], parser->ridBitfields[idx], parser->ridBitfields[idx] | (1 << j)) >> j ) & 0x1 ) == 0)
      return i;
  }
  return 255;
}

static void freerequestid(uint8_t rid)
{
  uint8_t idx = 0, i = 0;

  idx = (rid >= 32);
  i = rid % 32;
  sem_destroy(&parser->commandInProgress[rid].respSem);
  bzero(parser->commandInProgress + rid, sizeof(emp_command_ctx_t));
  compare_and_swap(&parser->ridBitfields[idx], parser->ridBitfields[idx], parser->ridBitfields[idx] & ~(1 << i));
  SWI_LOG("EMP", DEBUG, "%s: freed rid = %u\n", __FUNCTION__, rid);
}

static rc_ReturnCode_t getrequestid(uint8_t *rid)
{
  *rid = atomic_rid_lookup();
  if (*rid == 255)
    return RC_BUSY;
  parser->commandInProgress[*rid].status = EMP_RID_ALLOCATED;
  sem_init(&parser->commandInProgress[*rid].respSem, 0, 0);
  SWI_LOG("EMP", DEBUG, "%s: new rid = %d\n", __FUNCTION__, *rid);
  return RC_OK;
}

static rc_ReturnCode_t emp_sendmessage(EmpCommand command, uint8_t type, uint8_t* rid, const char* payload, uint32_t payloadsize)
{
  unsigned char header[8];
  rc_ReturnCode_t res;
  uint8_t *buffer;

  if (parser->sockfd == -1)
    return RC_COMMUNICATION_ERROR;

  // if the message is a (new) Command, let's generate a new rid (and  don't use rid parameter)
  if ((type & 1) == 0)
  {
    res = getrequestid(rid);
    if (RC_OK != res)
      return res;
  }
  // else we are sending a response, then the rid to used will be the fct parameter rid.
  SWI_LOG("EMP", DEBUG, "%s: [%d] cmd=%d, type=%d, payloadsize=%u\n", __FUNCTION__, *rid, command, type, payloadsize);

  SWI_LOG("EMP", DEBUG, "%s: [%d] packing header of the message\n", __FUNCTION__, *rid);
  // Command Id
  header[0] = (command >> 8) & 0xff;
  header[1] = command & 0xff;
  // Type: Command or Response ?
  header[2] = type;
  // Request Id
  header[3] = *rid;
  //payloadsize: 4 bytes Big endian coded unsigned integer
  header[4] = (payloadsize >> 24) & 0xff;
  header[5] = (payloadsize >> 16) & 0xff;
  header[6] = (payloadsize >> 8) & 0xff;
  header[7] = payloadsize & 0xff;


  buffer = malloc (8 + payloadsize);
  if (buffer == NULL)
  {
    res = RC_NO_MEMORY;
    goto quit;
  }
  memcpy(buffer, header, 8);
  memcpy(buffer + 8, payload, payloadsize);

  //now send the payload
  SWI_LOG("EMP", DEBUG, "%s: [%d] Sending message\n", __FUNCTION__, *rid);
  res = ipc_send((const char *)buffer, 8+payloadsize);

quit:
  if (res != RC_OK)
    freerequestid(*rid);
  SWI_LOG("EMP", DEBUG, "%s: [%d] exiting with res %d\n", __FUNCTION__, *rid, res);
  free(buffer);
  return res;
}

/*
 * used in reader thread
 */
static void reader_emp_parse()
{
  uint8_t  header[8]; // EMP header buffer (always 8 bytes long)
  uint32_t hcnt = 0; // EMP header received bytes
  uint32_t dcnt = 0; // EMP payload received bytes
  uint32_t dlen = 0;  // EMP payload size
  uint8_t  type = 0; // EMP command type (Command or Response)
  uint8_t  rid = 0; // EMP Request ID
  uint16_t command = 0; // EMP command ( EmpCommand)
  char *data = NULL; // EMP payload buffer (variable size, given by dlen)

  if (parser == NULL)
    pthread_exit(NULL);

  SWI_LOG("EMP", DEBUG, "%s: start\n", __FUNCTION__);

  // Receiving header of the message
  do
  {
    uint32_t size, need;
    need = 8 - hcnt;
    size = ipc_read((char*) header + hcnt, need);
    if (size == 0)
      return;
    hcnt += size;
  } while (hcnt < 8);

  command = ((uint16_t) header[0] << 8) + header[1];
  type = header[2];
  rid  = header[3];
  dlen = (((uint32_t) header[4]) << 24) + (((uint32_t) header[5]) << 16)
    + (((uint32_t) header[6]) << 8) + (uint32_t) header[7];

  SWI_LOG("EMP", DEBUG, "%s: new header! command=[%d], type=[%d], rid=[%d], dlen=[%d]\n",
      __FUNCTION__, command, type, rid, dlen);

  // If a payload is found in the message, allocating buffer
  if (dlen)
  {
    data = malloc(dlen + 8);
    if (data == NULL)
    {
      SWI_LOG("EMP", ERROR, "Failed to alloc data for the received message\n");
      return;
    }
    memcpy(data, header, 8);
    data += 8;

    // Receiving payload
    do
    {
      uint32_t need, size;
      need = dlen - dcnt;
      size = ipc_read((char *) data + dcnt, need);
      if (size == 0)
        return;
      dcnt += size;
    } while (dcnt < dlen);
  }

  // Dispatching message to upper layers
  char *dat = data;
  reader_dispatch_message(command, rid, type, dat, dlen);

  // Clearing data reading state vars
  data = NULL;
  hcnt = dcnt = dlen = rid = 0;

  SWI_LOG("EMP", DEBUG, "%s: exiting !!!\n", __FUNCTION__);
}

void emp_freemessage(char *buffer)
{
  if (buffer)
    free(buffer - 8);
}

static void throw_and_broadcast_err(rc_ReturnCode_t status)
{
  int i;

  for (i = 0; i < EMP_MAX_CMD; i++)
  {
    if (parser->commandInProgress[i].status == EMP_RID_ALLOCATED)
    {
      parser->commandInProgress[i].status = EMP_RID_ERROR;
      parser->commandInProgress[i].respStatus = status;
      sem_post(&parser->commandInProgress[i].respSem);
    }
  }
}

static int ipc_reconnect()
{
  int ret = 0, limit = 0, retry = 0, timeout = 0;
  char *var = NULL;

  var = getenv("SWI_EMP_RETRY_IPC_BROKEN");
  retry = var ? atoi(var) : 10;
  var = getenv("SWI_EMP_TIMEOUT_IPC_BROKEN");
  timeout = var ? atoi(var) : 3;

  while (limit < retry)
  {
    SWI_LOG("EMP", WARNING, "Connection lost, reconnecting to agent, retry #%d\n", limit);

    pthread_mutex_lock(&parser->sockLock);
    SWI_LOG("EMP", DEBUG, "%s: sockLock locked\n", __FUNCTION__, parser->sockfd);

    close(parser->sockfd);

    parser->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    SWI_LOG("EMP", DEBUG, "%s: got new fd %d\n", __FUNCTION__, parser->sockfd);

    ret = connect(parser->sockfd, (struct sockaddr*) &agent_addr, sizeof(agent_addr));
    if (ret == 0)
    {
      pthread_mutex_unlock(&parser->sockLock);
      SWI_LOG("EMP", DEBUG, "%s: success, exiting, sockLock unlocked\n", __FUNCTION__);
      return 0;
    }

    SWI_LOG("EMP", DEBUG, "%s: sockLock unlocked\n", __FUNCTION__);
    pthread_mutex_unlock(&parser->sockLock);
    limit++;
    sleep(timeout);
  }

  SWI_LOG("EMP", ERROR, "Reconnecting to agent has failed, closing socket\n");
  // Signal an error to the blocked thread which is waiting for a reply
  throw_and_broadcast_err(RC_COMMUNICATION_ERROR);
  if (parser->sockfd >= 0)
    close(parser->sockfd);
  parser->sockfd = -1;
  return -1;
}

static void *reconnection_dispatcher(void *arg)
{
  int i;

  SWI_LOG("EMP", DEBUG, "%s: Calling IPC handlers\n", __FUNCTION__);
  for (i = 0; i < EMP_MAX_IPC_HDLRS; i++)
    if (parser->ipcHdlrs[i])
      parser->ipcHdlrs[i]();
  return NULL;
}

/*
 * This function is executed in reader thread
 */
static void* read_routine(void* ud)
{
  while (parser->sockfd >= 0)
  {
    reader_emp_parse();

    if (errno == EPIPE)
    {
      if (ipc_reconnect() == 0)
      {
        // Reconnection handlers might block the reader thread or in the worst case
        // might call emp_send_and_wait_message which causes the thread to deadlock or timeout.
        // So we need to call reconnection handlers in a separated thread.
        pthread_t t;
        int res = pthread_create(&t, NULL, reconnection_dispatcher, NULL);
        if (res){
          SWI_LOG("EMP", ERROR, "Failed to create thread to run reconnection_dispatcher, errno:[%s]\n", strerror(errno));
          //reconnection_dispatcher won't be run, quite unlikely and no much thing to do.
        }
        else{
          pthread_detach(t);
          errno = 0;
        }
      }
    }
  }
  return NULL; // Correctly exit reader thread
}

static void emp_addIpcHandler(emp_ipc_broken_hdl_t handler)
{
  int i;

  for (i = 0; i < EMP_MAX_IPC_HDLRS; i++)
  {
    if (parser->ipcHdlrs[i] == NULL)
    {
      parser->ipcHdlrs[i] = handler;
      break;
    }
  }
}

static void emp_removeIpcHandler(emp_ipc_broken_hdl_t handler)
{
  int i;

  for (i = 0; i < EMP_MAX_IPC_HDLRS; i++)
  {
    if (parser->ipcHdlrs[i] == handler)
    {
      parser->ipcHdlrs[i] = NULL;
      break;
    }
  }
}

rc_ReturnCode_t emp_parser_init(size_t nbCmds, EmpCommand* cmds, emp_command_hdl_t* cmdHdlrs,  emp_ipc_broken_hdl_t ipcHdlr)
{
  SWI_LOG("EMP", DEBUG, "%s: init parser\n", __FUNCTION__);

  if (!parser)
  {
    SWI_LOG("EMP", DEBUG, "%s: No existing parser found, allocating a new one\n", __FUNCTION__);
    parser = calloc(1, sizeof(*parser));
    parser->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (parser->sockfd < 0)
    {
      SWI_LOG("EMP", ERROR, "socket creation failed\n");
      emp_parser_destroy(nbCmds, cmds, ipcHdlr);
      return RC_UNSPECIFIED_ERROR;
    }

    agent_addr.sin_family = AF_INET;

    char *port = getenv("SWI_EMP_SERVER_PORT");
    char *addr = getenv("SWI_EMP_SERVER_ADDR");
    char *timeout = getenv("SWI_EMP_CMD_TIMEOUT");

    agent_addr.sin_port = port ? htons( atoi(port) ) : htons(SWI_IPC_SERVER_PORT);
    agent_addr.sin_addr.s_addr = addr ? inet_addr(addr) : inet_addr(SWI_IPC_SERVER_ADDR);

    SWI_LOG("EMP", DEBUG, "%s: Connecting to agent\n", __FUNCTION__);
    if (connect(parser->sockfd, (struct sockaddr*) &agent_addr, sizeof(agent_addr)) < 0)
    {
      SWI_LOG("EMP", ERROR, "socket connection failed\n");
      emp_parser_destroy(nbCmds, cmds, ipcHdlr);
      return RC_COMMUNICATION_ERROR;
    }

    if(pthread_mutex_init(&parser->sockLock, 0)){
      SWI_LOG("EMP", ERROR, "parser mutex lock creation failed [%s]\n", strerror(errno));
      emp_parser_destroy(nbCmds, cmds, ipcHdlr);
      return RC_UNSPECIFIED_ERROR;
    }
    parser->cmdTimeout = timeout ? atoi(timeout) : 60;

    SWI_LOG("EMP", DEBUG, "%s: Creating reader thread\n", __FUNCTION__);
    if(pthread_create(&parser->readerThread, NULL, read_routine, NULL )){
          SWI_LOG("EMP", ERROR, "reader thread creation failed [%s]\n", strerror(errno));
          emp_parser_destroy(nbCmds, cmds, ipcHdlr);
          return RC_UNSPECIFIED_ERROR;
    }
  }

  int i;
  for (i = 0; i < nbCmds; i++)
    emp_addCmdHandler(cmds[i], cmdHdlrs[i]);
  if (ipcHdlr)
    emp_addIpcHandler(ipcHdlr);
  return RC_OK;
}

/*
 * @return 0 if no more handlers are set, 1 if at least one handler is set
 */
static int check_cmd_handlers()
{
  int i = 0;
  for (; i < EMP_NB_OF_COMMANDS; i++)
  {
    if (parser->commandHdlrs[i])
      return 1;
  }
  return 0;
}

rc_ReturnCode_t emp_parser_destroy(size_t nbCmds, EmpCommand* cmds, emp_ipc_broken_hdl_t ipcHdlr)
{
  SWI_LOG("EMP", DEBUG, "%s: destroying parser\n", __FUNCTION__);
  if (!parser)
    return RC_OK;

  int i;
  for (i = 0; i < nbCmds; i++)
    emp_removeCmdHandler(cmds[i]);

  if (check_cmd_handlers())
    return RC_OK;

  if (ipcHdlr)
    emp_removeIpcHandler(ipcHdlr);

  // Stop the reader thread
  if (parser->sockfd >= 0)
  {
    int fd = parser->sockfd;
    parser->sockfd = -1;
    shutdown(fd, 2);
    close(fd);
  }

  if (parser->readerThread)
    pthread_join(parser->readerThread, NULL);

  free(parser);
  parser = NULL;
  return RC_OK;
}

//note add and remove hdl function can be removed and handler install can be
// done at lib init
// repeat cmds in destroy to clean ?

static rc_ReturnCode_t emp_addCmdHandler(EmpCommand cmd, emp_command_hdl_t h)
{
  if (!parser)
    return RC_NOT_INITIALIZED;
  //todo protect handler data
  parser->commandHdlrs[cmd] = h;
  return RC_OK;
}

static rc_ReturnCode_t emp_removeCmdHandler(EmpCommand cmd)
{
  if (!parser)
    return RC_NOT_INITIALIZED;
  //todo protect handler data
  parser->commandHdlrs[cmd] = NULL;
  return RC_OK;
}

typedef struct thread_cmd_data
{
  EmpCommand command;
  uint32_t payloadsize;
  char* payload;
  uint8_t rid;
} thread_cmd_data_t;

static void * thread_cmd_routine(void* ud)
{

  thread_cmd_data_t* data = (thread_cmd_data_t*) ud;
  char* payload = data->payload;
  uint32_t payloadsize = data->payloadsize;
  EmpCommand command = data->command;
  uint8_t rid = data->rid;
  int16_t cmd_status = 0;

  SWI_LOG("EMP", DEBUG, "%s: [%d] start\n", __FUNCTION__, rid);

  free(data);

  if (!parser->commandHdlrs[command])
  {
    SWI_LOG("EMP", ERROR, "no handler set for %d\n", command);
    pthread_exit(NULL);
  }

  rc_ReturnCode_t res = parser->commandHdlrs[command](payloadsize, payload);
  //command status is transmitted as a big endian signed short
  cmd_status = htons(res);
  emp_sendmessage(command, 1, &rid, (char *)&cmd_status, sizeof(cmd_status));

  SWI_LOG("EMP", DEBUG, "%s: [%d] res = %d\n", __FUNCTION__, rid, res);
  return NULL;
}

/*
 * dispatching incoming messages, both new cmds and response.
 * this function runs in reader thread.
 */
static rc_ReturnCode_t reader_dispatch_message(EmpCommand command, uint32_t rid, uint8_t type, char* payload,
    uint32_t payloadsize)
{

  if (type) // that'is a response
  {
    unsigned char* p;
    rc_ReturnCode_t status;
    //status must always be in response
    if (payloadsize < 2)
    {
      SWI_LOG("EMP", ERROR, "%s: Response for rid[%d], payloadsize = %d, error: payload too small to get error\n",
          __FUNCTION__, rid, payloadsize);
      //setting default status, might not be the perfect value:
      status = RC_UNSPECIFIED_ERROR;
    }
    else
    {
      p = (unsigned char*) payload;
      //command status is transmitted as a big endian signed short
      status = (int16_t) ( p[0] << 8 | p[1] );
    }
    SWI_LOG("EMP", DEBUG, "%s: Response for rid[%d], payloadsize = %d, status=%d\n",
        __FUNCTION__, rid, payloadsize, status);

    if (parser->commandInProgress[rid].status == EMP_RID_ALLOCATED)
    {

      parser->commandInProgress[rid].respStatus = status;

      if (payloadsize > 2)
      {
        SWI_LOG("EMP", DEBUG, "%s: Response payload contains extra data (in addition to status)\n", __FUNCTION__);
        parser->commandInProgress[rid].respPayload = payload + 2;
        parser->commandInProgress[rid].respPayloadLen = payloadsize - 2;
        parser->commandInProgress[rid].respPayload = malloc(parser->commandInProgress[rid].respPayloadLen);
        if (NULL == parser->commandInProgress[rid].respPayload)
        {
          parser->commandInProgress[rid].respStatus = RC_NO_MEMORY;
          //don't return here: at least the cmd sender will get emp status, but not additional data.
        }
        else
        {
          memcpy(parser->commandInProgress[rid].respPayload, payload + 2, parser->commandInProgress[rid].respPayloadLen);
        }
      }

      // Signal the response of a command. This may unblock some thread that are waiting on that response
      SWI_LOG("EMP", DEBUG, "%s: broadcast to sender\n", __FUNCTION__);
      sem_post(&parser->commandInProgress[rid].respSem);
      SWI_LOG("EMP", DEBUG, "%s: broadcast done\n", __FUNCTION__);
    }
    else if (parser->commandInProgress[rid].status == EMP_RID_TIMEDOUT)
    {
      freerequestid(rid);
    }
    else
    {
      SWI_LOG("EMP", DEBUG, "Received an unexpected response: cmd [%d], payload [%s], rid[%d]\n",
          command, payload, rid);
    }
    emp_freemessage(payload);
    return RC_OK;
  }
  else //that's new emp cmd coming from RA
  {
    if (!parser->commandHdlrs[command])
    {
      SWI_LOG("EMP", DEBUG, "no handler set for %d\n", command);
      //todo check status!!
      return RC_NOT_AVAILABLE;
    }
    //spawn new thread
    pthread_t thread;
    thread_cmd_data_t* ud = malloc(sizeof(*ud));
    if (NULL == ud)
      return RC_NO_MEMORY;

    ud->command = command;
    ud->payloadsize = payloadsize;
    ud->payload = payload;
    ud->rid = rid;
    int res = pthread_create(&thread, 0, thread_cmd_routine, (void*) ud);
    if (res){
      SWI_LOG("EMP", ERROR, "Failed to create thread to process incoming command, errno[%s]\n", strerror(errno));
      //clean resources
      emp_freemessage(payload);
      //we could improve error reporting by returning RC error depending on actual errno error
      return RC_UNSPECIFIED_ERROR;
    }

    pthread_detach(thread);
    //it's up to the thread to send the response and free the payload!!

    return RC_OK;
  }
}

static rc_ReturnCode_t ipc_send(const char* payload, uint32_t payloadsize)
{
  int s;
  rc_ReturnCode_t status;

  pthread_mutex_lock(&parser->sockLock);
  s = send(parser->sockfd, payload, payloadsize, MSG_NOSIGNAL);
  if (s < 0)
  {
    if (errno == EPIPE || errno == ECONNRESET)
    {
      status = RC_CLOSED;
      goto quit;
    }
    SWI_LOG("EMP", DEBUG, "%s: fd=%d, errno=%d, error=%s\n", __FUNCTION__, parser->sockfd, errno, strerror(errno));
    status = RC_IO_ERROR;
    goto quit;
  }
  if (s != payloadsize)
  {
    SWI_LOG("EMP", ERROR, "%s: send was partial, sent size=%d, expected payloadsize=%u\n", __FUNCTION__, s, payloadsize);
    status = RC_BAD_FORMAT;
    goto quit;
  }
  status = RC_OK;
quit:
  pthread_mutex_unlock(&parser->sockLock);
  return status;
}

static uint32_t ipc_read(char* buffer, uint32_t size)
{
  int r;

  r = recv(parser->sockfd, buffer, size, MSG_WAITALL);

  if (r <= 0)
  {
    SWI_LOG("EMP", DEBUG, "%s: recv returned %d, fd=%d, errno=%d\n", __FUNCTION__, r, parser->sockfd, errno);

    // Don't treat a real standard error has a disconnection, when the remote host disconnects
    // recv returns a value <=0 and a Success errno. In fact there is no errno codes for that.
    // Considere the following cases :
    // * When the lib is deinited the socket is closed, replaced by -1 and the reader thread is killed, in this case
    //   recv will return <= 0 and parser->sockfd will contain -1
    // * When a standard error will occur on the socket, recv will return a value <= 0 and errno will contain a value
    //   described in man recv, like: EGAIN, EBADF, EFAULT and so on
    // * When a disconnection will occur, recv will return a value <= 0 and errno will contain "0", as there is no standard code for
    //   such a case.
    // FIXME: In some cases, in a multi-threaded context, errno might be equal to ECONNRESET, try to understand why... (that does not make sense
    // as errno is thread-local)
    if (parser->sockfd != -1 && (errno == 0 || errno == ECONNRESET))
    {
      // Signal a broken pipe to the blocked thread which is waiting for a reply
      throw_and_broadcast_err(RC_CLOSED);

      errno = EPIPE;
      return 0;
    }
    // Signal an error to the blocked thread which is waiting for a reply
    throw_and_broadcast_err(RC_IO_ERROR);
    if (parser->sockfd >= 0)
      close(parser->sockfd);
    parser->sockfd = -1;
    return 0;
  }
  return r;
}

static int wait_for_response(uint8_t rid, const struct timespec *abs_timeout)
{
  int ret;
  ret = sem_timedwait(&parser->commandInProgress[rid].respSem, abs_timeout);
  return ret;
}

rc_ReturnCode_t emp_send_and_wait_response(EmpCommand command, uint8_t type, const char* payload, uint32_t payloadsize,
    char **respPayload, uint32_t* respPayloadLen)
{
  rc_ReturnCode_t res;
  int ret;
  uint8_t rid = 0;
  struct timeval  tv;
  struct timespec timeout = {0, 0};

  if (parser == NULL)
    return RC_NOT_INITIALIZED;

  // Construct the message and send it through IPC to the agent
  res = emp_sendmessage(command, type, &rid, payload, payloadsize);
  if (res != RC_OK)
  {
    SWI_LOG("EMP", DEBUG, "%s: emp_sendmessage failed, res %d\n", __FUNCTION__, res);
    return res;
  }
  SWI_LOG("EMP", DEBUG, "%s: rid=%d\n", __FUNCTION__, rid);

  // Wait for a response associated to the current rid and block on a condition until
  // the expected response is received or an error is thrown.
  // If no such response is received, a timeout is triggered and an error is returned
  gettimeofday(&tv, NULL);
  timeout.tv_nsec = tv.tv_usec * 1000;
  timeout.tv_sec = tv.tv_sec + parser->cmdTimeout;


  SWI_LOG("EMP", DEBUG, "%s: [%d] waiting for response, time = %lu\n", __FUNCTION__, rid, timeout.tv_sec);
  // Wait on this condition until the reader thread wakes up the caller thread, when a response is received
  // or until the condition raises a timeout
  ret = wait_for_response(rid, &timeout);
  SWI_LOG("EMP", DEBUG, "%s: [%d] got response, time = %lu\n", __FUNCTION__, rid, timeout.tv_sec);

  if (ret == -1 && errno == ETIMEDOUT)
  {
    SWI_LOG("EMP", ERROR, "%s: [%d] timeout for response expired\n", __FUNCTION__, rid);
    parser->commandInProgress[rid].respPayload = NULL;
    parser->commandInProgress[rid].respStatus = RC_TIMEOUT;
    parser->commandInProgress[rid].status = EMP_RID_TIMEDOUT;
  }

  // Only log unexpected errors, not handled at all by EMP
  if (parser->commandInProgress[rid].status == EMP_RID_ERROR && parser->commandInProgress[rid].respStatus != RC_CLOSED)
    SWI_LOG("EMP", ERROR, "%s: [%d] Unexpected error %d raised by reader thread\n", __FUNCTION__, rid, parser->commandInProgress[rid].respStatus);

  res = parser->commandInProgress[rid].respStatus;
  if (parser->commandInProgress[rid].respPayload)
  {
    if (NULL == respPayload || NULL == respPayloadLen)
      free(parser->commandInProgress[rid].respPayload);
    if (respPayload)
      *respPayload = parser->commandInProgress[rid].respPayload;
    if (respPayloadLen)
      *respPayloadLen = parser->commandInProgress[rid].respPayloadLen;
    SWI_LOG("EMP", DEBUG, "%s: [%d] got respPayload=%.*s , len=%u\n", __FUNCTION__, rid, parser->commandInProgress[rid].respPayloadLen,
        parser->commandInProgress[rid].respPayload, parser->commandInProgress[rid].respPayloadLen);
  }

  if (parser->commandInProgress[rid].status != EMP_RID_TIMEDOUT)
    freerequestid(rid);
  return res;
}
