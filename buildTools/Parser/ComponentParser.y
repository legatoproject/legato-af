/* Copyright (c) 2013-2014, Sierra Wireless Inc. Use of this work is subject to license. */

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

%type <string> file_path;

%%  // ---------------------------------------------------------------------------------------------


cdef
    : section
    | cdef section
    ;


section
    : sources_section
    | import_section
    | export_section
    | requires_section
    | provides_section
    | files_section
    | bundles_section
    | pools_section
    | config_section
    ;


sources_section
    : SOURCES_SECTION_LABEL source_file_list
    | SOURCES_SECTION_LABEL
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


import_section
    : IMPORT_SECTION_LABEL import_list
    | IMPORT_SECTION_LABEL
    ;


import_list
    : import
    | import_list import
    ;


import
    : NAME '=' file_path                        { cyy_AddRequiredApi($1, $3); }
    | NAME '=' file_path TYPES_ONLY_MODIFIER    { cyy_AddTypesOnlyRequiredApi($1, $3); }
    | NAME '=' file_path MANUAL_START_MODIFIER  { cyy_AddManualStartRequiredApi($1, $3); }
    | file_path                                 { cyy_AddRequiredApi(NULL, $1); }
    | file_path TYPES_ONLY_MODIFIER             { cyy_AddTypesOnlyRequiredApi(NULL, $1); }
    | file_path MANUAL_START_MODIFIER           { cyy_AddManualStartRequiredApi(NULL, $1); }
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
    : NAME '=' file_path                { cyy_AddProvidedApi($1, $3); }

    | NAME '=' file_path ASYNC_MODIFIER { cyy_AddAsyncProvidedApi($1, $3); }

    | NAME '=' file_path MANUAL_START_MODIFIER
                                        { cyy_AddManualStartProvidedApi($1, $3); }

    | NAME '=' file_path ASYNC_MODIFIER MANUAL_START_MODIFIER
                                        { cyy_AddManualStartAsyncProvidedApi($1, $3); }

    | NAME '=' file_path MANUAL_START_MODIFIER ASYNC_MODIFIER
                                        { cyy_AddManualStartAsyncProvidedApi($1, $3); }
    | file_path                         { cyy_AddProvidedApi(NULL, $1); }

    | file_path ASYNC_MODIFIER          { cyy_AddAsyncProvidedApi(NULL, $1); }

    | file_path MANUAL_START_MODIFIER   { cyy_AddManualStartProvidedApi(NULL, $1); }

    | file_path ASYNC_MODIFIER MANUAL_START_MODIFIER
                                        { cyy_AddManualStartAsyncProvidedApi(NULL, $1); }

    | file_path MANUAL_START_MODIFIER ASYNC_MODIFIER
                                        { cyy_AddManualStartAsyncProvidedApi(NULL, $1); }
    ;


files_section
    : FILES_SECTION_LABEL bundled_files_list
    | FILES_SECTION_LABEL
    ;


bundled_files_list
    : bundled_file
    | bundled_files_list bundled_file
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
    : file_path file_path               { cyy_AddBundledFile("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { cyy_AddBundledFile($1, $2, $3); }
    ;


bundled_dir
    : file_path file_path               { cyy_AddBundledDir($1, $2); }
    | PERMISSIONS file_path file_path
            {
                cyy_error("Permissions not allowed for bundled directories."
                          "  They are always read-only at run time.");
            }
    ;


requires_section
    : REQUIRES_SECTION_LABEL
    | REQUIRES_SECTION_LABEL requires_subsection_list
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
    : API_SECTION_LABEL '{' file_path '}'               { cyy_AddRequiredApi(NULL, $3); }

    | API_SECTION_LABEL '{' file_path TYPES_ONLY_MODIFIER '}'
                                                        { cyy_AddTypesOnlyRequiredApi(NULL, $3); }
    | API_SECTION_LABEL NAME '{' file_path '}'          { cyy_AddRequiredApi($2, $4); }

    | API_SECTION_LABEL NAME '{' file_path TYPES_ONLY_MODIFIER '}'
                                                        { cyy_AddTypesOnlyRequiredApi($2, $4); }
    ;


required_file_section
    : FILE_SECTION_LABEL file_path file_path               { cyy_AddRequiredFile("[r]", $2, $3); }
    | FILE_SECTION_LABEL PERMISSIONS file_path file_path   { cyy_AddRequiredFile($2, $3, $4); }
    ;


required_dir_section
    : DIR_SECTION_LABEL file_path file_path               { cyy_AddRequiredDir("[r]", $2, $3); }
    | DIR_SECTION_LABEL PERMISSIONS file_path file_path   { cyy_AddRequiredDir($2, $3, $4); }
    ;


required_lib_section
    : LIB_SECTION_LABEL '{' file_path '}'       { cyy_AddRequiredLib($3); }
    | LIB_SECTION_LABEL file_path               { cyy_AddRequiredLib($2); }
    ;


required_component_section
    : COMPONENT_SECTION_LABEL '{' file_path '}'         { cyy_AddRequiredComponent($3); }
    | COMPONENT_SECTION_LABEL file_path                 { cyy_AddRequiredComponent($2); }
    ;


provides_section
    : PROVIDES_SECTION_LABEL
    | PROVIDES_SECTION_LABEL provides_subsection_list
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
    : API_SECTION_LABEL '{' file_path '}'           { cyy_AddProvidedApi(NULL, $3); }

    | API_SECTION_LABEL '{' file_path ASYNC_MODIFIER '}'
                                                    { cyy_AddAsyncProvidedApi(NULL, $3); }

    | API_SECTION_LABEL '{' file_path MANUAL_START_MODIFIER '}'
                                                    { cyy_AddManualStartProvidedApi(NULL, $3); }

    | API_SECTION_LABEL '{' file_path ASYNC_MODIFIER MANUAL_START_MODIFIER '}'
                                                    { cyy_AddManualStartAsyncProvidedApi(NULL, $3); }

    | API_SECTION_LABEL '{' file_path MANUAL_START_MODIFIER ASYNC_MODIFIER '}'
                                                    { cyy_AddManualStartAsyncProvidedApi(NULL, $3); }

    | API_SECTION_LABEL NAME '{' file_path '}'      { cyy_AddProvidedApi($2, $4); }

    | API_SECTION_LABEL NAME '{' file_path ASYNC_MODIFIER '}'
                                                    { cyy_AddAsyncProvidedApi($2, $4); }

    | API_SECTION_LABEL NAME '{' file_path MANUAL_START_MODIFIER '}'
                                                    { cyy_AddManualStartProvidedApi($2, $4); }

    | API_SECTION_LABEL NAME '{' file_path ASYNC_MODIFIER MANUAL_START_MODIFIER '}'
                                                    { cyy_AddManualStartAsyncProvidedApi($2, $4); }

    | API_SECTION_LABEL NAME '{' file_path MANUAL_START_MODIFIER ASYNC_MODIFIER '}'
                                                    { cyy_AddManualStartAsyncProvidedApi($2, $4); }
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
    : POOLS_SECTION_LABEL pool_list
    | POOLS_SECTION_LABEL { printf("Pools.\n"); }
    ;


pool_list
    : pool
    | pool_list pool
    ;


pool
    : NAME '=' NUMBER
    ;


config_section
    : CONFIG_SECTION_LABEL
    ;

%%

