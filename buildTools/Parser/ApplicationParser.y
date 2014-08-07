/* Copyright (C) 2013-2014, Sierra Wireless, Inc.  Use of this work is subject to license. */

%{

#include <stdio.h>
#include "ComponentParser.tab.h"
#include "lex.ayy.h"
#include "ApplicationParserInternals.h"

%}

%error-verbose

%union
{
    const char* string;
    void* objectPtr;
}


%token  VERSION_SECTION_LABEL;
%token  SANDBOXED_SECTION_LABEL;
%token  START_SECTION_LABEL;
%token  NUM_PROCS_SECTION_LABEL;
%token  MQUEUE_SIZE_SECTION_LABEL;
%token  RT_SIGNAL_QUEUE_SIZE_SECTION_LABEL;
%token  MEMORY_LIMIT_SECTION_LABEL;
%token  CPU_SHARE_SECTION_LABEL;
%token  FILE_SYSTEM_SIZE_SECTION_LABEL;
%token  COMPONENTS_SECTION_LABEL;
%token  GROUPS_SECTION_LABEL;
%token  FILES_SECTION_LABEL;
%token  EXECUTABLES_SECTION_LABEL;
%token  PROCESSES_SECTION_LABEL;
%token  RUN_SECTION_LABEL;
%token  ENV_VARS_SECTION_LABEL;
%token  PRIORITY_SECTION_LABEL;
%token  CORE_FILE_SIZE_SECTION_LABEL;
%token  MAX_FILE_SIZE_SECTION_LABEL;
%token  MEM_LOCK_SIZE_SECTION_LABEL;
%token  NUM_FDS_SECTION_LABEL;
%token  FAULT_ACTION_SECTION_LABEL;
%token  WATCHDOG_ACTION_SECTION_LABEL;
%token  WATCHDOG_TIMEOUT_SECTION_LABEL;
%token  IMPORT_SECTION_LABEL;
%token  POOLS_SECTION_LABEL;
%token  CONFIG_SECTION_LABEL;
%token  CONFIG_TREE_SECTION_LABEL;
%token  BINDINGS_SECTION_LABEL;
%token  REQUIRES_SECTION_LABEL;
%token  PROVIDES_SECTION_LABEL;
%token  BUNDLES_SECTION_LABEL;
%token  API_SECTION_LABEL;
%token  FILE_SECTION_LABEL;
%token  DIR_SECTION_LABEL;
%token  ARROW;
%token  <string> FILE_PATH;
%token  <string> PERMISSIONS;
%token  <string> NAME;
%token  <string> BIND_USER;
%token  <string> NUMBER;
%token  <string> DOTTED_STRING;

%type <string> file_path;
%type <string> ver_string;



%%  // ---------------------------------------------------------------------------------------------


adef
    : section
    | adef section
    ;

section
    : version_section
    | sandboxed_section
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
    | watchdog_timeout_subsection
    | watchdog_action_subsection
    | bindings_section
    | requires_section
    | provides_section
    | bundles_section
    ;

version_section
    : VERSION_SECTION_LABEL
    | VERSION_SECTION_LABEL ver_string    { ayy_SetVersion($2); }
    ;

sandboxed_section
    : SANDBOXED_SECTION_LABEL
    | SANDBOXED_SECTION_LABEL NAME    { ayy_SetSandboxed($2); }
    ;

start_section
    : START_SECTION_LABEL
    | START_SECTION_LABEL NAME    { ayy_SetStartMode($2); }
    ;

num_procs_section
    : NUM_PROCS_SECTION_LABEL
    | NUM_PROCS_SECTION_LABEL NUMBER      { ayy_SetNumProcsLimit(yy_GetNumber($2)); }
    ;

mqueue_size_section
    : MQUEUE_SIZE_SECTION_LABEL
    | MQUEUE_SIZE_SECTION_LABEL NUMBER    { ayy_SetMqueueSizeLimit(yy_GetNumber($2)); }
    ;

rt_signal_queue_section
    : RT_SIGNAL_QUEUE_SIZE_SECTION_LABEL
    | RT_SIGNAL_QUEUE_SIZE_SECTION_LABEL NUMBER { ayy_SetRTSignalQueueSizeLimit(yy_GetNumber($2)); }
    ;

memory_limit_section
    : MEMORY_LIMIT_SECTION_LABEL
    | MEMORY_LIMIT_SECTION_LABEL NUMBER   { ayy_SetMemoryLimit(yy_GetNumber($2)); }
    ;

cpu_share_section
    : CPU_SHARE_SECTION_LABEL
    | CPU_SHARE_SECTION_LABEL NUMBER   { ayy_SetCpuShare(yy_GetNumber($2)); }
    ;

file_system_size_section
    : FILE_SYSTEM_SIZE_SECTION_LABEL
    | FILE_SYSTEM_SIZE_SECTION_LABEL NUMBER     { ayy_SetFileSystemSizeLimit(yy_GetNumber($2)); }
    ;

groups_section
    : GROUPS_SECTION_LABEL
    | GROUPS_SECTION_LABEL group_list
    | GROUPS_SECTION_LABEL '{' group_list '}'
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
    : COMPONENTS_SECTION_LABEL
    | COMPONENTS_SECTION_LABEL component_list
    | COMPONENTS_SECTION_LABEL '{' component_list '}'
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
    : FILES_SECTION_LABEL file_list
    | FILES_SECTION_LABEL
    ;

file_list
    : file_include_mapping
    | file_include_mapping file_list
    ;

file_include_mapping
    : file_path file_path               { ayy_AddFile("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { ayy_AddFile($1, $2, $3); }
    ;


bundles_section
    : BUNDLES_SECTION_LABEL
    | BUNDLES_SECTION_LABEL bundles_subsection_list
    ;


bundles_subsection_list
    : bundles_subsection
    | bundles_subsection_list bundles_subsection
    ;


bundles_subsection
    : bundled_file_section
    | bundled_dir_section


bundled_file_section
    : FILE_SECTION_LABEL bundled_file
    ;


bundled_dir_section
    : DIR_SECTION_LABEL bundled_dir
    ;


bundled_file
    : '{' file_path file_path '}'               { ayy_AddBundledFile("[r]", $2, $3); }
    | '{' PERMISSIONS file_path file_path '}'   { ayy_AddBundledFile($2, $3, $4); }
    | file_path file_path               { ayy_AddBundledFile("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { ayy_AddBundledFile($1, $2, $3); }
    ;


bundled_dir
    : '{' file_path file_path '}'       { ayy_AddBundledDir($2, $3); }
    | '{' PERMISSIONS file_path file_path '}'
            {
                ayy_error("Permissions not allowed for bundled directories."
                          "  They are always read-only at run time.");
            }
    | file_path file_path       { ayy_AddBundledDir($1, $2); }
    | PERMISSIONS file_path file_path
            {
                ayy_error("Permissions not allowed for bundled directories."
                          "  They are always read-only at run time.");
            }
    ;


executables_section
    : EXECUTABLES_SECTION_LABEL
    | EXECUTABLES_SECTION_LABEL executables_list
    | EXECUTABLES_SECTION_LABEL '{' executables_list '}'
    ;

executables_list
    : exe_spec
    | executables_list exe_spec
    ;

exe_spec
    : exe_name '(' exe_content_list ')'     { ayy_FinalizeExecutable(); }
    | exe_name '{' exe_content_list '}'     { ayy_FinalizeExecutable(); }
    ;

exe_name
    : file_path         { ayy_AddExecutable($1); }
    ;

exe_content_list
    : file_path                         { ayy_AddExeContent($1); }
    | exe_content_list file_path        { ayy_AddExeContent($2); }
    ;

processes_section
    : PROCESSES_SECTION_LABEL
    | PROCESSES_SECTION_LABEL processes_subsection_list          { ayy_FinishProcessesSection(); }
    | PROCESSES_SECTION_LABEL '{' processes_subsection_list '}'  { ayy_FinishProcessesSection(); }
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
    | watchdog_timeout_subsection
    ;

run_subsection
    : RUN_SECTION_LABEL NAME '{' command_line '}'     { ayy_FinalizeProcess($2); }
    | RUN_SECTION_LABEL '{' command_line '}'          { ayy_FinalizeProcess(NULL); }
    | RUN_SECTION_LABEL NAME '(' command_line ')'     { ayy_FinalizeProcess($2); }
    | RUN_SECTION_LABEL '(' command_line ')'          { ayy_FinalizeProcess(NULL); }
    | RUN_SECTION_LABEL
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
    : ENV_VARS_SECTION_LABEL
    | ENV_VARS_SECTION_LABEL env_var_list
    | ENV_VARS_SECTION_LABEL '{' env_var_list '}'
    ;

env_var_list
    : env_var
    | env_var_list env_var


env_var
    : NAME '=' file_path    { ayy_AddEnvVar($1, $3); }
    ;

priority_subsection
    : PRIORITY_SECTION_LABEL
    | PRIORITY_SECTION_LABEL NAME     { ayy_SetPriority($2); }
    ;

core_file_size_subsection
    : CORE_FILE_SIZE_SECTION_LABEL
    | CORE_FILE_SIZE_SECTION_LABEL NUMBER     { ayy_SetCoreFileSizeLimit(yy_GetNumber($2)); }
    ;

max_file_size_subsection
    : MAX_FILE_SIZE_SECTION_LABEL
    | MAX_FILE_SIZE_SECTION_LABEL NUMBER      { ayy_SetMaxFileSizeLimit(yy_GetNumber($2)); }
    ;

mem_lock_size_subsection
    : MEM_LOCK_SIZE_SECTION_LABEL
    | MEM_LOCK_SIZE_SECTION_LABEL NUMBER      { ayy_SetMemLockSizeLimit(yy_GetNumber($2)); }
    ;

num_fds_subsection
    : NUM_FDS_SECTION_LABEL
    | NUM_FDS_SECTION_LABEL NUMBER        { ayy_SetNumFdsLimit(yy_GetNumber($2)); }
    ;

fault_action_subsection
    : FAULT_ACTION_SECTION_LABEL
    | FAULT_ACTION_SECTION_LABEL NAME     { ayy_SetFaultAction($2); }
    ;

watchdog_action_subsection
    : WATCHDOG_ACTION_SECTION_LABEL
    | WATCHDOG_ACTION_SECTION_LABEL NAME  { ayy_SetWatchdogAction($2); }
    ;

requires_section
    : REQUIRES_SECTION_LABEL
    | REQUIRES_SECTION_LABEL requires_subsection_list
    | REQUIRES_SECTION_LABEL '{' requires_subsection_list '}'
    ;

requires_subsection_list
    : requires_subsection
    | requires_subsection_list requires_subsection
    ;

requires_subsection
    : required_api_subsection
    | required_file_subsection
    | required_dir_subsection
    | required_config_tree_subsection
    ;

required_api_subsection
    : API_SECTION_LABEL required_api_spec
    ;

required_api_spec
    : '(' file_path ')'        { ayy_AddRequiredApi(NULL, $2); }
    | NAME '(' file_path ')'   { ayy_AddRequiredApi($1, $3); }
    | '{' file_path '}'        { ayy_AddRequiredApi(NULL, $2); }
    | NAME '{' file_path '}'   { ayy_AddRequiredApi($1, $3); }
    ;

required_file_subsection
    : FILE_SECTION_LABEL required_file_mapping
    ;

required_dir_subsection
    : DIR_SECTION_LABEL required_dir_mapping
    ;

required_dir_mapping
    : file_path file_path               { ayy_AddRequiredDir("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { ayy_AddRequiredDir($1, $2, $3); }
    ;

required_config_tree_subsection
    : CONFIG_TREE_SECTION_LABEL
    | CONFIG_TREE_SECTION_LABEL '{' NAME '}'                { ayy_AddConfigTreeAccess("[r]", $3); }
    | CONFIG_TREE_SECTION_LABEL '{' PERMISSIONS NAME '}'    { ayy_AddConfigTreeAccess($3, $4); }
    ;

watchdog_timeout_subsection
    : WATCHDOG_TIMEOUT_SECTION_LABEL
    | WATCHDOG_TIMEOUT_SECTION_LABEL NUMBER  { ayy_SetWatchdogTimeout(yy_GetNumber($2)); }
    | WATCHDOG_TIMEOUT_SECTION_LABEL NAME    { ayy_SetWatchdogDisabled($2); }
    ;

import_section
    : IMPORT_SECTION_LABEL
    | IMPORT_SECTION_LABEL import_list
    | IMPORT_SECTION_LABEL '{' import_list '}'
    ;

import_list
    : required_file_mapping
    | import_list required_file_mapping
    ;

required_file_mapping
    : file_path file_path               { ayy_AddRequiredFile("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { ayy_AddRequiredFile($1, $2, $3); }
    ;

file_path
    : FILE_PATH { $$ = $1; }
    | NAME      { $$ = $1; }
    | NUMBER    { $$ = $1; }
    ;

provides_section
    : PROVIDES_SECTION_LABEL
    | PROVIDES_SECTION_LABEL provides_subsection_list
    | PROVIDES_SECTION_LABEL '{' provides_subsection_list '}'
    ;

provides_subsection_list
    : provides_subsection
    | provides_subsection_list provides_subsection
    ;

provides_subsection
    : provided_api_subsection
    ;

provided_api_subsection
    : API_SECTION_LABEL provided_api_spec
    ;

provided_api_spec
    : '(' file_path ')'        { ayy_AddProvidedApi(NULL, $2); }
    | NAME '(' file_path ')'   { ayy_AddProvidedApi($1, $3); }
    | '{' file_path '}'        { ayy_AddProvidedApi(NULL, $2); }
    | NAME '{' file_path '}'   { ayy_AddProvidedApi($1, $3); }
    ;

pools_section
    : POOLS_SECTION_LABEL
    | POOLS_SECTION_LABEL pool_list
    | POOLS_SECTION_LABEL '{' pool_list '}'
    ;

pool_list
    : pool
    | pool_list pool
    ;

pool
    : NAME '=' NUMBER       { ayy_SetPoolSize($1, yy_GetNumber($3)); }
    ;

bindings_section
    : BINDINGS_SECTION_LABEL
    | BINDINGS_SECTION_LABEL bind_list
    | BINDINGS_SECTION_LABEL '{' bind_list '}'
    ;

bind_list
    : bind_spec
    | bind_list bind_spec
    ;

bind_spec
    : file_path ARROW file_path                     { ayy_AddBind($1, $3); }
    | file_path ARROW '<' file_path '>' file_path   { ayy_AddBindOutToUser($1, $4, $6); }
    ;

ver_string
    : file_path      { $$ = $1; }
    | DOTTED_STRING  { $$ = $1; }
    ;


%%

