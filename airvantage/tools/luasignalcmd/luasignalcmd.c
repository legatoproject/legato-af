/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *******************************************************************************/
#include "swi_log.h"
#include "luasignal.h"
#include <stdlib.h>

#define LUASIGTRC 1

/*
 * Simple Lua Signal command to send an event to an Lua VM using Lua Signal.
 *
 * Usage: luasignalcmd Lua_Signal_port_number signal_emitter signal_event [ event_param ]*
 *
 * Signal is sent to atoi(Lua_Signal_port_number), please refer to atoi() behavior.
 * All command params are mandatory except event params.
 * All signal parts (emitter, event, params) are sent as string only, see Lua Signal doc to get more details.
 */
int main(int argc, char** argv)
{
  int port = 0;
  //to use luaSignal API
  static LuaSignalCtx* luaSigCtx = NULL;
  const char* listen_emitters[] = { 0 };
  rc_ReturnCode_t res;
  if (argc < 3)
  {
    SWI_LOG("LUASIGTRC", ERROR, "Param errors: need at least 2 params: EMITTER, EVENT\n");
    return 1;
  }

  port = atoi(argv[1]);
  if(port == 0){
    SWI_LOG("LUASIGTRC", ERROR, "Param error: first param must to be Lua signal port number\n");
    return 1;
  }

  res = LUASIGNAL_Init(&luaSigCtx, port, listen_emitters, NULL);
  if (RC_OK != res)
  {
    SWI_LOG("LUASIGTRC", ERROR, "LUASIGNAL_Init failed with error [%d], exiting\n", res);
    return 1;
  }

  res = LUASIGNAL_SignalT(luaSigCtx, argv[2], argv[3], (const char**)argv+4);
  if (RC_OK != res){
    SWI_LOG("LUASIGTRC", ERROR, "LUASIGNAL_SignalT failed with error [%d]\n", res);
  }

  LUASIGNAL_Destroy(luaSigCtx);
  return 0;
}
