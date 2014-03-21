%option yylineno


file-path-element   [-0-9A-Za-z._+?]+

file-path    \/?{file-path-element}(\/{file-path-element})*\/?

name   [A-Za-z_][0-9A-Za-z_]*

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


sources        { return SOURCES; }
import         { return IMPORT; }
export         { return EXPORT; }
files          { return FILES; }
pools          { return POOLS; }
config         { return CONFIG; }

"/*"            BEGIN COMMENT;
<COMMENT>"*/"   BEGIN INITIAL;
<COMMENT>.|\n   {}

[']             { BEGIN IN_SINGLE_QUOTES; }
<IN_SINGLE_QUOTES>([^']|\n)*'        { cyy_lval.string = strndup(yytext, yyleng - 1); BEGIN INITIAL; return FILE_PATH; }

[\"]            { BEGIN IN_DOUBLE_QUOTES; }
<IN_DOUBLE_QUOTES>([^\"]|\n)*\"   { cyy_lval.string = strndup(yytext, yyleng - 1); BEGIN INITIAL; return FILE_PATH; }

"["[rwxp]+"]"   { cyy_lval.string = strdup(yytext); return PERMISSIONS; }

{name}          { cyy_lval.string = strdup(yytext); return NAME; }

-?(0x)?[0-9]+   { cyy_lval.string = strdup(yytext); return NUMBER; }

{file-path}     { cyy_lval.string = strdup(yytext); return FILE_PATH; }

                /* Pass these back to the parser as themselves. */
[=:]            { return yytext[0]; }

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
    fprintf(stderr, "%s: line %d: %s\n", cyy_FileName, cyy_lineno, errorString);

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
