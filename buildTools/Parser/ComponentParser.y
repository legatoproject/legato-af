%{

#include <stdio.h>
#include "ComponentParser.tab.h"
#include "lex.cyy.h"

%}

%error-verbose

%union
{
    const char* string;
}

%token  SOURCES;
%token  IMPORT;
%token  EXPORT;
%token  FILES;
%token  POOLS;
%token  CONFIG;
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
    | files_section
    | pools_section
    ;


sources_section
    : SOURCES ':' source_file_list
    | SOURCES ':'
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
    : IMPORT ':' import_list
    | IMPORT ':'
    ;

import_list
    : import
    | import_list import
    ;

import
    : NAME '=' file_path        { cyy_AddImportedInterface($1, $3); }
    ;


export_section
    : EXPORT ':' export_list
    | EXPORT ':'
    ;


export_list
    : export
    | export_list export
    ;


export
    : NAME '=' file_path        { cyy_AddExportedInterface($1, $3); }
    ;


files_section
    : FILES ':' included_files_list
    | FILES ':'
    ;


included_files_list
    : included_file
    | included_files_list included_file
    ;


included_file
    : file_path file_path               { cyy_AddIncludedFile("[r]", $1, $2); }
    | PERMISSIONS file_path file_path   { cyy_AddIncludedFile($1, $2, $3); }
    ;


pools_section
    : POOLS ':' pool_list
    | POOLS ':' { printf("Pools.\n"); }
    ;


pool_list
    : pool
    | pool_list pool
    ;


pool
    : NAME '=' NUMBER
    ;


%%

