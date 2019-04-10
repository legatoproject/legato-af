
//--------------------------------------------------------------------------------------------------
/**
 * .
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

'use strict';



//--------------------------------------------------------------------------------------------------
/**
 * Represent a location in the original source file.  Users of the parser are expected to extend
 * this class to better represent the language being parsed.
 */
//--------------------------------------------------------------------------------------------------
export abstract class Location
{
    readonly file: string;
    readonly line: number;
    readonly column: number;

    constructor(file: string, line: number, column: number)
    {
        this.file = file;
        this.line = line;
        this.column = column;
    }

    abstract toString(): string;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export abstract class Token
{
    readonly location?: Location;
    readonly text?: string;

    constructor(location?: Location, text?: string)
    {
        if (location != undefined) { this.location = location; }
        if (text !+ undefined)     { this.text = text;         }
    }

    abstract toString(): string;
    abstract isMatch(nextToken: Token): boolean;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export abstract class DataToken extends Token
{
    abstract convert(text: string): any;
}



//--------------------------------------------------------------------------------------------------
/**
 * Buffer used by the matchers to treat the list of tokens like a data stream.
 *
 * Our parsers allow for infinite lookahead.  We also allow for multiple levels of lookahead.
 * That is, an expression can try matching against multiple sub-expressions.
 *
 * Simply set a lookahead point, and try out your sub-expression.  If it doesn't pan out, cancel
 * the lookahead and go ahead and try other expressions.  The system allows this to be nested so
 * that as expressions can be chained, they all can initiate a mark and attempt to match.
 *
 * Any missed matches are easily rolled back for another attempt with a different matcher.
 */
//--------------------------------------------------------------------------------------------------
export class TokenBuffer
{
    /** The list of tokens to parse. */
    private tokens: Token[];

    /** The token that represents the end of the road. */
    private eofToken: Token;

    /** These tokens are to be ignored in the output. */
    private ignoreTokens: Token[];

    /** Our position in the token stream. */
    private position: number;

    /** A stack to keep track of the various lookahead markers the matchers may set. */
    private lookaheadStack: number[];

    /** Construct a new token buffer, ready for parsing. */
    constructor(tokens: Token[], eofToken: Token, ignoreTokens?: Token[])
    {
        this.tokens = tokens;
        this.eofToken = eofToken;
        this.ignoreTokens = ignoreTokens;
        this.reset();
    }

    /**
     * Reset the token buffer back to it's initial position, ready to start parsing again from
     * scratch.
     */
    reset(): void
    {
        this.position = 0;
        this.lookaheadStack = [];
    }

    /** Mark a lookahead location we can return to if need be. */
    markLookahead(): void
    {
        let lookahead = this.isPeeking ? this.lookaheadStack[0] : this.position;
        this.lookaheadStack.unshift(lookahead);
    }

    /**
     * Call this when we have finished looking ahead in the token stream and we want to want to
     * properly consume the tokens we previously peeked at.
     */
    commitLookahead(): void
    {
        if (!this.isPeeking)
        {
            return;
        }

        let lookahead = this.lookaheadStack.shift();

        if (this.isPeeking)
        {
            this.lookaheadStack[0] = lookahead;
        }
        else
        {
            this.position = lookahead;
        }
    }

    /**
     * Cancel the lookahead and make sure all peeked tokens are still available for other matchers.
     */
    clearLookahead(): void
    {
        if (this.isPeeking)
        {
            this.lookaheadStack.shift();
        }
    }

    /**
     * Read the next token in the stream.  If we are in lookahead mode, this operation is
     * cancelable.
     */
    getNext(): Token
    {
        let next: Token;

        do
        {
            let index: number;

            if (!this.isPeeking)
            {
                index = this.position;
                this.position += 1;
            }
            else
            {
                index = this.lookaheadStack[0];
                this.lookaheadStack[0] += 1;
            }

            if (index >= this.tokens.length)
            {
                next = this.eofToken;
            }
            else
            {
                next = this.tokens[index];
            }
        }
        while (this.isIgnored(next))

        return next;
    }

    /** Are we currently in lookahead mode? */
    get isPeeking(): boolean
    {
        return this.lookaheadStack.length != 0;
    }

    /** Is the given token the eof token? */
    isEof(token: Token): boolean
    {
        return this.eofToken.isMatch(token);
    }

    /** Check to see if the given token should be ignored or not. */
    private isIgnored(token: Token): boolean
    {
        for (let ignored of this.ignoreTokens)
        {
            if (token.isMatch(ignored))
            {
                return true;
            }
        }

        return false;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export type  MatchPullResult =
    {
        isMatch: boolean,
        data?:
        {
            location: Location,
            value?: any
        }
    };



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export abstract class Matcher
{
    abstract pull(buffer: TokenBuffer): MatchPullResult;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export type MatchHandler = (location: Location, data: any[]) => any;



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
type ArgMatcherType = Matcher | Token | DataToken;



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
type ExpressionArgType = ArgMatcherType | MatchHandler;



//--------------------------------------------------------------------------------------------------
/**
 * Create a matcher that expects a specific
 */
//--------------------------------------------------------------------------------------------------
export function expectToken(token: Token): Matcher
{
    class TokenMatcher extends Matcher
    {
        expectedToken: Token;

        constructor(expectedToken: Token)
        {
            super();
            this.expectedToken = expectedToken;
        }

        pull(buffer: TokenBuffer): MatchPullResult
        {
            let nextToken = buffer.getNext();

            if (   (nextToken !== undefined)
                && (nextToken.isMatch(this.expectedToken)))
            {
                return { isMatch: true, data: { location: nextToken.location } };
            }

            return { isMatch: false };
        }
    }

    return new TokenMatcher(token);
}



//--------------------------------------------------------------------------------------------------
/**
 * Create a matcher that expects a specific token and data.  The data token passed in needs to be
 * able to convert from a string to the expected data type.
 */
//--------------------------------------------------------------------------------------------------
export function expectData(token: DataToken): Matcher
{
    class DataMatcher extends Matcher
    {
        expectedToken: DataToken;

        constructor(newToken: DataToken)
        {
            super();
            this.expectedToken = newToken;
        }

        pull(buffer: TokenBuffer): MatchPullResult
        {
            let nextToken = buffer.getNext();

            if (   (nextToken !== undefined)
                && (nextToken.isMatch(this.expectedToken)))
            {
                return {
                    isMatch: true,

                    data:
                    {
                        location: nextToken.location,
                        value: this.expectedToken.convert(nextToken.text)
                    }
                };
            }

            return { isMatch: false };
        }
    }

    return new DataMatcher(token);
}



//--------------------------------------------------------------------------------------------------
/**
 * The matcher produced by this function expects the parsed token to have a specific token type and
 * a specific value.  Again, like expectData, we use the DataToken to convert from a string to
 * the token's true value for comparison against the expected value.
 */
//--------------------------------------------------------------------------------------------------
export function expectValue(token: DataToken, value: any): Matcher
{
    class ValueMatcher extends Matcher
    {
        expectedToken: DataToken;
        expectedValue: any;

        constructor(newToken: DataToken, newValue: any)
        {
            super();
            this.expectedToken = newToken;
            this.expectedValue = newValue;
        }

        pull(buffer: TokenBuffer): MatchPullResult
        {
            let nextToken = buffer.getNext();

            if (nextToken !== undefined)
            {
                if (   (nextToken.isMatch(this.expectedToken))
                    && (this.expectedToken.convert(nextToken.text) === this.expectedValue))
                {
                    return {
                        isMatch: true,
                        data: { location: nextToken.location }
                    };
                }

                return { isMatch: false, data: { location: nextToken.location } };
            }

            return { isMatch: false };
        }
    }

    return new ValueMatcher(token, value);
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
abstract class MultiMatcher extends Matcher
{
    matchers: Matcher[];
    handlers: MatchHandler[];

    constructor(...args: ExpressionArgType[])
    {
        super();

        this.matchers = [];
        this.handlers = [];

        for (let arg of args)
        {
            let result = this.processMatcherArg(arg);

            if (result.matcher !== undefined)
            {
                this.matchers.push(result.matcher);
            }
            else if (result.handler !== undefined)
            {
                this.handlers.push(result.handler);
            }
        }
    }

    private processMatcherArg(arg: ExpressionArgType): { matcher?: Matcher, handler?: MatchHandler }
    {
        if (arg instanceof Matcher)
        {
            return { matcher: arg };
        }
        else if (arg instanceof DataToken)
        {
            return { matcher: expectData(arg) };
        }
        else if (arg instanceof Token)
        {
            return { matcher: expectToken(arg) };
        }

        return { handler: arg };
    }

    /**
     * Called to execute the handler list for a matcher.  If there are no handlers, the input data
     * is returned unaltered.
     *
     * If there is one handler registered, then this function will return whatever the handler will
     * return.  If there are more handlers then a list of processed values are returned.
     */
    protected processHandlers(location: Location, value: any): any
    {
        switch (this.handlers.length)
        {
            case 0:
                return value;

            case 1:
                return this.handlers[0](location, value);

            default:
                let dataList: any[] = [];

                for (let handler of this.handlers)
                {
                    dataList.push(handler(location, value));
                }

                if (dataList.length == 1)
                {
                    return dataList[0];
                }

                return dataList;
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Take a list of expression sub-components and turn them all into a single matcher that expects to
 * be able to match against all of the sub-expression.
 *
 * This function is used to compose more complex expressions to parse.
 */
//--------------------------------------------------------------------------------------------------
export function expectExpression(...args: ExpressionArgType[]): Matcher
{
    class ExpectExpressionMatcher extends MultiMatcher
    {
        public pull(buffer: TokenBuffer): MatchPullResult
        {
            let capturedData: any = [];
            let location: Location = undefined;
            let wasMatched = false;

            for (let matcher of this.matchers)
            {
                let result = matcher.pull(buffer);

                if (!result.isMatch)
                {
                    break;
                }

                if  (result.data !== undefined)
                {
                    if (location === undefined)
                    {
                        location = result.data.location;
                    }

                    if (result.data.value !== undefined)
                    {
                        capturedData.push(result.data.value);
                    }
                }

                wasMatched = true;
            }

            if (capturedData.length == 1)
            {
                capturedData = capturedData[0];
            }

            return {
                isMatch: wasMatched,
                data:
                {
                    location: location,
                    value: wasMatched == true ? this.processHandlers(location, capturedData)
                                              : undefined
                }
            };
        }
    }

    return new ExpectExpressionMatcher(...args);
}



//--------------------------------------------------------------------------------------------------
/**
 * An extension to expectExpression.  This function's matcher works the same with the exception that
 * if the match is unsuccessful the match attempt is backed out allowing other matchers to try.
 */
//--------------------------------------------------------------------------------------------------
export function optionalExpression(...args: ExpressionArgType[]): Matcher
{
    class OptionalExpressionMatcher extends Matcher
    {
        /** The sub-matcher that does the actual work. */
        private expression: Matcher;

        public constructor(...args: ExpressionArgType[])
        {
            super();

            // We don't handle the matching ourselves.  Instead we get expectExpression to handle it
            // for us.
            this.expression = expectExpression(...args);
        }

        public pull(buffer: TokenBuffer): MatchPullResult
        {
            // Mark the buffer in case we need to rewind this match.  Then attempt to match the
            // expression and check the results.
            buffer.markLookahead();
            let result = this.expression.pull(buffer);

            if (result.isMatch)
            {
                // We have a match.
                buffer.commitLookahead();
                return result;
            }

            // This expression is optional, so we have a successful match, but with no data.
            buffer.clearLookahead();
            return { isMatch: true };
        }
    }

    return new OptionalExpressionMatcher(...args);
}



//--------------------------------------------------------------------------------------------------
/**
 * Given a list of expression components, this function will create a matcher that will attempt to
 * match one of them.
 *
 * Matches are handled in order, so if two expressions can succeed on a given input, the first
 * matcher to succeed will be the one that is used.
 */
//--------------------------------------------------------------------------------------------------
export function expectOneOf
(
    ...args: ExpressionArgType[]
)
: Matcher
//--------------------------------------------------------------------------------------------------
{
    class ExpectOneOfMatcher  extends MultiMatcher
    {
        pull(buffer: TokenBuffer): MatchPullResult
        {
            for (let matcher of this.matchers)
            {
                buffer.markLookahead();

                let result = matcher.pull(buffer);

                if (result.isMatch)
                {
                    let location: Location = undefined;
                    let value: any = undefined;

                    buffer.commitLookahead();

                    if (result.data !== undefined)
                    {
                        location = result.data.location;

                        if (result.data.value !== undefined)
                        {
                            value = result.data.value;
                        }
                    }

                    return {
                        isMatch: true,
                        data:
                        {
                            location: location,
                            value: this.processHandlers(location, value)
                        }
                    };
                }

                buffer.clearLookahead();
            }

            return { isMatch: false };
        }
    }

    return new ExpectOneOfMatcher(...args);
}



//--------------------------------------------------------------------------------------------------
/**
 * Given an expression, attempt to match as many of it as possible.  The extracted data is returned
 * as a list.
 */
//--------------------------------------------------------------------------------------------------
export function expectMultipleOf
(
    /** If specified the matcher will look for this expression between the previous expressions. */
    arg: ArgMatcherType,

    /** The expression to attempt to match in the input stream. */
    delimiter?: Token,


    /** The action to upon a successful match. */
    action?: BlockMatchHandler
)
: Matcher
//--------------------------------------------------------------------------------------------------
{
    class ExpectMultipleOfMatcher  extends MultiMatcher
    {
        private delimiter?: Matcher;
        private action: BlockMatchHandler;

        constructor(arg: ArgMatcherType, delimiter?: Token, action?: BlockMatchHandler)
        {
            super(arg);

            if (delimiter !== undefined)
            {
                this.delimiter = expectToken(delimiter);
            }

            if (action === undefined)
            {
                this.action = (_location: BlockMatchLocation, data: any[]): any =>
                    {
                        return data;
                    };
            }
            else
            {
                this.action = action;
            }
        }

        pull(buffer: TokenBuffer): MatchPullResult
        {
            let result: MatchPullResult;

            let isFirst = true;
            let data: any[] = [];
            let location: BlockMatchLocation = { startLocation: undefined, endLocation: undefined };

            while ((result = this.matchItem(isFirst, buffer)).isMatch)
            {
                if (result.data !== undefined)
                {
                    if (   (location.startLocation === undefined)
                        && (isFirst === true))
                    {
                        location.startLocation = result.data.location;
                    }
                    else
                    {
                        location.endLocation = result.data.location;
                    }

                    if (result.data.value !== undefined)
                    {
                        data.push(result.data.value);
                    }
                }

                isFirst = false;
            }

            return {
                    isMatch: true,
                    data: data.length === 0 ? undefined : {
                            location: location.startLocation,
                            value: this.action(location, data)
                        }
                };
        }

        private matchItem(isFirst: boolean, buffer: TokenBuffer): MatchPullResult
        {
            let matcher = this.matchers[0];

            buffer.markLookahead();

            if (   (isFirst !== true)
                && (this.delimiter !== undefined))
            {
                if (this.delimiter.pull(buffer).isMatch === false)
                {
                    buffer.clearLookahead();
                    return { isMatch: false };
                }
            }

            let result = matcher.pull(buffer);

            if (result.isMatch === false)
            {
                buffer.clearLookahead();
            }
            else
            {
                buffer.commitLookahead();
            }

            return result;
        }
    }

    return new ExpectMultipleOfMatcher(arg, delimiter, action);
}



//--------------------------------------------------------------------------------------------------
/**
 * Some matchers expect a specific at the beginning and at the end of a given block.  This structure
 * is used to define the delimiters.  Either as tokens or as matchers.
 */
//--------------------------------------------------------------------------------------------------
interface BlockDelimiters<DelimiterType>
{
    /** Expect the block to start with this delimiter. */
    begin: DelimiterType;

    /** The block needs to end with this delimiter. */
    end: DelimiterType;
}


//--------------------------------------------------------------------------------------------------
/**
 * We are looking for these specific tokens to mark the beginning and ending of an expression
 * block.
 */
//--------------------------------------------------------------------------------------------------
export type BlockDelimiterTokens = BlockDelimiters<Token>;



//--------------------------------------------------------------------------------------------------
/**
 * An internal structure used to store the expression matchers used at the beginning and ending of
 * an expression block.
 */
//--------------------------------------------------------------------------------------------------
type BlockDelimiterMatchers = BlockDelimiters<Matcher>;



//--------------------------------------------------------------------------------------------------
/**
 * A location that represents the beginning and ending locations of the block.
 */
//--------------------------------------------------------------------------------------------------
export interface BlockMatchLocation
{
    startLocation: Location;
    endLocation: Location;
}



//--------------------------------------------------------------------------------------------------
/**
 * We use a slightly different matcher for blocks.  This way we can track the textual area that the
 * block actually takes up.
 */
//--------------------------------------------------------------------------------------------------
export type BlockMatchHandler = (location: BlockMatchLocation, data: any[]) => any



//--------------------------------------------------------------------------------------------------
/**
 * Expect a list of sub-expressions in a block with a defined beginning and ending.
 */
//--------------------------------------------------------------------------------------------------
export function expectBlockListOf
(
    /** If specified the matcher will look for this expression between the previous expressions. */
    arg: ArgMatcherType,

    /**
     * If specified, the matcher will expect to find them at the beginning and the ending of the
     * list of items.
     * */
    blockDelimiterTokens: BlockDelimiterTokens,

    /** The expression to attempt to match in the input stream. */
    delimiter?: Token,

    /** The action to upon a successful match. */
    action?: BlockMatchHandler
)
: Matcher
//--------------------------------------------------------------------------------------------------
{
    class ExpectBlockListMatcher extends Matcher
    {
        private itemMatcher: Matcher;
        private action: BlockMatchHandler;
        private blockDelimiters: BlockDelimiterMatchers;

        constructor(arg: ArgMatcherType, blockDelimiterTokens: BlockDelimiterTokens,
                    delimiter?: Token, action?: BlockMatchHandler)
        {
            super();
            this.itemMatcher = expectMultipleOf(arg, delimiter);

            if (action === undefined)
            {
                this.action = (_location: BlockMatchLocation, data: any[]): any =>
                    {
                        return data;
                    };
            }
            else
            {
                this.action = action;
            }

            this.blockDelimiters =
                {
                    begin: expectToken(blockDelimiterTokens.begin),
                    end:   expectToken(blockDelimiterTokens.end )
                };
        }

        pull(buffer: TokenBuffer): MatchPullResult
        {
            let result: MatchPullResult;
            let location: BlockMatchLocation = { startLocation: undefined, endLocation: undefined };
            let dataStart: Location;

            if ((result = this.blockDelimiters.begin.pull(buffer)).isMatch === false)
            {
                return result;
            }

            location.startLocation = result.data.location;

            result = this.itemMatcher.pull(buffer);
            dataStart = result.data !== undefined ? result.data.location : undefined;

            let data = result.data;

            if ((result = this.blockDelimiters.end.pull(buffer)).isMatch === false)
            {
                return result;
            }

            location.endLocation = result.data.location;

            return {
                    isMatch: true,
                    data:
                    {
                        location: dataStart,
                        value: this.action(location, data !== undefined ? data.value : undefined)
                    }
                };
        }
    }

    return new ExpectBlockListMatcher(arg, blockDelimiterTokens, delimiter, action);
}



//--------------------------------------------------------------------------------------------------
/**
 * Look for an optional list of sub-expressions with a defined beginning and ending.
 */
//--------------------------------------------------------------------------------------------------
export function optionalBlockListOf
(
    /** If specified the matcher will look for this expression between the previous expressions. */
    arg: ArgMatcherType,

    /**
     * If specified, the matcher will expect to find them at the beginning and the ending of the
     * list of items.
     * */
    blockDelimiterTokens: BlockDelimiterTokens,

    /** The expression to attempt to match in the input stream. */
    delimiter?: Token,

    /** . */
    action?: BlockMatchHandler
)
: Matcher
//--------------------------------------------------------------------------------------------------
{
    class OptionalBlockMatcher extends Matcher
    {
        private blockMatcher: Matcher;

        constructor(arg: ArgMatcherType, blockDelimiterTokens?: BlockDelimiterTokens,
                    delimiter?: Token, action?: BlockMatchHandler)
        {
            super();
            this.blockMatcher = expectBlockListOf(arg, blockDelimiterTokens, delimiter, action);
        }

        pull(buffer: TokenBuffer): MatchPullResult
        {
            buffer.markLookahead();
            let result = this.blockMatcher.pull(buffer);

            if (result.isMatch === true)
            {
                buffer.commitLookahead();
            }
            else
            {
                buffer.clearLookahead();
            }

            return result;
        }
    }

    return new OptionalBlockMatcher(arg, blockDelimiterTokens, delimiter, action);
}
