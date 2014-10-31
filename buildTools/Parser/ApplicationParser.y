/* Copyright (C) 2013-2014, Sierra Wireless, Inc.  Use of this work is subject to license. */

%{

#include <stdio.h>
#include "ApplicationParser.tab.h"
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
%token  MAX_THREADS_SECTION_LABEL;
%token  MAX_MQUEUE_BYTES_SECTION_LABEL;
%token  MAX_QUEUED_SIGNALS_SECTION_LABEL;
%token  MAX_MEMORY_BYTES_SECTION_LABEL;
%token  CPU_SHARE_SECTION_LABEL;
%token  MAX_FILE_SYSTEM_BYTES_SECTION_LABEL;
%token  COMPONENTS_SECTION_LABEL;
%token  GROUPS_SECTION_LABEL;
%token  EXECUTABLES_SECTION_LABEL;
%token  PROCESSES_SECTION_LABEL;
%token  RUN_SECTION_LABEL;
%token  ENV_VARS_SECTION_LABEL;
%token  PRIORITY_SECTION_LABEL;
%token  MAX_CORE_DUMP_FILE_BYTES_SECTION_LABEL;
%token  MAX_FILE_BYTES_SECTION_LABEL;
%token  MAX_LOCKED_MEMORY_BYTES_SECTION_LABEL;
%token  MAX_FILE_DESCRIPTORS_SECTION_LABEL;
%token  FAULT_ACTION_SECTION_LABEL;
%token  WATCHDOG_ACTION_SECTION_LABEL;
%token  WATCHDOG_TIMEOUT_SECTION_LABEL;
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

%token END 0 "end of file"

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
    | max_threads_section
    | max_mqueue_bytes_section
    | max_queued_signals_section
    | max_memory_bytes_section
    | cpu_share_section
    | max_file_system_bytes_section
    | components_section
    | groups_section
    | executables_section
    | processes_section
    | pools_section
    | watchdog_timeout_subsection
    | watchdog_action_subsection
    | bindings_section
    | requires_section
    | provides_section
    | bundles_section
    ;

version_section
    : VERSION_SECTION_LABEL ver_string    { ayy_SetVersion($2); }
    ;

sandboxed_section
    : SANDBOXED_SECTION_LABEL NAME    { ayy_SetSandboxed($2); }
    ;

start_section
    : START_SECTION_LABEL NAME    { ayy_SetStartMode($2); }
    ;

max_threads_section
    : MAX_THREADS_SECTION_LABEL NUMBER      { ayy_SetMaxThreads(yy_GetNumber($2)); }
    ;

max_mqueue_bytes_section
    : MAX_MQUEUE_BYTES_SECTION_LABEL NUMBER { ayy_SetMaxMQueueBytes(yy_GetNumber($2)); }
    ;

max_queued_signals_section
    : MAX_QUEUED_SIGNALS_SECTION_LABEL NUMBER { ayy_SetMaxQueuedSignals(yy_GetNumber($2)); }
    ;

max_memory_bytes_section
    : MAX_MEMORY_BYTES_SECTION_LABEL NUMBER   { ayy_SetMaxMemoryBytes(yy_GetNumber($2)); }
    ;

cpu_share_section
    : CPU_SHARE_SECTION_LABEL NUMBER   { ayy_SetCpuShare(yy_GetNumber($2)); }
    ;

max_file_system_bytes_section
    : MAX_FILE_SYSTEM_BYTES_SECTION_LABEL NUMBER { ayy_SetMaxFileSystemBytes(yy_GetNumber($2)); }
    ;

groups_section
    : GROUPS_SECTION_LABEL '{' group_list '}'
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
    : COMPONENTS_SECTION_LABEL '{' '}'
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


bundles_section
    : BUNDLES_SECTION_LABEL '{' '}'
    | BUNDLES_SECTION_LABEL '{' bundles_subsection_list '}'
    ;


bundles_subsection_list
    : bundles_subsection
    | bundles_subsection_list bundles_subsection
    ;


bundles_subsection
    : bundled_file_section
    | bundled_dir_section
    ;


bundled_file_section
    : FILE_SECTION_LABEL '{' '}'
    | FILE_SECTION_LABEL '{' bundled_file_list '}'
    ;


bundled_file_list
    : bundled_file
    | bundled_file_list bundled_file
    ;


bundled_file
    : file_path file_path               { ayy_AddBundledFile("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { ayy_AddBundledFile($1, $2, $3); }
    ;


bundled_dir_section
    : DIR_SECTION_LABEL
    | DIR_SECTION_LABEL '{' '}'
    | DIR_SECTION_LABEL '{' bundled_dir_list '}'
    ;


bundled_dir_list
    : bundled_dir
    | bundled_dir bundled_dir_list
    ;


bundled_dir
    : file_path file_path               { ayy_AddBundledDir("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { ayy_AddBundledDir($1, $2, $3); }
    ;


executables_section
    : EXECUTABLES_SECTION_LABEL '{' '}'
    | EXECUTABLES_SECTION_LABEL '{' executables_list '}'
    ;

executables_list
    : exe_spec
    | executables_list exe_spec
    ;

exe_spec
    : exe_name '=' '(' exe_content_list ')'     { ayy_FinalizeExecutable(); }
    ;

exe_name
    : file_path         { ayy_AddExecutable($1); }
    ;

exe_content_list
    : file_path                         { ayy_AddExeContent($1); }
    | exe_content_list file_path        { ayy_AddExeContent($2); }
    ;

processes_section
    : PROCESSES_SECTION_LABEL '{' '}'
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
    | max_core_dump_file_bytes_subsection
    | max_file_bytes_subsection
    | max_locked_memory_bytes_subsection
    | max_file_descriptors_subsection
    | fault_action_subsection
    | watchdog_action_subsection
    | watchdog_timeout_subsection
    ;

run_subsection
    : RUN_SECTION_LABEL '{' '}'
    | RUN_SECTION_LABEL '{' process_list '}'
    ;

process_list
    : process
    | process process_list
    ;

process
    : '(' command_line ')'              { ayy_FinalizeProcess(NULL); }
    | NAME '=' '(' command_line ')'     { ayy_FinalizeProcess($1); }
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
    : ENV_VARS_SECTION_LABEL '{' '}'
    | ENV_VARS_SECTION_LABEL '{' env_var_list '}'
    ;

env_var_list
    : env_var
    | env_var_list env_var
    ;

env_var
    : NAME '=' file_path    { ayy_AddEnvVar($1, $3); }
    ;

priority_subsection
    : PRIORITY_SECTION_LABEL NAME     { ayy_SetPriority($2); }
    ;

max_core_dump_file_bytes_subsection
    : MAX_CORE_DUMP_FILE_BYTES_SECTION_LABEL NUMBER
                                                { ayy_SetMaxCoreDumpFileBytes(yy_GetNumber($2)); }
    ;

max_file_bytes_subsection
    : MAX_FILE_BYTES_SECTION_LABEL NUMBER      { ayy_SetMaxFileBytes(yy_GetNumber($2)); }
    ;

max_locked_memory_bytes_subsection
    : MAX_LOCKED_MEMORY_BYTES_SECTION_LABEL NUMBER
                                                { ayy_SetMaxLockedMemoryBytes(yy_GetNumber($2)); }
    ;

max_file_descriptors_subsection
    : MAX_FILE_DESCRIPTORS_SECTION_LABEL NUMBER { ayy_SetMaxFileDescriptors(yy_GetNumber($2)); }
    ;

fault_action_subsection
    : FAULT_ACTION_SECTION_LABEL NAME     { ayy_SetFaultAction($2); }
    ;

watchdog_action_subsection
    : WATCHDOG_ACTION_SECTION_LABEL NAME  { ayy_SetWatchdogAction($2); }
    ;

requires_section
    : REQUIRES_SECTION_LABEL '{' '}'
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
    : API_SECTION_LABEL '{' '}'
    | API_SECTION_LABEL '{' required_api_list '}'
    ;

required_api_list
    : required_api_spec
    | required_api_list required_api_spec
    ;

required_api_spec
    : file_path             { ayy_AddRequiredApi(NULL, $1); }
    | NAME '=' file_path    { ayy_AddRequiredApi($1, $3); }
    ;

required_file_subsection
    : FILE_SECTION_LABEL '{' '}'
    | FILE_SECTION_LABEL '{' required_file_list '}'
    ;

required_file_list
    : required_file
    | required_file required_file_list
    ;


required_file
    : file_path file_path               { ayy_AddRequiredFile($1, $2); }
    ;


required_dir_subsection
    : DIR_SECTION_LABEL '{' '}'
    | DIR_SECTION_LABEL '{' required_dir_list '}'
    ;

required_dir_list
    : required_dir_mapping
    | required_dir_list required_dir_mapping
    ;

required_dir_mapping
    : file_path file_path               { ayy_AddRequiredDir($1, $2); }
    ;

required_config_tree_subsection
    : CONFIG_TREE_SECTION_LABEL '{' '}'
    | CONFIG_TREE_SECTION_LABEL '{' required_config_tree_list '}'
    ;

required_config_tree_list
    : required_config_tree
    | required_config_tree_list required_config_tree
    ;

required_config_tree
    : NAME                  { ayy_AddConfigTreeAccess("[r]", $1); }
    | PERMISSIONS NAME      { ayy_AddConfigTreeAccess($1, $2); }
    ;

watchdog_timeout_subsection
    : WATCHDOG_TIMEOUT_SECTION_LABEL
    | WATCHDOG_TIMEOUT_SECTION_LABEL NUMBER  { ayy_SetWatchdogTimeout(yy_GetNumber($2)); }
    | WATCHDOG_TIMEOUT_SECTION_LABEL NAME    { ayy_SetWatchdogDisabled($2); }
    ;

file_path
    : FILE_PATH { $$ = $1; }
    | NAME      { $$ = $1; }
    | NUMBER    { $$ = $1; }
    ;

provides_section
    : PROVIDES_SECTION_LABEL '{' '}'
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
    : API_SECTION_LABEL '{' '}'
    | API_SECTION_LABEL '{' provided_api_list '}'
    ;

provided_api_list
    : provided_api_spec
    | provided_api_list provided_api_spec
    ;

provided_api_spec
    : file_path             { ayy_AddProvidedApi(NULL, $1); }
    | NAME '=' file_path    { ayy_AddProvidedApi($1, $3); }
    ;

pools_section
    : POOLS_SECTION_LABEL pool_list
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
    : BINDINGS_SECTION_LABEL '{' '}'
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
    ;


%%
