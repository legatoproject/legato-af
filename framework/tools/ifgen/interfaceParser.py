#
# Parse interface files
#
# Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
#

import sys
import pyparsing
import re

# Code generation specific files
import codeTypes


#-----------------------------------
# Parser functions and definitions
#-----------------------------------

def processComment(tokens):
    #print tokens
    parm = tokens[0]
    if len(tokens) > 1:
        # todo: would it be better to return a list of comment lines here?
        parm.comment = '\n'.join(tokens[1:])
    return parm


def ParmListExpr(content):
    """
    Based on pyparsing.nestedExpr, but intended for matching the parameter list
    of a function.  Most of the original functionality has been stripped out, in
    favour of simplying the expression.

    todo: There is another pyparsing helper function, which could possibly be used
          here instead. Not sure why I didn't notice it before.
    """

    opener = pyparsing.Literal('(').suppress()
    closer = pyparsing.Literal(')').suppress()
    separator = pyparsing.Literal(',').suppress()

    # todo: should use doxygenComment instead of pyparsing.cppStyleComment here?
    # Handle the separator between parameter expressions
    parameter_sep = content + separator + pyparsing.ZeroOrMore(pyparsing.cppStyleComment)
    parameter_sep.setParseAction(processComment)

    # The last parameter does not have any trailing separator
    parameter = content + pyparsing.ZeroOrMore(pyparsing.cppStyleComment)
    parameter.setParseAction(processComment)

    # todo: Not sure which of these is better.  Using the 2nd because it is  more explicit
    #ret = pyparsing.Group( opener
    #                       + pyparsing.Optional( pyparsing.ZeroOrMore(content_sep) + content )
    #                       + closer )
    ret = pyparsing.Group( (opener + closer) |
                           (opener + pyparsing.ZeroOrMore(parameter_sep) + parameter + closer) )
    return ret


# Define default value if a named token has not been found, and thus not been set
TokenNotSet = ''

# Define identifier
Identifier = pyparsing.Word(pyparsing.alphas, pyparsing.alphanums+'_')

# Define literals
Semicolon = pyparsing.Literal(';').suppress()
Pointer = pyparsing.Literal('*')
OpenBracket = pyparsing.Literal('[').suppress()
CloseBracket = pyparsing.Literal(']').suppress()
DotDot = pyparsing.Literal('..').suppress()

# Define keywords
KeywordDirIn = pyparsing.Keyword(codeTypes.DIR_IN)
KeywordDirOut = pyparsing.Keyword(codeTypes.DIR_OUT)
Direction = KeywordDirIn | KeywordDirOut

KeywordString = pyparsing.Keyword('string')
KeywordFunction = pyparsing.Keyword('FUNCTION')
KeywordHandler = pyparsing.Keyword('HANDLER')
KeywordAddHandler = pyparsing.Keyword('ADD_HANDLER')
KeywordHandlerParams = pyparsing.Keyword('HANDLER_PARAMS')
KeywordAddHandlerParams = pyparsing.Keyword('ADD_HANDLER_PARAMS')

# Define numbers
DecNumber = pyparsing.Word(pyparsing.nums)
HexNumber = pyparsing.Combine( pyparsing.CaselessLiteral('0x') + pyparsing.Word(pyparsing.hexnums) )
Number = HexNumber | DecNumber
Number.setParseAction( lambda t: int(t[0],0) )

# Define a range expression, which gives maxSize and optionally minSize.
# Used by arrays and strings below.
RangeExpr = pyparsing.Optional( Number('minSize') + DotDot ) + Number('maxSize')


# todo: This function still needs to be cleaned up, and maybe it will completely replace the
#       code that gets executed when the ParseException is detected.
def FailFunc(s, loc, expr, err):
    debug=False
    if debug:
        print 'got here'
        #print s
        print loc
        print repr(s[loc:])
        #print expr
        #print err, repr(err)
        print err
        print pyparsing.lineno(loc, s)

    # This function will always get called at the end of the input string, however, all that is
    # left will be some empty lines, so only print an error if there is valid text left.
    remainingString = s[loc:]
    if remainingString.strip():
        # If the err data is valid, it will give a more accurate location.
        if err:
            lineno = err.lineno
            col = err.col
        else:
            lineno = pyparsing.lineno(loc, s)
            col = pyparsing.col(loc, s)

        print "Parsing Error near line %s, column %s" % (lineno, col)
        print '-'*60
        print '\n'.join( s.splitlines()[lineno-1:] )

        # No need to continue; stop parsing now
        raise pyparsing.ParseFatalException('Stop Parsing')


def ProcessSimpleParm(tokens):
    #print tokens

    # In C, an IN parameter will normally be passed by value, but there may be cases where it
    # would be useful to pass an IN parameter by reference, e.g. if the parameter is a struct.
    # However, structs will probably not be used that often in interfaces, so it may not really
    # matter.
    if tokens.direction == codeTypes.DIR_IN:
        return codeTypes.SimpleData( tokens.name, tokens.type )
    elif tokens.direction == codeTypes.DIR_OUT:
        return codeTypes.PointerData( tokens.name, tokens.type, tokens.direction )

SimpleParm = ( Identifier('type')
                + Identifier('name')
                + pyparsing.Optional(Direction('direction'), default=codeTypes.DIR_IN) )
SimpleParm.setParseAction(ProcessSimpleParm)


def ProcessMaxMinSize(tokens):
    # IN Arrays can have both a minSize and maxSize, whereas OUT Arrays can only have a minSize,
    # so need separate definitions for each.  The following is done instead of having separate parse
    # expressions and associated parse actions for IN and OUT arrays.
    # todo: May want to change the names of the named tokens, so I'm not mixing min/max so much.
    maxSize = None
    minSize = None
    if tokens.direction == codeTypes.DIR_IN:
        maxSize = tokens.maxSize
        if (tokens.minSize != TokenNotSet):
            minSize = tokens.minSize
    else:
        # Must be DIR_OUT
        minSize = tokens.maxSize
        if tokens.minSize != TokenNotSet:
            # This is an error, so raise an parse exception.
            # todo: Use empty string for required parameter, but may need to revisit this
            #       when better error reporting is added.
            raise pyparsing.ParseException('')

    return maxSize, minSize


def ProcessArrayParm(tokens):
    #print tokens

    maxSize, minSize = ProcessMaxMinSize(tokens)
    return codeTypes.ArrayData( tokens.name,
                                tokens.type,
                                tokens.direction,
                                maxSize,
                                minSize )

ArrayParm = ( Identifier('type')
                + Identifier('name')
                + OpenBracket
                + RangeExpr
                + CloseBracket
                + Direction('direction') )
ArrayParm.setParseAction(ProcessArrayParm)


# todo: Does it make sense to have special handling for string OUT parameters?
#       If not, then this and the corresponding codeTypes.StringData() should
#       be changed to only support IN parameters.
def ProcessStringParm(tokens):
    #print tokens

    maxSize, minSize = ProcessMaxMinSize(tokens)
    return codeTypes.StringData( tokens.name, tokens.direction, maxSize, minSize )

StringParm = ( KeywordString
                        + Identifier('name')
                        + OpenBracket
                        + RangeExpr
                        + CloseBracket
                        + Direction('direction') )
StringParm.setParseAction(ProcessStringParm)


def ProcessFunc(tokens):
    #print tokens

    f = codeTypes.FunctionData(
        tokens.funcname,
        tokens.functype if (tokens.functype != TokenNotSet) else 'void',

        # Handle the case where there are no parameters
        tokens.body if tokens.body else [codeTypes.VoidData()]
    )

    # todo: Should this be done in the FunctionData() call above
    if tokens.comment:
        #print tokens.comment
        f.comment = tokens.comment

    return f

def MakeFuncExpr():

    # The expressions are checked in the given order, so the order must not be changed.
    all_parameters = StringParm | ArrayParm | SimpleParm

    # The function type is optional but it doesn't seem work with Optional(), so need to
    # specify two separate expressions.
    typeNameInfo = ( Identifier("functype") + Identifier("funcname") ) | Identifier("funcname")

    body = ParmListExpr(all_parameters)
    # todo: Should this be ZeroOrMore comments, rather than just Optional?
    # todo: Should use something else other than pyparsing.cStyleComment
    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordFunction
            + typeNameInfo
            + body("body")
            + Semicolon )
    all.setParseAction(ProcessFunc)
    all.setFailAction(FailFunc)

    return all


def ProcessHandlerClassFunc(tokens):
    #print tokens

    # todo: It may be better to define a new class and return an instance of that, rather than
    #       returning the two classes here.  Something to consider when the code is cleaned up.
    #
    h = codeTypes.HandlerFunctionData(
        tokens.handlerType + "Func_t",
        "",

        # Handle the case where there are no parameters
        tokens.handlerBody if tokens.handlerBody else [codeTypes.VoidData()]
    )

    f = codeTypes.AddHandlerFunctionData(
        tokens.handlerType,
        tokens.handlerType + "Func_t",

        # Handle the case where there are no parameters
        tokens.addBody if tokens.addBody else [codeTypes.VoidData()]
    )

    #print h
    #print f

    return [h, f]

def MakeHandlerClassExpr():

    # The expressions are checked in the given order, so the order must not be changed.
    allParameters = StringParm | ArrayParm | SimpleParm

    body = ParmListExpr(allParameters)
    handlerParam = KeywordHandlerParams + body("handlerBody") + Semicolon

    body = ParmListExpr(allParameters)
    addParam = KeywordAddHandlerParams + body("addBody") + Semicolon

    # The expressions can be given in any order; in addition, addParam is optional, if there are
    # no parameters to this function.
    allBody = handlerParam & pyparsing.Optional(addParam)

    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordHandler
            + Identifier("handlerType")
            + "{"
            + allBody
            + "}"
            + Semicolon )
    all.setParseAction(ProcessHandlerClassFunc)

    return all


def preProcessVerbatim(text):
    # Extract the #VERBATIM section, and clear it from the text

    verbatim = re.compile("(^#VERBATIM *$(.*)^#ENDVERBATIM *$)", flags=re.M|re.S)
    matches = verbatim.findall(text)
    #print matches

    # If no verbatim section was found, then just return the original text, and
    # an empty verbatim result.
    if not matches:
        return text, ''

    # Process the verbatim section
    outer_match, inner_match = matches[0]

    # Replace the section in the text, with the appropriate number of blank lines
    lineCount = outer_match.count('\n')
    newText = text.replace(outer_match, '\n'*lineCount)
    #print newText,

    # Return the modified text, as well as the contents of the verbatim section,
    # but strip off any leading or trailing whitespace on the verbatim section.
    return newText, inner_match.strip()


def processDoxygen(tokens):

    # If the comment is being replaced by blank lines, need to know how many blank lines
    # to use.  This is necessary so that the line numbers match up for any parsing error
    # messages that may be printed
    lineCount = tokens[0].count('\n')

    #print >> sys.stderr, lineCount, tokens

    if not tokens[0].startswith( ("/**", "///<") ):
        return "\n"*lineCount

    return tokens


def parseCode(codeDefn):
    funcExpr = MakeFuncExpr()
    handlerClassExpr = MakeHandlerClassExpr()

    # todo: There is probably a better way to do this with the expressions above, rather than
    #       defining 'keywords' here, but it would probably require changes to the expression
    #       definitions, so this is good enough for now.
    keywords = KeywordHandler | KeywordFunction

    # Note that funcExpr must be listed last, so that the expression gets applied last.
    allcode = ( pyparsing.ZeroOrMore(pyparsing.cStyleComment + pyparsing.NotAny(keywords))
                + pyparsing.ZeroOrMore(handlerClassExpr | funcExpr) )

    # Extract the VERBATIM section
    codeDefn, verbatim = preProcessVerbatim(codeDefn)

    # Pre-process to remove all comments except for doxygen comments.
    pyparsing.cppStyleComment.setParseAction(processDoxygen)
    codeDefn = pyparsing.cppStyleComment.transformString(codeDefn)

    try:
        resultList =  allcode.parseString(codeDefn, parseAll=True)
    except pyparsing.ParseException, error:
        print "ERROR: stopped parsing at line %s, column %s" % (error.lineno, error.col)
        print '-'*60
        print '\n'.join( codeDefn.splitlines()[error.lineno-1:] )
        sys.exit(1)
    except pyparsing.ParseFatalException, error:
        # Error message has already been printed out before this exception was raised, so no
        # need to do anything further here.
        sys.exit(1)

    # Need to separate the header comments from the code in the raw resultList, and return
    # a dictionary with the appropriate sections
    headerList = []
    codeList = []
    for r in resultList:
        if isinstance( r, str ):
            headerList.append(r)
        else:
            codeList.append(r)

    resultData = dict(headerList=headerList,
                      codeList=codeList,
                      verbatim=verbatim)

    return resultData


def printCode(codeData):
    codeList = codeData['codeList']
    #print codeList

    for c in codeList:
        print '-'*10
        print c



#
# Test Main
#

if __name__ == "__main__" :
    # Add some unit tests here
    pass

