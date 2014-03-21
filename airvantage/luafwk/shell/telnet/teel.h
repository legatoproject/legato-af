/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Cuero Bugot for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#ifndef TEEL_H_
#define TEEL_H_

#include "mem_define.h"

typedef struct _TeelInstance TeelInstance;

// must return an (unsigned) byte value or -1 when there is nothing to read
// may be blocking until something is read
typedef int (*TeelReadFunc)(void* ud);
// must write the given number of byte
// may return -1 in case of failure
typedef int (*TeelWriteFunc)(void* ud, const char* buf, unsigned int size);

// Autocomplete callback
// A function that set nb and tab pointers accordingly (number of entry and an array of entries)
// *tab must be a memory block allocated with malloc (a free will be called internally), may be set to null if no entries are returned
typedef void (*TeelAutoCompleteFunc)(void* ud, char* tocomplete, int len, int* nb, char*** tab);

typedef enum
{
  CMD_NOP,  // means no operation. Used internally only, teel_run will never return this command
  CMD_EOS,  // the input stream was exhausted, need to call teel_run() function again when new bytes arrive
  CMD_DONE, // a line (editing) is done and available (use xx function to retrieve it)
  CMD_IP,   // Ctrl-C, Interrupt process
  CMD_SUSP, // Ctrl-Z, Suspend
  CMD_EOF,  // Ctrl-D, End of file: user requested to close the shell

} TeelCmd;  // Commands that is returned by teel_run() function.



TeelInstance* teel_initialize(TeelReadFunc reader, TeelWriteFunc writer, void* ud);
int teel_destroy(TeelInstance* ti);

TeelCmd teel_run(TeelInstance* ti);
int teel_getline(TeelInstance* ti, char** line, int* len, unsigned int format);
int teel_showprompt(TeelInstance* ti, const char* prompt, int len);
int teel_outputbeforeline(TeelInstance* ti, const char* buf, int len);
int teel_setdisplaysize(TeelInstance* ti, int width, int height);
int teel_sethistorysize(TeelInstance* ti, int size);
int teel_addhistory(TeelInstance* ti, const char* line, int len);
int teel_setautocompletefunc(TeelInstance* ti, TeelAutoCompleteFunc autocomplete);


#endif /* TEEL_H_ */
