%option yylineno


file-path-element   [-0-9A-Za-z._+?]+

file-path    \/?{file-path-element}(\/{file-path-element})*\/?

name   [A-Za-z_][0-9A-Za-z_]*

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


sandboxed[ \t\n]*:          { return SANDBOXED; }
start[ \t\n]*:              { return START; }
numProcs[ \t\n]*:           { return NUM_PROCS; }
mqueueSize[ \t\n]*:         { return MQUEUE_SIZE; }
rtSignalQueueSize[ \t\n]*:  { return RT_SIGNAL_QUEUE_SIZE; }
memLimit[ \t\n]*:           { return MEMORY_LIMIT; }
cpuShare[ \t\n]*:           { return CPU_SHARE; }
fileSystemSize[ \t\n]*:     { return FILE_SYSTEM_SIZE; }
components[ \t\n]*:         { return COMPONENTS; }
groups[ \t\n]*:             { return GROUPS; }
files[ \t\n]*:              { return FILES; }
executables[ \t\n]*:        { return EXECUTABLES; }
processes[ \t\n]*:          { return PROCESSES; }
run[ \t\n]*:                { return RUN; }
envVars[ \t\n]*:            { return ENV_VARS; }
priority[ \t\n]*:           { return PRIORITY; }
coreFileSize[ \t\n]*:       { return CORE_FILE_SIZE; }
maxFileSize[ \t\n]*:        { return MAX_FILE_SIZE; }
memLockSize[ \t\n]*:        { return MEM_LOCK_SIZE; }
numFds[ \t\n]*:             { return NUM_FDS; }
faultAction[ \t\n]*:        { return FAULT_ACTION; }
watchdogAction[ \t\n]*:     { return WATCHDOG_ACTION; }
import[ \t\n]*:             { return IMPORT; }
pools[ \t\n]*:              { return POOLS; }
bind[ \t\n]*:               { return BIND; }
config[ \t\n]*:             { return CONFIG; }

"/*"            { BEGIN COMMENT; }
<COMMENT>"*/"   { BEGIN INITIAL; }
<COMMENT>.|\n   {}

[']             { BEGIN IN_SINGLE_QUOTES; }
<IN_SINGLE_QUOTES>([^']|\n)*'        { ayy_lval.string = strndup(yytext, yyleng - 1); BEGIN INITIAL; return FILE_PATH; }

[\"]            { BEGIN IN_DOUBLE_QUOTES; }
<IN_DOUBLE_QUOTES>([^\"]|\n)*\"   { ayy_lval.string = strndup(yytext, yyleng - 1); BEGIN INITIAL; return FILE_PATH; }

"["[rwxp]+"]"   { ayy_lval.string = strdup(yytext); return PERMISSIONS; }

{name}          { ayy_lval.string = strdup(yytext); return NAME; }

-?(0x)?[0-9]+K? { ayy_lval.string = strdup(yytext); return NUMBER; }

{file-path}     { ayy_lval.string = strdup(yytext); return FILE_PATH; }

                /* Pass these back to the parser as themselves. */
[=:()]          { return yytext[0]; }

                /* ignore whitespace */
[ \t]           {}

                /* ignore C++-style comments */
"//".*$         {}

                /* ignore newlines */
\n              {}

                /* Everything else is invalid. */
.               { char msg[128]; snprintf(msg, sizeof(msg), "Unexpected character '%s'", yytext); ayy_error(msg); }

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
    fprintf(stderr, "%s: line %d: %s\n", ayy_FileName, ayy_lineno, errorString);

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
