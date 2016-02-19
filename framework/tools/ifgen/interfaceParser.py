#
# Parse interface files
#
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#

import sys
import pyparsing
import re
import functools

# Code generation specific files
import codeTypes


#-----------------------------------
# Definitions
#-----------------------------------


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
KeywordHandlerParm = pyparsing.Keyword('handler')

# Define these string literals as variables, since they are used in several places
StringFunction = 'FUNCTION'
StringHandler = 'HANDLER'
StringEvent = 'EVENT'
StringHandlerParams = 'HANDLER'
StringAddHandlerParams = 'ADD_HANDLER'
StringReference = 'REFERENCE'
StringDefine = 'DEFINE'
StringEnum = 'ENUM'
StringBitMask = 'BITMASK'

# Originally, the keyword was 'IMPORT', but this was changed to 'USETYPES' to better convey what
# actually happens when importing a .api file.  Within the implementation, this is still called
# importing.
StringImport = 'USETYPES'

# Convert the string literals to pyparsing keywords
KeywordFunction = pyparsing.Keyword(StringFunction)
KeywordHandler = pyparsing.Keyword(StringHandler)
KeywordEvent = pyparsing.Keyword(StringEvent)
KeywordHandlerParams = pyparsing.Keyword(StringHandlerParams)
KeywordAddHandlerParams = pyparsing.Keyword(StringAddHandlerParams)
KeywordReference = pyparsing.Keyword(StringReference)
KeywordDefine = pyparsing.Keyword(StringDefine)
KeywordEnum = pyparsing.Keyword(StringEnum)
KeywordBitMask = pyparsing.Keyword(StringBitMask)
KeywordImport = pyparsing.Keyword(StringImport)

# List of valid keywords, used when handling parser errors in FailFunc()
KeywordList = [ StringFunction,
                StringHandler,
                StringEvent,
                StringHandlerParams,
                StringAddHandlerParams,
                StringReference,
                StringDefine,
                StringEnum,
                StringBitMask,
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



#-----------------------------------
# Utility functions
#-----------------------------------


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



def PrintContext(lines, lineNum, numContext):
    # Line numbers start at 1
    numberedLines = [ (i+1, l) for i, l in enumerate(lines) ]

    startIndex = lineNum-numContext-1
    if startIndex < 0:
        startIndex = 0

    endIndex = lineNum+numContext
    if endIndex > len(numberedLines):
        endIndex = len(numberedLines)

    for n, l in numberedLines[startIndex:endIndex]:
        prefix = ' '*3 if (n != lineNum) else '-> '
        print '%s%05i : %s' % (prefix, n, l)



def PrintErrorMessage(text, lineNum, col, errMsg):
    lines = text.splitlines()

    # lineNum could be more than number of lines, if the error is at the end of the file,
    # so adjust it to the last line in this case.
    if lineNum > len(lines):
        lineNum = len(lines)

    # User friendlier (hopefully) error message, with a few lines of context
    divider = '-'*60
    print "Parsing Error near line %s, column %s" % (lineNum, col)
    print divider
    print errMsg
    print divider
    PrintContext(lines, lineNum, 3)
    print divider

    # The GCC style error for those IDEs that understand it
    print "%s:%s:%s: error: %s" % (CurrentFileName, lineNum, col, errMsg)



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

    # Set default error message; may get overwritten below
    errMsg = err.msg

    # If it is a valid keyword but not the expected one, then this is not an error.
    if keyword in KeywordList:
        if keyword != expected:
            if False:
                print "Expected '%s', got '%s'" % (expected, keyword)
                print '='*80
            return

        # A valid failure/error has occurred.
        # If the 'err' data is valid, it will give a more accurate location, as long as the 'loc'
        # attribute has been set.  For exceptions generated within this file, 'loc' is not set.
        # The default value of 'loc' is zero, so checking against zero should be a valid way of
        # determining if it was set, since we should never get here if 'loc' is legitimately zero.
        if err and err.loc != 0:
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
        errMsg = "unknown keyword '%s'" % keyword


    # It is a real error, so print out the error message, and exit right away
    PrintErrorMessage(s, lineno, col, errMsg)
    sys.exit(1)



#
# Wrapper around codeTypes.EvaluateDefinition, with exception handling
#
def EvaluateDefinition(expr):

    try:
        result = codeTypes.EvaluateDefinition(expr)

    except NameError as error:
        raise pyparsing.ParseException("name error in expression '%s' : %s" % (expr.strip(), error))

    except SyntaxError as error:
        raise pyparsing.ParseException("syntax error in expression '%s'" % expr.strip())

    except:
        raise pyparsing.ParseException("unknown error in expression '%s'" % expr.strip())

    return result



def ProcessComment(tokens):
    #print tokens
    parm = tokens[0]
    if len(tokens) > 1:
        # todo: would it be better to return a list of comment lines here?
        parm.comment = '\n'.join(tokens[1:])
    return parm



def MakeListExpr(content, opener='(', closer=')', allowTrailingSep=False):
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
    parameter_sep.setParseAction(ProcessComment)

    # The last parameter does not have any trailing separator, but if a trailing separator is
    # allowed, this last parameter is optional.
    parameter = content + pyparsing.ZeroOrMore(pyparsing.cppStyleComment)
    parameter.setParseAction(ProcessComment)
    if allowTrailingSep:
        parameter = pyparsing.Optional(parameter)

    ret = pyparsing.Group( (opener + closer) |
                           (opener + pyparsing.ZeroOrMore(parameter_sep) + parameter + closer) )
    return ret



#-------------------------------------------------------
# Parser expressions and associated processing functions
#-------------------------------------------------------


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
        maxSize = EvaluateDefinition(maxSize)
    if isinstance(minSize, basestring):
        minSize = EvaluateDefinition(minSize)

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


def ProcessHandlerParm(tokens):
    #print tokens

    return codeTypes.HandlerTypeParmData( tokens.name )

HandlerParm = ( KeywordHandlerParm
                + Identifier('name') )
HandlerParm.setParseAction(ProcessHandlerParm)


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
    all_parameters = HandlerParm | StringParm | ArrayParm | FileParm | SimpleParm

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



def ProcessHandlerFunc(tokens):
    #print tokens

    h = codeTypes.HandlerFunctionData(
        tokens.type,

        # Handle the case where there are no parameters
        tokens.body if tokens.body else [codeTypes.VoidData()],
        tokens.comment
    )

    return h

def MakeHandlerExpr():

    # The expressions are checked in the given order, so the order must not be changed.
    allParameters = StringParm | FileParm | SimpleParm

    body = MakeListExpr(allParameters)
    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordHandler
            + Identifier("type")
            + body("body")
            + Semicolon )
    all.setParseAction(ProcessHandlerFunc)
    all.setFailAction(functools.partial(FailFunc, expected=StringHandler))

    return all



def ProcessEventFunc(tokens):
    #print tokens

    f = codeTypes.AddHandlerFunctionData(
        tokens.eventName,

        # Handle the case where there are no parameters (todo: is this still necessary?)
        tokens.body if tokens.body else [codeTypes.VoidData()],
        tokens.comment
    )

    return f

def MakeEventExpr():

    # The expressions are checked in the given order, so the order must not be changed.
    allParameters = HandlerParm | StringParm | ArrayParm | FileParm | SimpleParm

    body = MakeListExpr(allParameters)
    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordEvent
            + Identifier("eventName")
            + body("body")
            + Semicolon )
    all.setParseAction(ProcessEventFunc)

    all.setFailAction(functools.partial(FailFunc, expected=StringEvent))
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

    r = codeTypes.DefineData(tokens.name,
                             tokens.value,
                             tokens.comment)

    return r

def ProcessDefineValue(tokens):
    #print tokens

    # todo: Some additional checks could probably be done on value ...
    if tokens.value == tokens.expression:
        #print 'In ProcessValue, got expr', tokens.value
        tokens.value = EvaluateDefinition(tokens.value)

    return tokens.value

def MakeDefineExpr():

    # The defined value can either be a string (double quotes), a character (single quotes)
    # or an expression that should evaluate to a number.  The expression can contain
    # previously DEFINEd symbols, and will be evaluated immediately.
    valueExpr = ( pyparsing.quotedString | pyparsing.Regex(r"[^;]+")("expression") )("value")
    valueExpr.setParseAction(ProcessDefineValue)

    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordDefine
            + Identifier("name")
            + Equal
            + valueExpr
            + Semicolon )

    all.setParseAction(ProcessDefine)
    all.setFailAction(functools.partial(FailFunc, expected=StringDefine))

    return all



def ProcessEnumMember(tokens):
    #print tokens

    r = codeTypes.EnumMemberData(tokens.name,
                                 tokens.comment)

    return r

EnumMember = Identifier('name')
EnumMember.setParseAction(ProcessEnumMember)


def ProcessEnum(tokens):
    #print tokens

    r = codeTypes.EnumData(tokens.name,
                           tokens.memberList,
                           tokens.comment)

    return r

def MakeEnumExpr():

    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordEnum
            + Identifier("name")
            + MakeListExpr(EnumMember, opener='{', closer='}', allowTrailingSep=True)("memberList")
            + Semicolon )

    all.setParseAction(ProcessEnum)
    all.setFailAction(functools.partial(FailFunc, expected=StringEnum))

    return all


def ProcessBitMask(tokens):
    #print tokens

    r = codeTypes.BitMaskData(tokens.name,
                              tokens.memberList,
                              tokens.comment)

    return r

def MakeBitMaskExpr():

    all = ( pyparsing.Optional(pyparsing.cStyleComment)("comment")
            + KeywordBitMask
            + Identifier("name")
            + MakeListExpr(EnumMember, opener='{', closer='}', allowTrailingSep=True)("memberList")
            + Semicolon )

    all.setParseAction(ProcessBitMask)
    all.setFailAction(functools.partial(FailFunc, expected=StringBitMask))

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


def ProcessDoxygen(tokens):

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
def GetImportList(codeDefn):
    # As an optimization, return an emtpy list right away if there are no import statements.
    if not StringImport in codeDefn:
        return []

    # Create the IMPORT expression, but ignore IMPORT statements in comments.
    importExpr = MakeImportExpr(useFailAction=False)
    importExpr.ignore(pyparsing.cppStyleComment)

    # Extract valid import statements, and return the result as a simple list
    result = importExpr.searchString(codeDefn)
    result = [ r[0].name for r in result ]
    return result



#
# Get a hashable version of the given text. This will have all comments and whitespace removed.
#

# Take the regex from pyparsing, since it already has it defined
CommentRegEx = re.compile(pyparsing.cppStyleComment.pattern)

# Have a separate regex just for whitespace
WhitespaceRegEx = re.compile('\s')

def GetHashText(codeDefn):
    # Remove all comments and then all spaces.  This is done in two steps, since the comment
    # regex is already complicated enough as it is.
    codeDefn = CommentRegEx.sub('', codeDefn)
    codeDefn = WhitespaceRegEx.sub('', codeDefn)

    return codeDefn



def ParseCode(codeDefn, filename):
    # The file name is used when printing error messages
    global CurrentFileName
    CurrentFileName = filename

    funcExpr = MakeFuncExpr()
    handlerExpr = MakeHandlerExpr()
    eventExpr = MakeEventExpr()
    refExpr = MakeRefExpr()
    defineExpr = MakeDefineExpr()
    enumExpr = MakeEnumExpr()
    bitMaskExpr = MakeBitMaskExpr()
    importExpr = MakeImportExpr()

    # Define an expression containing all keywords that can be preceded by a doxygen-style comment.
    # todo: There is probably a better way to do this with the expressions above, rather than
    #       defining 'keywords' here, but it would probably require changes to the expression
    #       definitions, so this is good enough for now.
    keywords = ( KeywordFunction
                 | KeywordHandler
                 | KeywordEvent
                 | KeywordReference
                 | KeywordDefine
                 | KeywordEnum
                 | KeywordBitMask )

    # The expressions are applied in the order listed, so give the more common expressions first.
    allcode = ( pyparsing.ZeroOrMore(pyparsing.cStyleComment + pyparsing.NotAny(keywords))
                + pyparsing.ZeroOrMore( funcExpr
                                        | handlerExpr
                                        | eventExpr
                                        | refExpr
                                        | defineExpr
                                        | enumExpr
                                        | bitMaskExpr
                                        | importExpr ) )

    # Pre-process to remove all comments except for doxygen comments.
    pyparsing.cppStyleComment.setParseAction(ProcessDoxygen)
    codeDefn = pyparsing.cppStyleComment.transformString(codeDefn)

    # Error handling is done in FailFunc() now.  However, just in case a parser exception slips
    # through, handle it here, although the error message and/or location may not be as accurate
    # as when handled by FailFunc().  In the rare case that another, unexpected, exception happens,
    # then just let it crash the program so that we get a traceback.
    try:
        resultList =  allcode.parseString(codeDefn, parseAll=True)
    except pyparsing.ParseException as error:
        print "** Unexpected ParseException occurred **"
        PrintErrorMessage(codeDefn, error.lineno, error.col, error.msg)
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

