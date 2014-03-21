/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include "yajl_gen.h"
#include "yajl_helpers.h"
#include "testutils.h"
#include "emp.h"

// If building for a x86 compatible target, assuming it's at least a core i5, so 8 threads is sufficient
#if defined (i386) || defined (__i386) || defined (__i386__) || defined (__x86_64) || defined (__x86_64__)
#define EMP_SEND_NB_THREADS 8
#else // otherwises, assuming it's an embedded device with at most 1 core
#define EMP_SEND_NB_THREADS 2
#endif

typedef enum
{
  EMP_TRIGGER_TIMEOUT = 1,
  EMP_SEND_CMD = 2,
  EMP_CALLBACK_CMD = 3,
  EMP_IPC_BROKEN = 4,
  EMP_SIMULATE_CRASH = 7
} EmpTestCommand;

static rc_ReturnCode_t newCallbackCmd(uint32_t payloadsize, char* payload);

static EmpCommand empCmds[] = { EMP_CALLBACK_CMD };
static emp_command_hdl_t empHldrs[] = { newCallbackCmd };
static volatile uint8_t cb_invoked = 0;
static volatile uint8_t reconnected = 0;
static pthread_t threads[EMP_SEND_NB_THREADS];

static rc_ReturnCode_t newCallbackCmd(uint32_t payloadsize, char* payload)
{
  SWI_LOG("EMP_TEST", DEBUG, "%s\n", __FUNCTION__);
  cb_invoked = 1;
  return RC_OK;
}

static void empReconnectionCallback()
{
  SWI_LOG("EMP_TEST", DEBUG, "%s\n", __FUNCTION__);
  reconnected = 1;
}

static rc_ReturnCode_t emp_init()
{
  rc_ReturnCode_t res;

  res = emp_parser_destroy(SWI_EMP_DESTROY_NO_CMDS);
  if (res != RC_OK)
    return res;

  res = emp_parser_init(SWI_EMP_INIT_NO_CMDS);
  if (res != RC_OK)
    return res;

  res = emp_parser_init(SWI_EMP_INIT_NO_CMDS);
  if (res != RC_OK)
    return res;
  return RC_OK;
}

static rc_ReturnCode_t emp_init_with_callbacks()
{
  rc_ReturnCode_t res;

  res = emp_parser_init(1, empCmds, empHldrs, empReconnectionCallback);
  if (res != RC_OK)
    return res;

  res = emp_parser_init(1, empCmds, empHldrs, empReconnectionCallback);
  if (res != RC_OK)
    return res;
  return RC_OK;
}

static rc_ReturnCode_t emp_destroy()
{
  rc_ReturnCode_t res;

  res = emp_parser_destroy(SWI_EMP_DESTROY_NO_CMDS);
  if (res != RC_OK)
    return res;

  res = emp_parser_destroy(SWI_EMP_DESTROY_NO_CMDS);
  if (res != RC_OK)
    return res;
  return RC_OK;
}

static rc_ReturnCode_t emp_trigger_response_timeout()
{
  rc_ReturnCode_t res;

  res = emp_send_and_wait_response(EMP_TRIGGER_TIMEOUT, 0, NULL, 0, NULL, NULL);
  if (res != RC_TIMEOUT)
    return res;
  return RC_OK;
}

static void * send_cmd(void *arg)
{
  uintptr_t id = (uintptr_t)arg, i = 0, fd = -1;
  rc_ReturnCode_t res;
  uint8_t rbufferLen;
  size_t payloadLen = 0;
  uint32_t respPayloadLen = 0;
  char *payload = NULL, *rbuffer = NULL, *respPayload = NULL;
  yajl_gen gen;
  ssize_t ret;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

   // Allocating payload with a random size modulo 256
  fd = open("/dev/urandom", O_RDONLY);
  ret = read(fd, &rbufferLen, sizeof(rbufferLen));

  if (ret != sizeof(rbufferLen))
  {
    SWI_LOG("EMP_TEST", ERROR, "EMP sender thread #%" PRIuPTR " failed:  unable to get random payload size\n", id);
    return NULL;
  }

  if (rbufferLen == 0)
    rbufferLen++;

  rbuffer = malloc(rbufferLen);

  // Generating random payload with displayable characters
  for (i = 0; i < rbufferLen; i++)
  {
    ret = read(fd, rbuffer + i, sizeof(rbuffer[i]));

    if (ret != sizeof(rbuffer[i]))
    {
      SWI_LOG("EMP_TEST", ERROR, "EMP sender thread #%" PRIuPTR " failed:  unable to generate random payload\n", id);
      return NULL;
    }
    rbuffer[i] = rbuffer[i] % 127;
    if (rbuffer[i] < (char)0x33)
      rbuffer[i] = (char)0x33;
  }
  close(fd);

  gen = yajl_gen_alloc(NULL);
  yajl_gen_string(gen, (const unsigned char *)rbuffer, rbufferLen);
  yajl_gen_get_buf(gen, (const unsigned char **)&payload, &payloadLen);

  while (true)
  {
    res = emp_send_and_wait_response(EMP_SEND_CMD, 0, payload, payloadLen, &respPayload, &respPayloadLen);
    if (res != RC_OK && res != RC_CLOSED)
    {
      SWI_LOG("EMP_TEST", ERROR, "EMP sender thread #%" PRIuPTR " failed:  unexpected status code %d\n", id, res);
      exit(1);
    }

    if (payloadLen != respPayloadLen || strncmp(payload, respPayload, payloadLen) != 0)
    {
      SWI_LOG("EMP_TEST", ERROR, "EMP sender thread #%" PRIuPTR " failed: payload mismatched\n"
                             "payload = %.*s, payloadLen = %u, respPayload = %.*s, respPayloadLen = %u\n"
                                 , id, payloadLen, payload, payloadLen, respPayloadLen, respPayload, respPayloadLen);
      exit(1);
    }
  }
  yajl_gen_clear(gen);
  yajl_gen_free(gen);
  free(rbuffer);
  return NULL;
}

static rc_ReturnCode_t emp_start_mt_cmd()
{
  uintptr_t i;

  for (i = 0; i < EMP_SEND_NB_THREADS; i++)
      pthread_create(threads + i, NULL, send_cmd, (void *)i);
  return RC_OK;
}

static rc_ReturnCode_t emp_stop_mt_cmd()
{
  int i;
  for (i = 0; i < EMP_SEND_NB_THREADS; i++)
    pthread_cancel(threads[i]);
  return RC_OK;
}

static rc_ReturnCode_t emp_reconnecting()
{
  // Asking EMP testing server to simulate a crash, in this way EMP
  // can handles reconnecting
  emp_send_and_wait_response(EMP_IPC_BROKEN, 0, NULL, 0, NULL, NULL);

  while (reconnected == 0)
    usleep(1000 * 5); // 5ms
  return RC_OK;
}

static rc_ReturnCode_t emp_fail_reconnecting()
{
  rc_ReturnCode_t res;

  do
  {
    res = emp_send_and_wait_response(EMP_SIMULATE_CRASH, 0, NULL, 0, NULL, NULL);
  } while (res != RC_COMMUNICATION_ERROR);

  return RC_OK;
}

int main(void)
{
  INIT_TEST("EMP_TEST");

  setenv("SWI_EMP_SERVER_PORT", "1234", 1);
  setenv("SWI_EMP_CMD_TIMEOUT", "2", 1);
  setenv("SWI_EMP_RETRY_IPC_BROKEN", "2", 1);
  setenv("SWI_EMP_TIMEOUT_IPC_BROKEN", "2", 1);

  CHECK_TEST(emp_init());
  CHECK_TEST(emp_destroy());
  CHECK_TEST(emp_init_with_callbacks());
  CHECK_TEST(emp_start_mt_cmd());
  CHECK_TEST(emp_trigger_response_timeout());

  while (cb_invoked == 0)
    usleep(1000 * 5); // 5ms

  CHECK_TEST(emp_reconnecting());
  CHECK_TEST(emp_stop_mt_cmd());
  CHECK_TEST(emp_fail_reconnecting());
  CHECK_TEST(emp_destroy());
  return 0;
}
