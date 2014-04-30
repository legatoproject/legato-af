%{

#include <stdio.h>
#include "ComponentParser.tab.h"
#include "lex.ayy.h"

%}

%error-verbose

%union
{
    const char* string;
}

%token  SANDBOXED;
%token  START;
%token  NUM_PROCS;
%token  MQUEUE_SIZE;
%token  RT_SIGNAL_QUEUE_SIZE;
%token  MEMORY_LIMIT;
%token  CPU_SHARE;
%token  FILE_SYSTEM_SIZE;
%token  COMPONENTS;
%token  GROUPS;
%token  FILES;
%token  EXECUTABLES;
%token  PROCESSES;
%token  RUN;
%token  ENV_VARS;
%token  PRIORITY;
%token  CORE_FILE_SIZE;
%token  MAX_FILE_SIZE;
%token  MEM_LOCK_SIZE;
%token  NUM_FDS;
%token  FAULT_ACTION;
%token  WATCHDOG_ACTION;
%token  IMPORT;
%token  POOLS;
%token  CONFIG;
%token  BIND;
%token  <string> FILE_PATH;
%token  <string> PERMISSIONS;
%token  <string> NAME;
%token  <string> NUMBER;

%type <string> file_path;



%%  // ---------------------------------------------------------------------------------------------


adef
    : section
    | adef section
    ;

section
    : sandboxed_section
    | start_section
    | num_procs_section
    | mqueue_size_section
    | rt_signal_queue_section
    | memory_limit_section
    | cpu_share_section
    | file_system_size_section
    | components_section
    | groups_section
    | files_section
    | executables_section
    | processes_section
    | import_section
    | pools_section
    | bind_section
    ;

sandboxed_section
    : SANDBOXED
    | SANDBOXED NAME    { ayy_SetSandboxed($2); }
    ;

start_section
    : START
    | START NAME    { ayy_SetStartMode($2); }
    ;

num_procs_section
    : NUM_PROCS
    | NUM_PROCS NUMBER      { ayy_SetNumProcsLimit(yy_GetNumber($2)); }
    ;

mqueue_size_section
    : MQUEUE_SIZE
    | MQUEUE_SIZE NUMBER    { ayy_SetMqueueSizeLimit(yy_GetNumber($2)); }
    ;

rt_signal_queue_section
    : RT_SIGNAL_QUEUE_SIZE
    | RT_SIGNAL_QUEUE_SIZE NUMBER   { ayy_SetRTSignalQueueSizeLimit(yy_GetNumber($2)); }
    ;

memory_limit_section
    : MEMORY_LIMIT
    | MEMORY_LIMIT NUMBER   { ayy_SetMemoryLimit(yy_GetNumber($2)); }
    ;

cpu_share_section
    : CPU_SHARE
    | CPU_SHARE NUMBER   { ayy_SetCpuShare(yy_GetNumber($2)); }
    ;

file_system_size_section
    : FILE_SYSTEM_SIZE
    | FILE_SYSTEM_SIZE NUMBER       { ayy_SetFileSystemSizeLimit(yy_GetNumber($2)); }
    ;

groups_section
    : GROUPS
    | GROUPS group_list
    ;

group_list
    : group
    | group_list group
    ;

group
    : FILE_PATH { ayy_AddGroup($1); }
    | NAME      { ayy_AddGroup($1); }
    ;

components_section
    : COMPONENTS
    | COMPONENTS component_list
    ;

component_list
    : component
    | component_list component
    ;

component
    : NAME '=' file_path    { ayy_AddComponent($1, $3); }
    | file_path             { ayy_AddComponent("", $1); }
    ;

files_section
    : FILES file_list
    | FILES
    ;

file_list
    : file_include_mapping
    | file_include_mapping file_list
    ;

file_include_mapping
    : file_path file_path               { ayy_AddFile("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { ayy_AddFile($1, $2, $3); }
    ;

executables_section
    : EXECUTABLES executables_list
    | EXECUTABLES
    ;

executables_list
    : exe_spec
    | executables_list exe_spec
    ;

exe_spec
    : exe_name '(' exe_content_list ')'
    ;

exe_name
    : file_path         { ayy_AddExecutable($1); }
    ;

exe_content_list
    : file_path                         { ayy_AddExeContent($1); }
    | exe_content_list file_path        { ayy_AddExeContent($2); }
    ;

processes_section
    : PROCESSES processes_subsection_list   { ayy_FinishProcessesSection(); }
    | PROCESSES
    ;

processes_subsection_list
    : processes_subsection
    | processes_subsection_list processes_subsection
    ;

processes_subsection
    : run_subsection
    | env_vars_subsection
    | priority_subsection
    | core_file_size_subsection
    | max_file_size_subsection
    | mem_lock_size_subsection
    | num_fds_subsection
    | fault_action_subsection
    | watchdog_action_subsection
    ;

run_subsection
    : RUN NAME '(' command_line ')'     { ayy_FinalizeProcess($2); }
    | RUN '(' command_line ')'          { ayy_FinalizeProcess(NULL); }
    | RUN
    ;

command_line
    : file_path             { ayy_SetProcessExe($1); }
    | file_path args_list   { ayy_SetProcessExe($1); }
    ;

args_list
    : file_path             { ayy_AddProcessArg($1); }
    | args_list file_path   { ayy_AddProcessArg($2); }
    ;

env_vars_subsection
    : ENV_VARS env_var_list
    | ENV_VARS
    ;

env_var_list
    : env_var
    | env_var_list env_var


env_var
    : NAME '=' file_path    { ayy_AddEnvVar($1, $3); }
    ;

priority_subsection
    : PRIORITY
    | PRIORITY NAME     { ayy_SetPriority($2); }
    ;

core_file_size_subsection
    : CORE_FILE_SIZE
    | CORE_FILE_SIZE NUMBER     { ayy_SetCoreFileSizeLimit(yy_GetNumber($2)); }
    ;

max_file_size_subsection
    : MAX_FILE_SIZE
    | MAX_FILE_SIZE NUMBER      { ayy_SetMaxFileSizeLimit(yy_GetNumber($2)); }
    ;

mem_lock_size_subsection
    : MEM_LOCK_SIZE
    | MEM_LOCK_SIZE NUMBER      { ayy_SetMemLockSizeLimit(yy_GetNumber($2)); }
    ;

num_fds_subsection
    : NUM_FDS
    | NUM_FDS NUMBER        { ayy_SetNumFdsLimit(yy_GetNumber($2)); }
    ;

fault_action_subsection
    : FAULT_ACTION
    | FAULT_ACTION NAME     { ayy_SetFaultAction($2); }
    ;

watchdog_action_subsection
    : WATCHDOG_ACTION
    | WATCHDOG_ACTION NAME  { ayy_SetWatchdogAction($2); }
    ;

import_section
    : IMPORT file_import_list
    | IMPORT
    ;

file_import_list
    : file_import_mapping
    | file_import_list file_import_mapping
    ;

file_import_mapping
    : file_path file_path               { ayy_AddFileImport("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { ayy_AddFileImport($1, $2, $3); }
    ;

pools_section
    : POOLS pool_list
    | POOLS
    ;

pool_list
    : pool
    | pool_list pool
    ;

pool
    : NAME '=' NUMBER       { ayy_SetPoolSize($1, yy_GetNumber($3)); }
    ;

bind_section
    : BIND bind_list
    | BIND
    ;

bind_list
    : bind_spec
    | bind_list bind_spec
    ;

bind_spec
    : FILE_PATH '=' FILE_PATH   { ayy_CreateBind($1, $3); }

file_path
    : FILE_PATH { $$ = $1; }
    | NAME      { $$ = $1; }
    | NUMBER    { $$ = $1; }
    ;


%%

