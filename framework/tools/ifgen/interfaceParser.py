#
# Parse interface files
#
# Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
#

import sys
import pyparsing
import re
import functools

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


def MakeListExpr(content, opener='(', closer=')'):
    """
    Based on pyparsing.nestedExpr, but intended for matching a list of expressions, such as the
    parameter list of a function, or a list of enumerated values.  Each expression in the list
    can also have associated comments, using a doxygen-like style.

    Most of the original functionality has been stripped out, in favour of simplying the expression,
    since nesting is not actually needed.  This is also similar to the delimitedList, but that can't
    be used because of the need to include comments.
    """

    if isinstance(opener, basestring):
        opener = pyparsing.Literal(opener).suppress()
    if isinstance(closer, basestring):
        closer = pyparsing.Literal(closer).suppress()
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
TypeIdentifier = pyparsing.Combine( Identifier
                                    + pyparsing.Optional( pyparsing.Literal('.') + Identifier) )

# Define literals
Semicolon = pyparsing.Literal(';').suppress()
Pointer = pyparsing.Literal('*')
OpenBracket = pyparsing.Literal('[').suppress()
CloseBracket = pyparsing.Literal(']').suppress()
DotDot = pyparsing.Literal('..').suppress()
Equal = pyparsing.Literal('=').suppress()

# Define keywords
KeywordDirIn = pyparsing.Keyword(codeTypes.DIR_IN)
KeywordDirOut = pyparsing.Keyword(codeTypes.DIR_OUT)
Direction = KeywordDirIn | KeywordDirOut

# Define keywords for built-in types
KeywordString = pyparsing.Keyword('string')
KeywordFile = pyparsing.Keyword('file')

# Define these string literals as variables, since they are used in several places
StringFunction = 'FUNCTION'
StringHandler = 'HANDLER'
StringHandlerParams = 'HANDLER_PARAMS'
StringAddHandlerParams = 'ADD_HANDLER_PARAMS'
StringReference = 'REFERENCE'
StringDefine = 'DEFINE'
StringEnum = 'ENUM'

# Originally, the keyword was 'IMPORT', but this was changed to 'USETYPES' to better convey what
# actually happens when importing a .api file.  Within the implementation, this is still called
# importing.
StringImport = 'USETYPES'

# Convert the string literals to pyparsing keywords
KeywordFunction = pyparsing.Keyword(StringFunction)
KeywordHandler = pyparsing.Keyword(StringHandler)
KeywordHandlerParams = pyparsing.Keyword(StringHandlerParams)
KeywordAddHandlerParams = pyparsing.Keyword(StringAddHandlerParams)
KeywordReference = pyparsing.Keyword(StringReference)
KeywordDefine = pyparsing.Keyword(StringDefine)
KeywordEnum = pyparsing.Keyword(StringEnum)
KeywordImport = pyparsing.Keyword(StringImport)

# List of valid keywords, used when handling parser errors in FailFunc()
KeywordList = [ StringFunction,
                StringHandler,
                StringHandlerParams,
                StringAddHandlerParams,
                StringReference,
                StringDefine,
                StringEnum,
                StringImport]

# Define numbers
# todo: This assumes numbers are positive; should we handle negative numbers as well?
DecNumber = pyparsing.Word(pyparsing.nums)
HexNumber = pyparsing.Combine( pyparsing.CaselessLiteral('0x') + pyparsing.Word(pyparsing.hexnums) )
Number = HexNumber | DecNumber
Number.setParseAction( lambda t: int(t[0],0) )

# Define a range expression, which gives maxSize and optionally minSize.
# Used by arrays and strings below.
RangeExpr = ( pyparsing.Optional( (TypeIdentifier | Number)('minSize') + DotDot )
              + (TypeIdentifier | Number)('maxSize') )



#
# Get the keyword at the start of the string.  This could either be:
#  - a sequence of all-cap chars, along with underscore, or if not found,
#  - a space-separated token.
#
PatternKeyword = re.compile("^\s*([A-Z][A-Z_]*)")
def GetKeyword(s):
    result = PatternKeyword.findall(s)
    if result:
        keyword = result[0]
    else:
        keyword = s.split(None, 1)[0]

    return keyword


#
# This function gets called every time one of the registered parse expressions fails. Due to the
# way the parser works, this means there will be a lot of false positives.  Thus, some filtering
# needs to be done to ensure that the failure/error is indeed valid.
#
def FailFunc(s, loc, expr, err, expected=''):
    #print '*'*60 + " FailFunc called " + '*'*60
    if False:
        #print "Expected =", expected
        print 'got here'
        #print s
        print loc
        print repr(s[loc:])
        #print expr
        #print err, repr(err)
        print err
        print pyparsing.lineno(loc, s)

    # Get the remaining string to parse
    remainingString = s[loc:]

    # This function will always get called at the end of the input string, however, all that is
    # left will be some empty lines, so only check for errors if there is valid text left.
    if not remainingString.strip():
        return

    # Check the keyword of the remaining input string
    keyword = GetKeyword(remainingString)

    # Handle the comment header
    if keyword.startswith( '/**' ) :

        if False:
            print 'Got comment header'
            print "expected =", expected
            print err
            print repr(remainingString)
            print '-'*40

        # Skip over the comment header and continue. Note that 'loc' must be updated to reflect the
        # new starting position, but 'err' has the correct position, if it is used.
        initialCommentHeader, remainingString = remainingString.split('*/', 1)
        keyword = GetKeyword(remainingString)
        #print 'keyword = ', keyword

        # Update 'loc':
        #  - +2 since the '*/' is not part of the initial comment header
        #  - additional offset required, since keyword may not be at the start of remainingString,
        #    if there is whitespace between '*/' and keyword.
        loc += ( len(initialCommentHeader) + 2 + remainingString.index(keyword) )

    #print 'keyword = ', keyword

    # The 'ADD_HANDLER_PARAMS' and 'HANDLER_PARAMS' keywords are optional within a HANDLER expression
    # and can appear in any order within the expression.  Thus, the associated parse expressions
    # could fail when we reach the end of the HANDLER definition, and so this is not an error.
    if expected in [ StringHandlerParams, StringAddHandlerParams ] and keyword[0] == '}':
        #print "Got %s special case" % expected
        #print '*'*80
        return

    # If it is a valid keyword but not the expected one, then this is not an error.
    if keyword in KeywordList:
        if keyword != expected:
            if False:
                print "Expected '%s', got '%s'" % (expected, keyword)
                print '='*80
            return

        # A valid failure/error has occurred.
        # If the err data is valid, it will give a more accurate location.
        if err:
            lineno = err.lineno
            col = err.col
        else:
            lineno = pyparsing.lineno(loc, s)
            col = pyparsing.col(loc, s)

    # This is an unknown keyword, which is also an error
    else:
        #print "Unknown keyword '%s'" % keyword
        lineno = pyparsing.lineno(loc, s)
        col = pyparsing.col(loc, s)
        err = '** Unknown keyword "%s" (at char %s), (line:%s, col:%s) **' % (keyword, loc, lineno, col)


    print "Parsing Error near line %s, column %s" % (lineno, col)
    print '-'*60
    print err
    print '-'*60
    print '\n'.join( s.splitlines()[lineno-1:] )

    # No need to continue; stop parsing now
    # todo: Got some odd behaviour.  This exception didn't actually stop the parsing when
    #       trying to handle HANDLER definition errors.  Use just a regular exit instead.
    #raise pyparsing.ParseFatalException('Stop Parsing')
    sys.exit(1)


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

SimpleParm = ( TypeIdentifier('type')
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

    # One or both of maxSize and minSize could be a previously DEFINEd value, in which case the
    # value would be a string giving the name.  This needs to be evaluated to get the actual value.
    if isinstance(maxSize, basestring):
        maxSize = codeTypes.EvaluateDefinition(maxSize)
    if isinstance(minSize, basestring):
        minSize = codeTypes.EvaluateDefinition(minSize)

    return maxSize, minSize


def ProcessArrayParm(tokens):
    #print tokens

    maxSize, minSize = ProcessMaxMinSize(tokens)
    return codeTypes.ArrayData( tokens.name,
                                tokens.type,
                                tokens.direction,
                                maxSize,
                                minSize )

ArrayParm = ( TypeIdentifier('type')
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


def ProcessFileParm(tokens):
    #print tokens

    if tokens.direction == codeTypes.DIR_IN:
        return codeTypes.FileInData( tokens.name, tokens.direction )
    elif tokens.direction == codeTypes.DIR_OUT:
        return codeTypes.FileOutData( tokens.name, tokens.direction )

FileParm = ( KeywordFile
             + Identifier('name')
             + Direction('direction') )
FileParm.setParseAction(ProcessFileParm)


def ProcessFunc(tokens):
    #print tokens

    f = codeTypes.FunctionData(
        tokens.funcname,
        tokens.functype if (tokens.functype != TokenNotSet) else 'void',

        # Handle the case where there are no parameters
        tokens.body if tokens.body else [codeTypes.VoidData()],
        tokens.comment
    )

    return f

def MakeFuncExpr():

    # The expressions are checked in the given order, so the order must not be changed.
    all_parameters = StringParm | ArrayParm | FileParm | SimpleParm

    # The function type is optional but it doesn't seem work with Optional(), so need to
    # specify two separate expressions.
    typeNameInfo = ( TypeIdentifier("functype") + Identifier("funcname") ) | Identifier("funcname")

    body = MakeListExpr(all_parameters)
    # todo: Should this be ZeroOrMore comments, rather than just Optional?
    # todo: Should use something else other than pyparsing.cStyleComment
    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordFunction
            + typeNameInfo
            + body("body")
            + Semicolon )
    all.setParseAction(ProcessFunc)
    all.setFailAction(functools.partial(FailFunc, expected=StringFunction))

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
        tokens.handlerBody if tokens.handlerBody else [codeTypes.VoidData()],

        # Handler Class comment will be used for the handler function definition
        tokens.comment
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

    body = MakeListExpr(allParameters)
    handlerParam = KeywordHandlerParams + body("handlerBody") + Semicolon
    handlerParam.setFailAction(functools.partial(FailFunc, expected=StringHandlerParams))

    body = MakeListExpr(allParameters)
    addParam = KeywordAddHandlerParams + body("addBody") + Semicolon
    addParam.setFailAction(functools.partial(FailFunc, expected=StringAddHandlerParams))

    # The expressions can be given in any order; in addition, addParam is optional, if there are
    # no parameters to this function.
    allBody =  pyparsing.Optional(handlerParam) & pyparsing.Optional(addParam)

    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordHandler
            + Identifier("handlerType")
            + "{"
            + allBody
            + "}"
            + Semicolon )
    all.setParseAction(ProcessHandlerClassFunc)

    all.setFailAction(functools.partial(FailFunc, expected=StringHandler))
    return all


def ProcessRef(tokens):
    #print tokens

    r = codeTypes.ReferenceData(tokens.name,
                                tokens.comment)

    return r

def MakeRefExpr():

    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordReference
            + Identifier("name")
            + Semicolon )

    all.setParseAction(ProcessRef)
    all.setFailAction(functools.partial(FailFunc, expected=StringReference))

    return all


def ProcessDefine(tokens):
    #print tokens

    # todo: Some additional checks should probably be done on value ...
    if tokens.value == tokens.expression:
        #print 'got expr', tokens.value
        tokens.value = codeTypes.EvaluateDefinition(tokens.value)

    r = codeTypes.DefineData(tokens.name,
                             tokens.value,
                             tokens.comment)

    return r

def MakeDefineExpr():

    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordDefine
            + Identifier("name")
            + Equal

            # The defined value can either be a string (double quotes), a character (single quotes)
            # or an expression that should evaluate to a number.  The expression can contain
            # previously DEFINEd symbols, and will be evaluated immediately.
            + ( pyparsing.quotedString | pyparsing.Regex(r"[^;]+")("expression") )("value")

            + Semicolon )

    all.setParseAction(ProcessDefine)
    all.setFailAction(functools.partial(FailFunc, expected=StringDefine))

    return all



def ProcessEnumValue(tokens):
    #print tokens

    r = codeTypes.EnumValue(tokens.name,
                            tokens.comment)

    return r

EnumValue = TypeIdentifier('name')
EnumValue.setParseAction(ProcessEnumValue)


def ProcessEnum(tokens):
    #print tokens

    r = codeTypes.EnumData(tokens.name,
                           tokens.valueList,
                           tokens.comment)

    return r

def MakeEnumExpr():

    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordEnum
            + Identifier("name")
            + MakeListExpr(EnumValue, opener='{', closer='}')("valueList")
            + Semicolon )

    all.setParseAction(ProcessEnum)
    all.setFailAction(functools.partial(FailFunc, expected=StringEnum))

    return all


def ProcessImport(tokens):
    #print tokens

    r = codeTypes.ImportData(tokens.name)

    return r

def MakeImportExpr(useFailAction=True):

    all = ( KeywordImport
            + pyparsing.Combine( Identifier("name") + pyparsing.Optional(".api") )
            + Semicolon )

    all.setParseAction(ProcessImport)

    if useFailAction:
        # This fail action is not compatible with the searchString() used in getImportList
        all.setFailAction(functools.partial(FailFunc, expected=StringImport))

    return all


def processDoxygen(tokens):

    # If the comment is being replaced by blank lines, need to know how many blank lines
    # to use.  This is necessary so that the line numbers match up for any parsing error
    # messages that may be printed
    lineCount = tokens[0].count('\n')

    #print >> sys.stderr, lineCount, tokens

    if not tokens[0].startswith( ("/**", "///<") ):
        return "\n"*lineCount

    return tokens


#
# Get the valid import statements, so that the imported files can be processed before the main file.
# Any errors in the import statements will be caught during the normal processing of the main file.
#
def getImportList(codeDefn):
    # Create the IMPORT expression, but ignore IMPORT statements in comments.
    importExpr = MakeImportExpr(useFailAction=False)
    importExpr.ignore(pyparsing.cppStyleComment)

    # Exract valid import statements, and return the result as a simple list
    result = importExpr.searchString(codeDefn)
    result = [ r[0] for r in result ]
    return result


def parseCode(codeDefn):
    funcExpr = MakeFuncExpr()
    handlerClassExpr = MakeHandlerClassExpr()
    refExpr = MakeRefExpr()
    defineExpr = MakeDefineExpr()
    enumExpr = MakeEnumExpr()
    importExpr = MakeImportExpr()

    # Define an expression containing all keywords that can be preceded by a doxygen-style comment.
    # todo: There is probably a better way to do this with the expressions above, rather than
    #       defining 'keywords' here, but it would probably require changes to the expression
    #       definitions, so this is good enough for now.
    keywords = ( KeywordFunction
                 | KeywordHandler
                 | KeywordReference
                 | KeywordDefine
                 | KeywordEnum )

    # The expressions are applied in the order listed, so give the more common expressions first.
    allcode = ( pyparsing.ZeroOrMore(pyparsing.cStyleComment + pyparsing.NotAny(keywords))
                + pyparsing.ZeroOrMore( funcExpr
                                        | handlerClassExpr
                                        | refExpr
                                        | defineExpr
                                        | enumExpr
                                        | importExpr ) )

    # Pre-process to remove all comments except for doxygen comments.
    pyparsing.cppStyleComment.setParseAction(processDoxygen)
    codeDefn = pyparsing.cppStyleComment.transformString(codeDefn)

    try:
        resultList =  allcode.parseString(codeDefn, parseAll=True)
    except pyparsing.ParseException, error:
        print "ERROR: stopped parsing at line %s, column %s" % (error.lineno, error.col)
        print '-'*60
        print error
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
    importList = []
    for r in resultList:
        if isinstance( r, str ):
            headerList.append(r)
        elif isinstance(r, codeTypes.ImportData):
            importList.append(r)
        else:
            codeList.append(r)

    resultData = dict(headerList=headerList,
                      codeList=codeList,
                      importList=importList)

    return resultData



#
# Test Main
#

if __name__ == "__main__" :
    # Add some unit tests here
    pass

