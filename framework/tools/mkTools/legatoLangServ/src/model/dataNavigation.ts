
//--------------------------------------------------------------------------------------------------
/**
 * Code used for locating points of interest within the definition files.
 */
//--------------------------------------------------------------------------------------------------

"use strict";

import * as jdoc from './jsonDocument';
import * as model from './annotatedModel';
import * as parser from './parser';
import * as loader from './loader';



/** Result type for some token processing functions. */
type TokenResult = parser.Token | undefined;



/**
 * Used to return a token result for the end token of a section and the section the cursor is on.
*/
type SectionTokenResult = [ TokenResult, TokenResult ];



//--------------------------------------------------------------------------------------------------
/**
 * A super simple rough approximation of a def file section.
 */
//--------------------------------------------------------------------------------------------------
export interface Section
{
    name: string;
    start: parser.Location;
    end: parser.Location;
    onToken: TokenResult;
}



//--------------------------------------------------------------------------------------------------
/**
 * Is the given position on or after the given token.
 *
 * @param line The source text line.
 * @param column The source text column.
 * @param testLoc The token location we're checking against.
 */
//--------------------------------------------------------------------------------------------------
function isPosAfter(line: number, column: number, testLoc: model.Location): boolean
{
    if (line < testLoc.line)
    {
        return false;
    }

    if (line === testLoc.line)
    {
        if (column < testLoc.column)
        {
            return false;
        }

        return true;
    }

    return true;
}



//--------------------------------------------------------------------------------------------------
/**
 * Is the position on or before this token location?
 *
 * @param line The source text line.
 * @param column The source text column.
 * @param testLoc The token location we're checking.
 */
//--------------------------------------------------------------------------------------------------
function isPosBefore(line: number, column: number, testLoc: model.Location): boolean
{
    if (line > testLoc.line)
    {
        return false;
    }

    if (line === testLoc.line)
    {
        if (column > testLoc.column)
        {
            return false;
        }

        return true;
    }

    return true;
}



//--------------------------------------------------------------------------------------------------
/**
 * Is the given position within the token's text?
 *
 * @param line The line of the source text.
 * @param column The column with in the source text line.
 * @param token The token we're checking.
 */
//--------------------------------------------------------------------------------------------------
function isPositionInToken(line: number, column: number, token: parser.Token): boolean
{
    let endLoc = new loader.Location(token.location.file,
        token.location.line,
        token.location.column + (token.text !== undefined ? token.text.length : 1));

    return    isPosAfter(line, column, token.location)
           && isPosBefore(line, column, endLoc);
}



//--------------------------------------------------------------------------------------------------
/**
 * Find the end token for a given def section.  Also return the token the cursor happens to be on.
 *
 * @param buffer The token buffer to scan.
 */
//--------------------------------------------------------------------------------------------------
function findSectionToken
(
    buffer: parser.TokenBuffer,
    line: number,
    column: number
)
//--------------------------------------------------------------------------------------------------
: SectionTokenResult
//--------------------------------------------------------------------------------------------------
{
    let foundToken: parser.Token = undefined;

    for (let next = buffer.getNext();
         !next.isMatch(loader.defTokens.EndOfFile);
         next = buffer.getNext())
    {
        if (isPositionInToken(line, column, next))
        {
            foundToken = next;
        }

        if (next.isMatch(loader.defTokens.CloseCurly))
        {
            return [ foundToken, next ];
        }
    }

    return [ undefined, undefined ];
}



//--------------------------------------------------------------------------------------------------
/**
 * Check if the given point is on or in between the two given tokens.
 *
 * @param line The position's line number.
 * @param column The positions column in the text.
 * @param begin The start token.
 * @param end The end token.
 */
//--------------------------------------------------------------------------------------------------
function isBetweenTokens
(
    line: number,
    column: number,
    begin: parser.Token,
    end: parser.Token
)
//--------------------------------------------------------------------------------------------------
: boolean
//--------------------------------------------------------------------------------------------------
{
    return isPosAfter(line, column, begin.location) && isPosBefore(line, column, end.location);
}



//--------------------------------------------------------------------------------------------------
/**
 * Attempt to figure out the section type based on a location structure.
 *
 * @param document The original document's token list we are searching through.
 * @param line The line of the file in question.
 * @param column The column of the file we are searching from.
 */
//--------------------------------------------------------------------------------------------------
export function getSectionType
(
    docName: string,
    document: jdoc.Token[],
    line: number,
    column: number,
    isLooseSearch: boolean = false
)
//--------------------------------------------------------------------------------------------------
: Section | undefined
//--------------------------------------------------------------------------------------------------
{
    let buffer = new parser.TokenBuffer(loader.convertTokensFromJson(docName, document),
                                       loader.defTokens.EndOfFile,
                                       [ loader.defTokens.Comment, loader.defTokens.Whitespace,
                                         loader.defTokens.Directive ]);

    let lastSection: Section = undefined;

    for (let next = buffer.getNext();
         !next.isMatch(loader.defTokens.EndOfFile);
         next = buffer.getNext())
    {
        if (next.isMatch(loader.defTokens.Name))
        {
            let nameToken = next;
            let colon = buffer.getNext();

            let openBracket = buffer.getNext();

            if (   (colon.isMatch(loader.defTokens.Colon))
                && (openBracket.isMatch(loader.defTokens.OpenCurly)))
            {
                if (isLooseSearch === false)
                {
                    let [ foundToken, closeBracket ] = findSectionToken(buffer, line, column);

                    if (closeBracket === undefined)
                    {
                        continue;
                    }

                    if (!isBetweenTokens(line, column, openBracket, closeBracket))
                    {
                        continue;
                    }

                    return {
                            name: nameToken.text,
                            start: openBracket.location,
                            end: closeBracket.location,
                            onToken: foundToken
                        };
                }
                else if (isPosAfter(line, column, openBracket.location))
                {
                    lastSection = {
                            name: nameToken.text,
                            start: openBracket.location,
                            end: openBracket.location,
                            onToken: undefined
                        };
                }
            }
        }
    }

    return lastSection;
}



//--------------------------------------------------------------------------------------------------
/**
 * Expand a single environment variable string into it's value.  It can be in one of two forms:
 *
 * $var_name
 * ${var_name}
 *
 * In the second form, extra text may occur after the variable name, which needs to be preserved in
 * the expansion.
 *
 * @param text The variable name text to expand.
 * @param env The environment to find the variable within.
 */
//--------------------------------------------------------------------------------------------------
function processVar(text: string, env: NodeJS.ProcessEnv): string | undefined
{
    if (text[0] === '{')
    {
        let found = text.indexOf('}', 1);

        if (found > -1)
        {
            let name = text.substring(1, found);
            let value = env[name];

            return value + text.substring(found + 1);
        }

        return undefined;
    }

    return env[text];
}



//--------------------------------------------------------------------------------------------------
/**
 * Given a string with variables embedded within it, find each variable from the environment
 * structure and replace it's value within the source string.
 *
 * @param text The original text, with variables.
 * @param env The table to use for the lookup.
 */
//--------------------------------------------------------------------------------------------------
export function expandText(text: string, env: NodeJS.ProcessEnv): string
{
    let split = text.split('$');

    for (let i = 1; i < split.length; i++)
    {
        let lookup: string | undefined = processVar(split[i], env);

        if (lookup !== undefined)
        {
            split[i] = lookup;
        }
        else
        {
            split[i] = '';
        }
    }

    return split.join("");
}
