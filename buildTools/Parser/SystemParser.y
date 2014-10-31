/* Copyright (C) 2014, Sierra Wireless, Inc.  Use of this work is subject to license. */

%{

#include <stdio.h>
#include "SystemParser.tab.h"
#include "lex.ayy.h"
#include "SystemParserInternals.h"

%}

%error-verbose

%union
{
    const char* string;
    void* objectPtr;
}


%token  APPS_SECTION_LABEL;
%token  BINDINGS_SECTION_LABEL;
%token  MAX_CORE_DUMP_FILE_BYTES_SECTION_LABEL;
%token  CPU_SHARE_SECTION_LABEL;
%token  FAULT_ACTION_SECTION_LABEL;
%token  GROUPS_SECTION_LABEL;
%token  MAX_FILE_SYSTEM_BYTES_SECTION_LABEL;
%token  MAX_FILE_BYTES_SECTION_LABEL;
%token  MAX_PRIORITY_SECTION_LABEL;
%token  MAX_MEMORY_BYTES_SECTION_LABEL;
%token  MAX_LOCKED_MEMORY_BYTES_SECTION_LABEL;
%token  MAX_MQUEUE_BYTES_SECTION_LABEL;
%token  MAX_FILE_DESCRIPTORS_SECTION_LABEL;
%token  MAX_THREADS_SECTION_LABEL;
%token  MAX_QUEUED_SIGNALS_SECTION_LABEL;
%token  SANDBOXED_SECTION_LABEL;
%token  START_SECTION_LABEL;
%token  VERSION_SECTION_LABEL;
%token  WATCHDOG_ACTION_SECTION_LABEL;
%token  WATCHDOG_TIMEOUT_SECTION_LABEL;
%token  ARROW;
%token  <string> FILE_PATH;
%token  <string> PERMISSIONS;
%token  <string> NAME;
%token  <string> BIND_USER;
%token  <string> NUMBER;

%token  END 0 "end of file"

%type <string> file_path;



%%  // ---------------------------------------------------------------------------------------------


sdef
    : section
    | sdef section
    ;

section
    : app_section
    | bindings_section
    | version_section
    ;

app_section
    : APPS_SECTION_LABEL '{' '}'
    | APPS_SECTION_LABEL '{' apps_list '}'
    ;

apps_list
    : app
    | app apps_list
    ;

app
    : adef_path                               { syy_FinalizeApp(); }
    | adef_path '{' '}'                       { syy_FinalizeApp(); }
    | adef_path '{' app_subsection_list '}'   { syy_FinalizeApp(); }
    ;

adef_path
    : file_path     { syy_AddApp($1); }

app_subsection_list
    : app_subsection
    | app_subsection_list app_subsection
    ;

app_subsection
    : max_core_dump_file_bytes_subsection
    | cpu_share_subsection
    | fault_action_subsection
    | groups_section
    | max_file_system_bytes_subsection
    | max_file_bytes_subsection
    | max_memory_bytes_subsection
    | max_locked_memory_bytes_subsection
    | max_mqueue_bytes_subsection
    | max_file_descriptors_subsection
    | max_threads_subsection
    | max_priority_subsection
    | max_queued_signals_subsection
    | sandboxed_subsection
    | start_subsection
    | watchdog_timeout_subsection
    | watchdog_action_subsection
    ;

sandboxed_subsection
    : SANDBOXED_SECTION_LABEL NAME    { syy_SetSandboxed($2); }
    ;

start_subsection
    : START_SECTION_LABEL NAME    { syy_SetStartMode($2); }
    ;

max_threads_subsection
    : MAX_THREADS_SECTION_LABEL NUMBER    { syy_SetMaxThreads(yy_GetNumber($2)); }
    ;

max_mqueue_bytes_subsection
    : MAX_MQUEUE_BYTES_SECTION_LABEL NUMBER    { syy_SetMaxMQueueBytes(yy_GetNumber($2)); }
    ;

max_queued_signals_subsection
    : MAX_QUEUED_SIGNALS_SECTION_LABEL NUMBER { syy_SetMaxQueuedSignals(yy_GetNumber($2)); }
    ;

max_memory_bytes_subsection
    : MAX_MEMORY_BYTES_SECTION_LABEL NUMBER   { syy_SetMaxMemoryBytes(yy_GetNumber($2)); }
    ;

cpu_share_subsection
    : CPU_SHARE_SECTION_LABEL NUMBER   { syy_SetCpuShare(yy_GetNumber($2)); }
    ;

max_file_system_bytes_subsection
    : MAX_FILE_SYSTEM_BYTES_SECTION_LABEL NUMBER  { syy_SetMaxFileSystemBytes(yy_GetNumber($2)); }
    ;

max_priority_subsection
    : MAX_PRIORITY_SECTION_LABEL NAME     { syy_SetMaxPriority($2); }
    ;

max_core_dump_file_bytes_subsection
    : MAX_CORE_DUMP_FILE_BYTES_SECTION_LABEL NUMBER
                                                { syy_SetMaxCoreDumpFileBytes(yy_GetNumber($2)); }
    ;

max_file_bytes_subsection
    : MAX_FILE_BYTES_SECTION_LABEL NUMBER     { syy_SetMaxFileBytes(yy_GetNumber($2)); }
    ;

max_locked_memory_bytes_subsection
    : MAX_LOCKED_MEMORY_BYTES_SECTION_LABEL NUMBER
                                                { syy_SetMaxLockedMemoryBytes(yy_GetNumber($2)); }
    ;

max_file_descriptors_subsection
    : MAX_FILE_DESCRIPTORS_SECTION_LABEL NUMBER { syy_SetMaxFileDescriptors(yy_GetNumber($2)); }
    ;

fault_action_subsection
    : FAULT_ACTION_SECTION_LABEL NAME     { syy_SetFaultAction($2); }
    ;

groups_section
    : GROUPS_SECTION_LABEL group_section_open_curly '}'
    | GROUPS_SECTION_LABEL group_section_open_curly group_list '}'
    ;

group_section_open_curly
    : '{'                   { syy_ClearGroups(); }
    ;

group_list
    : group
    | group_list group
    ;

group
    : FILE_PATH { syy_AddGroup($1); }
    | NAME      { syy_AddGroup($1); }
    ;

version_section
    : VERSION_SECTION_LABEL file_path     { syy_SetVersion($2); }
    ;

watchdog_action_subsection
    : WATCHDOG_ACTION_SECTION_LABEL NAME  { syy_SetWatchdogAction($2); }
    ;

watchdog_timeout_subsection
    : WATCHDOG_TIMEOUT_SECTION_LABEL NUMBER  { syy_SetWatchdogTimeout(yy_GetNumber($2)); }
    | WATCHDOG_TIMEOUT_SECTION_LABEL NAME    { syy_SetWatchdogDisabled($2); }
    ;

bindings_section
    : BINDINGS_SECTION_LABEL '{' '}'
    | BINDINGS_SECTION_LABEL '{' bind_list '}'
    ;

bind_list
    : bind_spec
    | bind_list bind_spec
    ;

bind_spec
    : file_path ARROW file_path                     { syy_AddAppToAppBind($1, $3); }
    | file_path ARROW '<' file_path '>' file_path   { syy_AddAppToUserBind($1, $4, $6); }
    | '<' file_path '>' file_path ARROW file_path   { syy_AddUserToAppBind($2, $4, $6); }
    | '<' file_path '>' file_path ARROW '<' file_path '>' file_path
                                                    { syy_AddUserToUserBind($2, $4, $7, $9); }
    ;

file_path
    : FILE_PATH { $$ = $1; }
    | NAME      { $$ = $1; }
    | NUMBER    { $$ = $1; }
    ;


%%

