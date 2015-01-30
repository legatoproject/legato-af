/* Copyright (C) Sierra Wireless Inc. Use of this work is subject to license. */

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

#include "ComponentParser.tab.h"    // Definitions from the parser.
#include "ComponentParserInternals.h"

// Count of the number of errors that have been reported during scanning.
size_t cyy_ErrorCount = 0;

// Non-zero if the end of the file has been reached.
int cyy_EndOfFile = 0;

// Name of the file that is currently being parsed.
const char* cyy_FileName = "";


//--------------------------------------------------------------------------------------------------
%}


%%


sources[ \t\n]*:        { return SOURCES_SECTION_LABEL; }
cflags[ \t\n]*:         { return CFLAGS_SECTION_LABEL; }
cxxflags[ \t\n]*:       { return CXXFLAGS_SECTION_LABEL; }
ldflags[ \t\n]*:        { return LDFLAGS_SECTION_LABEL; }
import[ \t\n]*:         { return IMPORT_SECTION_LABEL; }
export[ \t\n]*:         { return EXPORT_SECTION_LABEL; }
requires[ \t\n]*:       { return REQUIRES_SECTION_LABEL; }
provides[ \t\n]*:       { return PROVIDES_SECTION_LABEL; }
bundles[ \t\n]*:        { return BUNDLES_SECTION_LABEL; }
api[ \t\n]*:            { return API_SECTION_LABEL; }
"[async]"               { return ASYNC_MODIFIER; }
"[types-only]"          { return TYPES_ONLY_MODIFIER; }
"[manual-start]"        { return MANUAL_START_MODIFIER; }
file[ \t\n]*:           { return FILE_SECTION_LABEL; }
dir[ \t\n]*:            { return DIR_SECTION_LABEL; }
lib[ \t\n]*:            { return LIB_SECTION_LABEL; }
component[ \t\n]*:      { return COMPONENT_SECTION_LABEL; }
files[ \t\n]*:          { return FILES_SECTION_LABEL; }
pools[ \t\n]*:          { return POOLS_SECTION_LABEL; }
config[ \t\n]*:         { return CONFIG_SECTION_LABEL; }

"/*"            BEGIN COMMENT;
<COMMENT>"*/"   BEGIN INITIAL;
<COMMENT>.|\n   {}

[']             { BEGIN IN_SINGLE_QUOTES; }
<IN_SINGLE_QUOTES>([^']|\n)*'        { cyy_lval.string = strndup(yytext, yyleng - 1); BEGIN INITIAL; return FILE_PATH; }

[\"]            { BEGIN IN_DOUBLE_QUOTES; }
<IN_DOUBLE_QUOTES>([^\"]|\n)*\"   { cyy_lval.string = strndup(yytext, yyleng - 1); BEGIN INITIAL; return FILE_PATH; }

"["[rwx]+"]"    { cyy_lval.string = strdup(yytext); return PERMISSIONS; }

{name}          { cyy_lval.string = strdup(yytext); return NAME; }

-?(0x)?[0-9]+   { cyy_lval.string = strdup(yytext); return NUMBER; }

{file-path}     { cyy_lval.string = strdup(yytext); return FILE_PATH; }

                /* Pass these back to the parser as themselves. */
[=:{}()]        { return yytext[0]; }

                /* ignore whitespace */
[ \t]           ;

                /* ignore C++-style comments */
"//".*          ;

                /* ignore newlines */
\n              ;

                /* Everything else is invalid. */
.               cyy_error("Unexpected character");


%%
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Error handling function.  Prints an error message to the standard error stream and counts
 * errors.  If the number of errors gets too high, terminates the program.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_error
(
    const char* errorString
)
//--------------------------------------------------------------------------------------------------
{
    // Make error messages stand out from the clutter when running in verbose mode.
    if (cyy_IsVerbose)
    {
        fprintf(stderr, " [-- ERROR --]\n");
    }

    fprintf(stderr, "%s:%d: ERROR: %s\n", cyy_FileName, cyy_lineno, errorString);

    cyy_ErrorCount++;

    if (cyy_ErrorCount > CYY_MAX_ERROR_COUNT)
    {
        fprintf(stderr, "Error limit reached.  Stopping at line %d.\n", cyy_lineno);
        exit(cyy_ErrorCount);
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
int cyy_wrap(void)
{
    cyy_EndOfFile = 1;

    return 1;
}
