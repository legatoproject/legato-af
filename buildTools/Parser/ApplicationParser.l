/* Copyright (C) 2013-2014, Sierra Wireless, Inc.  Use of this work is subject to license. */

%option yylineno

name   [A-Za-z_][0-9A-Za-z_]*

env-var             "$"({name}|"{"{name}"}")

file-path-element   ({env-var}|([-0-9A-Za-z:;._+?]+))+

file-path    (\/?{file-path-element}(\/{file-path-element})*\/?)|\/

dotted-string   [0-9A-Za-z_.]*

%x COMMENT
%x IN_SINGLE_QUOTES
%x IN_DOUBLE_QUOTES

%{
//--------------------------------------------------------------------------------------------------

#include "ApplicationParser.tab.h"    // Definitions from the parser.
#include "ApplicationParserInternals.h"

// Count of the number of errors that have been reported during scanning.
size_t ayy_ErrorCount = 0;

// Non-zero if the end of the file has been reached.
int ayy_EndOfFile = 0;

// Name of the file that is currently being parsed.
const char* ayy_FileName = "";


//--------------------------------------------------------------------------------------------------
%}


%%


version[ \t\n]*:            { return VERSION_SECTION_LABEL; }
sandboxed[ \t\n]*:          { return SANDBOXED_SECTION_LABEL; }
start[ \t\n]*:              { return START_SECTION_LABEL; }
numProcs[ \t\n]*:           { return NUM_PROCS_SECTION_LABEL; }
mqueueSize[ \t\n]*:         { return MQUEUE_SIZE_SECTION_LABEL; }
rtSignalQueueSize[ \t\n]*:  { return RT_SIGNAL_QUEUE_SIZE_SECTION_LABEL; }
memLimit[ \t\n]*:           { return MEMORY_LIMIT_SECTION_LABEL; }
cpuShare[ \t\n]*:           { return CPU_SHARE_SECTION_LABEL; }
fileSystemSize[ \t\n]*:     { return FILE_SYSTEM_SIZE_SECTION_LABEL; }
components[ \t\n]*:         { return COMPONENTS_SECTION_LABEL; }
groups[ \t\n]*:             { return GROUPS_SECTION_LABEL; }
files[ \t\n]*:              { return FILES_SECTION_LABEL; }
executables[ \t\n]*:        { return EXECUTABLES_SECTION_LABEL; }
processes[ \t\n]*:          { return PROCESSES_SECTION_LABEL; }
run[ \t\n]*:                { return RUN_SECTION_LABEL; }
envVars[ \t\n]*:            { return ENV_VARS_SECTION_LABEL; }
priority[ \t\n]*:           { return PRIORITY_SECTION_LABEL; }
coreFileSize[ \t\n]*:       { return CORE_FILE_SIZE_SECTION_LABEL; }
maxFileSize[ \t\n]*:        { return MAX_FILE_SIZE_SECTION_LABEL; }
memLockSize[ \t\n]*:        { return MEM_LOCK_SIZE_SECTION_LABEL; }
numFds[ \t\n]*:             { return NUM_FDS_SECTION_LABEL; }
faultAction[ \t\n]*:        { return FAULT_ACTION_SECTION_LABEL; }
watchdogAction[ \t\n]*:     { return WATCHDOG_ACTION_SECTION_LABEL; }
watchdogTimeout[ \t\n]*:    { return WATCHDOG_TIMEOUT_SECTION_LABEL; }
import[ \t\n]*:             { return IMPORT_SECTION_LABEL; }
pools[ \t\n]*:              { return POOLS_SECTION_LABEL; }
bindings[ \t\n]*:           { return BINDINGS_SECTION_LABEL; }
config[ \t\n]*:             { return CONFIG_SECTION_LABEL; }
configTree[ \t\n]*:         { return CONFIG_TREE_SECTION_LABEL; }
requires[ \t\n]*:           { return REQUIRES_SECTION_LABEL; }
provides[ \t\n]*:           { return PROVIDES_SECTION_LABEL; }
bundles[ \t\n]*:            { return BUNDLES_SECTION_LABEL; }
api[ \t\n]*:                { return API_SECTION_LABEL; }
file[ \t\n]*:               { return FILE_SECTION_LABEL; }
dir[ \t\n]*:                { return DIR_SECTION_LABEL; }
->                          { return ARROW; }

"/*"            { BEGIN COMMENT; }
<COMMENT>"*/"   { BEGIN INITIAL; }
<COMMENT>.|\n   {}

[']             { BEGIN IN_SINGLE_QUOTES; }
<IN_SINGLE_QUOTES>([^']|\n)*'        { ayy_lval.string = strndup(yytext, yyleng - 1); BEGIN INITIAL; return FILE_PATH; }

[\"]            { BEGIN IN_DOUBLE_QUOTES; }
<IN_DOUBLE_QUOTES>([^\"]|\n)*\"   { ayy_lval.string = strndup(yytext, yyleng - 1); BEGIN INITIAL; return FILE_PATH; }

"["[rwx]+"]"    { ayy_lval.string = strdup(yytext); return PERMISSIONS; }

{name}          { ayy_lval.string = strdup(yytext); return NAME; }

-?(0x)?[0-9]+K? { ayy_lval.string = strdup(yytext); return NUMBER; }

{file-path}     { ayy_lval.string = strdup(yytext); return FILE_PATH; }

{dotted-string}   { ayy_lval.string = strdup(yytext); return DOTTED_STRING; }

                /* Pass these back to the parser as themselves. */
[=:(){}<>]      { return yytext[0]; }

                /* ignore whitespace */
[ \t]           {}

                /* ignore C++-style comments */
"//".*$         {}

                /* ignore newlines */
\n              {}

                /* Everything else is invalid. */
.               {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Unexpected character '%s'", yytext);
                    ayy_error(msg);
                }

%%
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Error handling function.  Prints an error message to the standard error stream and counts
 * errors.  If the number of errors gets too high, terminates the program.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_error
(
    const char* errorString
)
//--------------------------------------------------------------------------------------------------
{
    fprintf(stderr, "%s:%d: %s\n", ayy_FileName, ayy_lineno, errorString);

    ayy_ErrorCount++;

    if (ayy_ErrorCount > AYY_MAX_ERROR_COUNT)
    {
        fprintf(stderr, "Error limit reached.  Stopping at line %d.\n", ayy_lineno);
        exit(ayy_ErrorCount);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The component parser's "yywrap" function, which tells the lexical scanner what to do when it
 * hits an end-of-file.
 *
 * @return 1 always (meaning stop scanning the input).
 */
//--------------------------------------------------------------------------------------------------
int ayy_wrap(void)
{
    ayy_EndOfFile = 1;

    return 1;
}
