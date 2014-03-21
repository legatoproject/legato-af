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
/*
 * signal.c
 *
 *  Created on: Aug 21, 2009
 *      Author: cbugot
 */


#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "luasignal.h"

#define MAGIC               0xf56a2fa6
#define LUASIGNAL_ADDRESS   "127.0.0.1"


struct LuaSignalCtx
{
  uint32_t magic;
  int sockfd;
  HookCB hook;
  pthread_t reader;
  char* buf;
};


static inline int send_header(int fd, int size)
{
  int status;
  uint16_t s = htons(size);
  if ((status=send(fd, &s, 2, 0)) != 2)
    return status==0 ? -1 : status;
  return 0;
}


static inline int send_binary_so(int fd, const char* so, int size)
{
  if (!so)
    return 0;

  int status;
  if ((status=send_header(fd, size)) != 0)
    return status;
  if ((status=send(fd, so, size, 0)) != size)
    return status==0 ? -1 : status;

  return 0;
}

static inline int send_text_so(int fd, const char* so)
{
  if (!so)
    return 0;
  return send_binary_so(fd, so, strlen(so));
}



static inline int send_text_frame(int fd, const char* so0, const char* so1, const char* so[])
{
  int size = 0;
  int status;

  if (so0)
    size += strlen(so0)+2;
  if (so1)
    size += strlen(so1)+2;

  int i;
  if (so)
  {
    i = 0;
    while (so[i])
      size += strlen(so[i++])+2;
  }

  if ((status=send_header(fd, size)) != 0)
    return status;

  if ((status=send_text_so(fd, so0)) != 0)
    return status;

  if ((status=send_text_so(fd, so1)) != 0)
    return status;

  if (so)
  {
    i = 0;
    while (so[i])
      if ((status=send_text_so(fd, so[i++])) != 0)
        return status;
  }

  return 0;
}

static inline int send_binary_frame(int fd, const char* so0, const char* so1, const char* so[], const uint16_t sol[])
{
  int size = 0;
  int status;

  if (so0)
    size += strlen(so0)+2;
  if (so1)
    size += strlen(so1)+2;

  int i;
  if (so && sol)
  {
    i = 0;
    while (so[i])
      size += sol[i++]+2;
  }

  if ((status=send_header(fd, size)) != 0)
    return status;

  if ((status=send_text_so(fd, so0)) != 0)
    return status;

  if ((status=send_text_so(fd, so1)) != 0)
    return status;

  if (so && sol)
  {
    i = 0;
    while (so[i])
    {
      if ((status=send_binary_so(fd, so[i], sol[i])) != 0)
        return status;
      i++;
    }

  }

  return 0;
}

static inline int read_header(int fd, int* size)
{
  int status;
  uint16_t s;

  if ((status=recv(fd, &s, 2, 0)) != 2)
    return status==0 ? -1 : status;

  *size = (int) ntohs(s);

  return 0;
}

static inline int read_so(int fd, char** buf, int* bufsize, uint16_t* length)
{
  int status;
  int s;
  char* b = *buf;

  if ((status=read_header(fd, &s)) != 0)
    return status;

  if ((status=recv(fd, b, s, 0)) != s)
    return status;

  b[s] = 0; // null char terminator;
  b += s+1;

  *buf = b;
  *bufsize = *bufsize - s -2;
  if (length)
    *length = s;
  return 0;
}

static void* reader_routine(void* c)
{
  LuaSignalCtx* ctx = (LuaSignalCtx*)c;

  int fd = ctx->sockfd;

  int max_nb_of_args = 1;
  char** args;
  uint16_t* args_length;

  args = malloc(sizeof(args[0])*max_nb_of_args);
  args_length = malloc(sizeof(args_length[0])*max_nb_of_args);
  assert(args && args_length);

  while (ctx->magic == MAGIC)
  {
    char* buf;
    int bufsize;
    int status;

    if ((status=read_header(fd, &bufsize)) != 0)
      break;

    buf = malloc(bufsize);
    if (!buf)
      break;
    char* b = buf;

    char* emitter = b;
    if ((status=read_so(fd, &b, &bufsize, 0)) != 0)
    {
      free(buf);
      break;
    }
    char* event = b;
    if ((status=read_so(fd, &b, &bufsize, 0)) != 0)
    {
      free(buf);
      break;
    }

    int i = 0;
    while (bufsize)
    {
      args[i] = b;
      if ((status=read_so(fd, &b, &bufsize, &args_length[i])) != 0)
      {
        free(buf);
        break;
      }
      i++;
      if (i==max_nb_of_args)
      {
        max_nb_of_args *= 2;
        args = realloc((void*)args, sizeof(args[0])*max_nb_of_args);
        args_length = realloc(args_length, sizeof(args_length[0])*max_nb_of_args);
        assert(args && args_length);
      }
    }
    args[i] = 0;

    ctx->hook(emitter, event, (const char**)args, args_length);

    free(buf);

  }
  free(args);
  free(args_length);

  return 0;
}

rc_ReturnCode_t LUASIGNAL_Init(LuaSignalCtx** c, int port, const char* emitters[], HookCB hook)
{
  // This is an error to give a hook but no emitters to receive from !
  if (!c || (hook && (!emitters || !emitters[0])))
    return RC_BAD_PARAMETER;

  struct sockaddr_in serv_addr;
  socklen_t addrlen;
  LuaSignalCtx* ctx;
  *c = 0;

  ctx = malloc(sizeof(*ctx));
  if (!ctx)
    return RC_NO_MEMORY;

  ctx->magic = MAGIC;
  ctx->hook = hook;
  ctx->sockfd = -1;
  ctx->reader = 0;

  ctx->sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (ctx->sockfd < 0)
  {
    LUASIGNAL_Destroy(ctx);
    return RC_UNSPECIFIED_ERROR;
  }

  // Configure the socket to connect to the Agent
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(LUASIGNAL_ADDRESS);
  serv_addr.sin_port = htons(port);
  addrlen = sizeof(struct sockaddr_in);

  // Connect to the agent
  if (connect(ctx->sockfd, (const struct sockaddr*) &serv_addr, addrlen) != 0)
  {
    LUASIGNAL_Destroy(ctx);
    return RC_UNSPECIFIED_ERROR;
  }

  // Send the emitters names to receive signals from
  int status;
  status = send_text_frame(ctx->sockfd, 0, 0, emitters);
  if (status != 0)
  {
    LUASIGNAL_Destroy(ctx);
    return RC_IO_ERROR;
  }

  // Only create a thread if there is a hook ! (the user cannot give a hook with no emitters to receive from)
  if (hook)
  {
    // Create the reader thread
    if (pthread_create(&ctx->reader, 0, reader_routine, (void*)ctx) != 0)
    {
      LUASIGNAL_Destroy(ctx);
      return RC_UNSPECIFIED_ERROR;
    }
  }

  *c = ctx;
  return RC_OK;
}


rc_ReturnCode_t LUASIGNAL_SignalT(LuaSignalCtx* ctx, const char* emitter, const char* event, const char* args[])
{
  if (ctx->magic != MAGIC)
    return RC_BAD_PARAMETER;

  int status;
  status = send_text_frame(ctx->sockfd, emitter, event, args);
  if (status != 0)
    return RC_IO_ERROR;

  return RC_OK;
}

rc_ReturnCode_t LUASIGNAL_SignalB(LuaSignalCtx* ctx, const char* emitter, const char* event, const char* args[], const uint16_t args_length[])
{
  if (ctx->magic != MAGIC)
    return RC_BAD_PARAMETER;

  int status;
  status = send_binary_frame(ctx->sockfd, emitter, event, args, args_length);
  if (status != 0)
    return RC_IO_ERROR;

  return RC_OK;
}

rc_ReturnCode_t LUASIGNAL_Destroy(LuaSignalCtx* ctx)
{
  if (!ctx || ctx->magic != MAGIC)
    return RC_BAD_PARAMETER;

  ctx->magic = ~MAGIC;

  if (ctx->sockfd >= 0)
    shutdown(ctx->sockfd, SHUT_RDWR);

  if (ctx->reader)
    pthread_join(ctx->reader, 0);

  free(ctx);
  return RC_OK;
}
