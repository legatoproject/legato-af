/* Copyright (C) 2014, Sierra Wireless, Inc.  Use of this work is subject to license. */

%option yylineno

name   [A-Za-z_][0-9A-Za-z_]*

env-var             "$"({name}|"{"{name}"}")

/* Note: file-path-element must not start with a '*' because "/*" starts a comment. */
file-path-element   ({env-var}|([-0-9A-Za-z:;._+?]+))({env-var}|([-0-9A-Za-z:;._+?]+)|[*])*

file-path           [*]?(\/?{file-path-element}(\/{file-path-element})*\/?)|\/

%x COMMENT
%x IN_SINGLE_QUOTES
%x IN_DOUBLE_QUOTES

%{
//--------------------------------------------------------------------------------------------------

#include "SystemParser.tab.h"    // Definitions from the parser.
#include "SystemParserInternals.h"

// Count of the number of errors that have been reported during scanning.
size_t syy_ErrorCount = 0;

// Non-zero if the end of the file has been reached.
int syy_EndOfFile = 0;

// Name of the file that is currently being parsed.
const char* syy_FileName = "";


//--------------------------------------------------------------------------------------------------
%}


%%


->                          { return ARROW; }
apps[ \t\n]*:               { return APPS_SECTION_LABEL; }
bindings[ \t\n]*:           { return BINDINGS_SECTION_LABEL; }
maxCoreDumpFileBytes[ \t\n]*:   { return MAX_CORE_DUMP_FILE_BYTES_SECTION_LABEL; }
cpuShare[ \t\n]*:           { return CPU_SHARE_SECTION_LABEL; }
faultAction[ \t\n]*:        { return FAULT_ACTION_SECTION_LABEL; }
groups[ \t\n]*:             { return GROUPS_SECTION_LABEL; }
maxFileSystemBytes[ \t\n]*: { return MAX_FILE_SYSTEM_BYTES_SECTION_LABEL; }
maxFileBytes[ \t\n]*:       { return MAX_FILE_BYTES_SECTION_LABEL; }
maxPriority[ \t\n]*:        { return MAX_PRIORITY_SECTION_LABEL; }
maxMemoryBytes[ \t\n]*:     { return MAX_MEMORY_BYTES_SECTION_LABEL; }
maxLockedMemoryBytes[ \t\n]*:   { return MAX_LOCKED_MEMORY_BYTES_SECTION_LABEL; }
maxMQueueBytes[ \t\n]*:     { return MAX_MQUEUE_BYTES_SECTION_LABEL; }
maxFileDescriptors[ \t\n]*: { return MAX_FILE_DESCRIPTORS_SECTION_LABEL; }
maxThreads[ \t\n]*:         { return MAX_THREADS_SECTION_LABEL; }
maxQueuedSignals[ \t\n]*:   { return MAX_QUEUED_SIGNALS_SECTION_LABEL; }
sandboxed[ \t\n]*:          { return SANDBOXED_SECTION_LABEL; }
start[ \t\n]*:              { return START_SECTION_LABEL; }
version[ \t\n]*:            { return VERSION_SECTION_LABEL; }
watchdogAction[ \t\n]*:     { return WATCHDOG_ACTION_SECTION_LABEL; }
watchdogTimeout[ \t\n]*:    { return WATCHDOG_TIMEOUT_SECTION_LABEL; }

"/*"            { BEGIN COMMENT; }
<COMMENT>"*/"   { BEGIN INITIAL; }
<COMMENT>.|\n   {}

[']             { BEGIN IN_SINGLE_QUOTES; }
<IN_SINGLE_QUOTES>([^']|\n)*'        { syy_lval.string = strndup(yytext, yyleng - 1); BEGIN INITIAL; return FILE_PATH; }

[\"]            { BEGIN IN_DOUBLE_QUOTES; }
<IN_DOUBLE_QUOTES>([^\"]|\n)*\"   { syy_lval.string = strndup(yytext, yyleng - 1); BEGIN INITIAL; return FILE_PATH; }

"["[rwx]+"]"    { syy_lval.string = strdup(yytext); return PERMISSIONS; }

{name}          { syy_lval.string = strdup(yytext); return NAME; }

-?(0x)?[0-9]+K? { syy_lval.string = strdup(yytext); return NUMBER; }

{file-path}     { syy_lval.string = strdup(yytext); return FILE_PATH; }

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
                    syy_error(msg);
                }

%%
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Error handling function.  Prints an error message to the standard error stream and counts
 * errors.  If the number of errors gets too high, terminates the program.
 **/
//--------------------------------------------------------------------------------------------------
void syy_error
(
    const char* errorString
)
//--------------------------------------------------------------------------------------------------
{
    // Make error messages stand out from the clutter when running in verbose mode.
    if (syy_IsVerbose)
    {
        fprintf(stderr, " [-- ERROR --]\n");
    }

    fprintf(stderr, "%s:%d: ERROR: %s\n", syy_FileName, syy_lineno, errorString);

    syy_ErrorCount++;

    if (syy_ErrorCount > SYY_MAX_ERROR_COUNT)
    {
        fprintf(stderr, "Error limit reached.  Stopping at line %d.\n", syy_lineno);
        exit(syy_ErrorCount);
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
int syy_wrap(void)
{
    syy_EndOfFile = 1;

    return 1;
}
