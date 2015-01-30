/* Copyright (C) Sierra Wireless Inc. Use of this work is subject to license. */

%{

#include <stdio.h>
#include "ComponentParser.tab.h"
#include "lex.cyy.h"
#include "ComponentParserInternals.h"

%}

%error-verbose

%union
{
    const char* string;
    void* objectPtr;
}

%token  SOURCES_SECTION_LABEL;
%token  CFLAGS_SECTION_LABEL;
%token  CXXFLAGS_SECTION_LABEL;
%token  LDFLAGS_SECTION_LABEL;
%token  IMPORT_SECTION_LABEL;
%token  EXPORT_SECTION_LABEL;
%token  REQUIRES_SECTION_LABEL;
%token  PROVIDES_SECTION_LABEL;
%token  BUNDLES_SECTION_LABEL;
%token  API_SECTION_LABEL;
%token  ASYNC_MODIFIER;
%token  TYPES_ONLY_MODIFIER;
%token  MANUAL_START_MODIFIER;
%token  FILE_SECTION_LABEL;
%token  DIR_SECTION_LABEL;
%token  LIB_SECTION_LABEL;
%token  COMPONENT_SECTION_LABEL;
%token  FILES_SECTION_LABEL;
%token  POOLS_SECTION_LABEL;
%token  CONFIG_SECTION_LABEL;
%token  <string> FILE_PATH;
%token  <string> PERMISSIONS;
%token  <string> NAME;
%token  <string> NUMBER;

%token  END 0 "end of file"

%type <string> file_path;

%%  // ---------------------------------------------------------------------------------------------


cdef
    : section
    | cdef section
    ;


section
    : sources_section
    | cflags_section
    | cxxflags_section
    | ldflags_section
    | import_section        { cyy_error("'import:' section no longer supported.  Use 'api:' "
                                        "subsection in 'requires:' section instead."); }
    | export_section        { cyy_error("'export:' section no longer supported.  Use 'api:' "
                                        "subsection in 'provides:' section instead."); }
    | requires_section
    | provides_section
    | files_section         { cyy_error("'files:' section no longer supported.  Use 'file:' "
                                        "subsection in 'bundles:' section instead."); }
    | bundles_section
    | pools_section
    | config_section
    ;


sources_section
    : SOURCES_SECTION_LABEL '{' '}'
    | SOURCES_SECTION_LABEL '{' source_file_list '}'
    ;


source_file_list
    : file_path                     { cyy_AddSourceFile($1); }
    | source_file_list file_path    { cyy_AddSourceFile($2); }
    ;


file_path
    : FILE_PATH { $$ = $1; }
    | NAME      { $$ = $1; }
    | NUMBER    { $$ = $1; }
    ;


cflags_section
    : CFLAGS_SECTION_LABEL '{' '}'
    | CFLAGS_SECTION_LABEL '{' cflags_list '}'
    ;

cflags_list
    : file_path                 { cyy_AddCFlag($1); }
    | file_path cflags_list     { cyy_AddCFlag($1); }
    ;


cxxflags_section
    : CXXFLAGS_SECTION_LABEL '{' '}'
    | CXXFLAGS_SECTION_LABEL '{' cxxflags_list '}'
    ;

cxxflags_list
    : file_path                 { cyy_AddCxxFlag($1); }
    | file_path cxxflags_list   { cyy_AddCxxFlag($1); }
    ;


ldflags_section
    : LDFLAGS_SECTION_LABEL '{' '}'
    | LDFLAGS_SECTION_LABEL '{' ldflags_list '}'
    ;

ldflags_list
    : file_path                 { cyy_AddLdFlag($1); }
    | file_path ldflags_list    { cyy_AddLdFlag($1); }
    ;


import_section
    : IMPORT_SECTION_LABEL import_list
    | IMPORT_SECTION_LABEL
    ;


import_list
    : import
    | import_list import
    ;


import
    : NAME '=' file_path
    | NAME '=' file_path TYPES_ONLY_MODIFIER
    | NAME '=' file_path MANUAL_START_MODIFIER
    | file_path
    | file_path TYPES_ONLY_MODIFIER
    | file_path MANUAL_START_MODIFIER
    ;


export_section
    : EXPORT_SECTION_LABEL export_list
    | EXPORT_SECTION_LABEL
    ;


export_list
    : export
    | export_list export
    ;


export
    : NAME '=' file_path
    | NAME '=' file_path ASYNC_MODIFIER
    | NAME '=' file_path MANUAL_START_MODIFIER
    | NAME '=' file_path ASYNC_MODIFIER MANUAL_START_MODIFIER
    | NAME '=' file_path MANUAL_START_MODIFIER ASYNC_MODIFIER
    | file_path
    | file_path ASYNC_MODIFIER
    | file_path MANUAL_START_MODIFIER
    | file_path ASYNC_MODIFIER MANUAL_START_MODIFIER
    | file_path MANUAL_START_MODIFIER ASYNC_MODIFIER
    ;


files_section
    : FILES_SECTION_LABEL bundled_file_list
    | FILES_SECTION_LABEL
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
    : file_path file_path               { cyy_AddBundledFile("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { cyy_AddBundledFile($1, $2, $3); }
    ;


bundled_dir_section
    : DIR_SECTION_LABEL '{' '}'
    | DIR_SECTION_LABEL '{' bundled_dir_list '}'
    ;


bundled_dir_list
    : bundled_dir
    | bundled_dir_list bundled_dir
    ;


bundled_dir
    : file_path file_path               { cyy_AddBundledDir("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { cyy_AddBundledDir($1, $2, $3); }
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
    : required_api_section
    | required_file_section
    | required_dir_section
    | required_lib_section
    | required_component_section
    ;


required_api_section
    : API_SECTION_LABEL '{' '}'
    | API_SECTION_LABEL '{' required_api_list '}'
    ;


required_api_list
    : required_api
    | required_api_list required_api
    ;


required_api
    : file_path                                 { cyy_AddRequiredApi(NULL, $1); }
    | file_path TYPES_ONLY_MODIFIER             { cyy_AddTypesOnlyRequiredApi(NULL, $1); }
    | file_path MANUAL_START_MODIFIER           { cyy_AddManualStartRequiredApi(NULL, $1); }
    | NAME '=' file_path                        { cyy_AddRequiredApi($1, $3); }
    | NAME '=' file_path TYPES_ONLY_MODIFIER    { cyy_AddTypesOnlyRequiredApi($1, $3); }
    | NAME '=' file_path MANUAL_START_MODIFIER  { cyy_AddManualStartRequiredApi($1, $3); }
    ;


required_file_section
    : FILE_SECTION_LABEL '{' '}'
    | FILE_SECTION_LABEL '{' required_file_list '}'
    ;


required_file_list
    : required_file
    | required_file_list required_file
    ;


required_file
    : file_path file_path               { cyy_AddRequiredFile($1, $2); }
    ;


required_dir_section
    : DIR_SECTION_LABEL '{' '}'
    | DIR_SECTION_LABEL '{' required_dir_list '}'
    ;


required_dir_list
    : required_dir
    | required_dir_list required_dir
    ;


required_dir
    : file_path file_path               { cyy_AddRequiredDir($1, $2); }
    ;


required_lib_section
    : LIB_SECTION_LABEL '{' '}'
    | LIB_SECTION_LABEL '{' required_lib_list '}'
    ;


required_lib_list
    : required_lib
    | required_lib_list required_lib
    ;


required_lib
    : file_path     { cyy_AddRequiredLib($1); }
    ;


required_component_section
    : COMPONENT_SECTION_LABEL '{' '}'
    | COMPONENT_SECTION_LABEL '{' required_component_list '}'
    ;


required_component_list
    : required_component
    | required_component_list required_component
    ;


required_component
    : file_path                 { cyy_AddRequiredComponent($1); }
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
    : provided_api_section
    | provided_file_section
    | provided_dir_section
    ;


provided_api_section
    : API_SECTION_LABEL '{' '}'
    | API_SECTION_LABEL '{' provided_api_list '}'
    ;


provided_api_list
    : provided_api
    | provided_api_list provided_api
    ;


provided_api
    : file_path                         { cyy_AddProvidedApi(NULL, $1); }
    | file_path ASYNC_MODIFIER          { cyy_AddAsyncProvidedApi(NULL, $1); }
    | file_path MANUAL_START_MODIFIER   { cyy_AddManualStartProvidedApi(NULL, $1); }
    | file_path ASYNC_MODIFIER MANUAL_START_MODIFIER
                                                   { cyy_AddManualStartAsyncProvidedApi(NULL, $1); }
    | file_path MANUAL_START_MODIFIER ASYNC_MODIFIER
                                                   { cyy_AddManualStartAsyncProvidedApi(NULL, $1); }
    | NAME '=' file_path                { cyy_AddProvidedApi($1, $3); }
    | NAME '=' file_path ASYNC_MODIFIER { cyy_AddAsyncProvidedApi($1, $3); }
    | NAME '=' file_path MANUAL_START_MODIFIER      { cyy_AddManualStartProvidedApi($1, $3); }
    | NAME '=' file_path ASYNC_MODIFIER MANUAL_START_MODIFIER
                                                    { cyy_AddManualStartAsyncProvidedApi($1, $3); }
    | NAME '=' file_path MANUAL_START_MODIFIER ASYNC_MODIFIER
                                                    { cyy_AddManualStartAsyncProvidedApi($1, $3); }
    ;


provided_file_section
    : FILE_SECTION_LABEL
                    { cyy_error("'file:' subsection not permitted inside 'provides:' section."); }
    ;


provided_dir_section
    : DIR_SECTION_LABEL
                    { cyy_error("'dir:' subsection not permitted inside 'provides:' section."); }
    ;


pools_section
    : POOLS_SECTION_LABEL
    | POOLS_SECTION_LABEL pool_list
    ;


pool_list
    : pool
    | pool_list pool
    ;


pool
    : NAME '=' NUMBER   { cyy_error("'pools:' section not yet implemented."); }
    ;


config_section
    : CONFIG_SECTION_LABEL  { cyy_error("'config:' section not yet implemented."); }
    ;

%%

