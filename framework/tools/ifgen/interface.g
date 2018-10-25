/*
 * API file ANTLR grammar
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 *
 * Note: After modifying this file, run antlr3 interface.g to regenerate interfaceLexer.py and
 *       interfaceParser.py.  These files are checked-in to avoid adding a dependency on antlr3.
 */

grammar interface;
options {
    language=Python;
}

@header
{
    import os
    import sys

    import interfaceIR
    from interfaceLexer import interfaceLexer

    def UnquoteString(quotedString):
        return quotedString[1:-1].decode('string_escape')

    def StripPostComment(comment):
        return comment[4:]

    def StripPreComment(comment):
        """Get text from C-style doxygen comment."""
        commentLines=comment[3:-2].split(u'\n')
        strippedCommentLines=[]
        inVerbatim=False
        for commentLine in commentLines:
            if commentLine.startswith (' *'):
                 commentLine = commentLine[2:]
            strippedCommentLines.append(commentLine)
            if commentLine.strip() == u'@verbatim':
                inVerbatim = True
            elif commentLine.strip() == u'@endverbatim':
                inVerbatim = False

        result = u'\n'.join(strippedCommentLines)
        return result
}

@init
{
    self.iface = interfaceIR.Interface()
    self.searchPath = []
    self.compileErrors = 0
}

@lexer::members
{
    def getErrorHeader(self, e):
        """
        What is the error header, normally line/character position information?
        """

        source_name = self.getSourceName()
        if source_name is not None:
            return "{}:{}:{}: error:".format(source_name, e.line, e.charPositionInLine)
        return "{}:{}: error:".format(e.line, e.charPositionInLine)

    def getErrorMessage(self, e, tokenNames):
        """Give more user-friendly error messages for some lexing errors"""
        if isinstance(e, NoViableAltException):
            return "Unexpected character " + self.getCharErrorDisplay(e.c)
        else:
            return super(interfaceLexer, self).getErrorMessage(e, tokenNames)

}

@parser::members
{
    INVALID_NUMBER = 0xcdcdcdcd

    def getTokenErrorDisplay(self, t):
        """
        Display a token for an error message.  Since doc comments can be multi-line, only display
        the first line of the token.
        """
        s = t.text
        if s is None:
            # No text -- fallback to default handler
            return super(self, Parser).getTokenErrorDisplay(t)
        else:
            s = s.split('\n', 2)[0]

        return repr(s)

    def getErrorHeader(self, e):
        """
        What is the error header, normally line/character position information?
        """

        source_name = self.getSourceName()
        if source_name is not None:
            return "{}:{}:{}: error:".format(source_name, e.line, e.charPositionInLine)
        return "{}:{}: error:".format(e.line, e.charPositionInLine)

    def getErrorHeaderForToken(self, token):
        """
        What is the error header for a token, normally line/character position information?
        """

        source_name = self.getSourceName()
        if source_name is not None:
            return "{}:{}:{}: error:".format(source_name, token.line, token.charPositionInLine)
        return "{}:{}: error:".format(token.line, token.charPositionInLine)

    def getWarningHeaderForToken(self, token):
        """
        What is the warning header for a token, normally line/character position information?
        """

        source_name = self.getSourceName()
        if source_name is not None:
            return "{}:{}:{}: warning:".format(source_name, token.line, token.charPositionInLine)
        return "{}:{}: warning:".format(token.line, token.charPositionInLine)

}

@footer
{
    def ParseCode(apiFile, searchPath=[], ifaceName=None):
        if os.path.isabs(apiFile) or os.path.isfile(apiFile):
            apiPath = apiFile
        else:
            for path in searchPath:
                apiPath = os.path.join(path, apiFile)
                if os.path.isfile(apiPath):
                    break
            else:
                # File not found in search path.  Try as a relative path.
                # This will usually fail since the current directory should be in the search
                # path but at least will raise a reasonable exception
                apiPath = apiFile

        fileStream = ANTLRFileStream(apiPath, 'utf-8')
        lexer = interfaceLexer(fileStream)
        tokens = CommonTokenStream(lexer)
        parser = interfaceParser(tokens)
        parser.searchPath=searchPath
        baseName = os.path.splitext(os.path.basename(apiPath))[0]
        if ifaceName == None:
            parser.iface.name = baseName
        else:
            parser.iface.name = ifaceName
        parser.iface.baseName = baseName
        parser.iface.path = apiPath

        iface = parser.apiDocument()

        if (parser.getNumberOfSyntaxErrors() > 0 or
            parser.compileErrors > 0):
            return None

        if tokens.getTokens():
            iface.text= ''.join([ token.text
                                for token in tokens.getTokens()
                                if token.type not in frozenset([ C_COMMENT,
                                                               CPP_COMMENT,
                                                               DOC_PRE_COMMENT,
                                                               DOC_POST_COMMENT ]) ])

        return iface
}

/* Character classes */
fragment ALPHA : 'a'..'z' | 'A'..'Z' ;
fragment NUM : '0'..'9' ;
fragment HEXNUM : NUM | 'a'..'f' | 'A'..'F' ;
fragment ALPHANUM : ALPHA | NUM ;

/* Some symbols
 *
 * Generally these are just literals in the rule but we need to refer to some symbols explicitly
 */
SEMICOLON : ';';

/* Keywords */
IN : 'IN' ;
OUT : 'OUT' ;
FUNCTION : 'FUNCTION' ;
HANDLER : 'HANDLER' ;
EVENT : 'EVENT' ;
REFERENCE : 'REFERENCE' ;
DEFINE : 'DEFINE' ;
ENUM : 'ENUM' ;
BITMASK : 'BITMASK' ;
STRUCT : 'STRUCT' ;
USETYPES : 'USETYPES' ;

/** Identifiers */
IDENTIFIER : ALPHA ( ALPHANUM | '_' )* ;

/** Scoped identifiers -- allowed for typenames and defines */
SCOPED_IDENTIFIER : IDENTIFIER '.' IDENTIFIER ;

/** Decimal numbers */
DEC_NUMBER : number=( '0' | '1'..'9' NUM*) ;
HEX_NUMBER : '0' ('x' | 'X' ) HEXNUM+ ;

/* Quoted strings */
QUOTED_STRING : '"' ( ~('\"' | '\\') | '\\' . )* '"'
    | '\'' ( ~('\'' | '\\') | '\\' . )* '\'' ;

/** Documentation comments which come before the item they're documenting */
DOC_PRE_COMMENT : '/**' ~'*' (~'*' | '*'+ ~('/' | '*') )+ '*/' ;

/** Documentation comments which come after the item they're documenting */
DOC_POST_COMMENT : '///<' ~'\n'* ;

/** Skip whitespace */
WS : (' ' | '\t' | '\n')+ { self.skip() } ;

/** Skip C comments which are not documentation comments */
C_COMMENT : '/*' (~'*' | '*'+ ~('/' | '*') )+ '*'+ '/' { self.skip() };

/** Skip C++ comments which are not documentation comments */
CPP_COMMENT : '//' ~'\n'* { self.skip() } ;

/*
 * Grammar
 */
/** Argument direction can be in or out */
direction returns [direction]
    : IN     { $direction = interfaceIR.DIR_IN }
    | OUT    { $direction = interfaceIR.DIR_OUT }
    ;

/** unsigned numbers */
number returns [value]
    : DEC_NUMBER    { $value = int($DEC_NUMBER.text, 10) }
    | HEX_NUMBER    { $value = int($HEX_NUMBER.text, 16) }
    ;

integer returns [value]
    : '+' DEC_NUMBER   { $value = int($DEC_NUMBER.text, 10) }
    | '-' DEC_NUMBER   { $value = -int($DEC_NUMBER.text, 10) }
    | DEC_NUMBER       { $value = int($DEC_NUMBER.text, 10) }
    | HEX_NUMBER       { $value = int($HEX_NUMBER.text, 16) }
    ;

defineValue returns [value]
    : IDENTIFIER        { $value = int(self.iface.findDefinition($IDENTIFIER.text).value) }
    | SCOPED_IDENTIFIER { $value = int(self.iface.findDefinition($SCOPED_IDENTIFIER.text).value) }
    ;
catch [KeyError, e]
{
    self.emitErrorMessage(self.getErrorHeaderForToken($defineValue.start) +
                          " No definition for {}".format($defineValue.text))
    self.compileErrors += 1
}
catch [TypeError, e]
{
    self.emitErrorMessage(self.getErrorHeaderForToken($defineValue.start) +
                          " {} is not an integer definition".format($defineValue.text))
    self.compileErrors += 1
}

/** Either a number or a DEFINE */
simpleNumber returns [value]
    : number            { $value = $number.value }
    | defineValue       { $value = $defineValue.value }
    ;

simpleExpression returns [value]
    : integer               { $value = $integer.value }
    | defineValue           { $value = $defineValue.value }
    | '(' sumExpression ')' { $value = $sumExpression.value }
    ;

productExpression returns [value]
    : initialValue=simpleExpression     { $value = $initialValue.value }
        ( '*' mulValue=simpleExpression { $value *= $mulValue.value }
        | '/' divValue=simpleExpression { $value /= $divValue.value }
        )*
    ;

sumExpression returns [value]
    : initialValue=productExpression     { $value = $initialValue.value }
        ( '+' addValue=productExpression { $value += $addValue.value }
        | '-' subValue=productExpression { $value -= $subValue.value }
        )*
    ;

valueExpression returns [value]
    : sumExpression    { $value = $sumExpression.value }
    | QUOTED_STRING    { $value = UnquoteString($QUOTED_STRING.text) }
    ;

// Array expressions used to allow expressions of the form [minSize..maxSize] in some cases,
// however these never quite worked right, so have been removed.  If they need to be re-added
// for compatibility, suggest ignoring minSize and issue a deprecation warning when encountered.
arrayExpression returns [size]
    : '[' simpleNumber ']'
        { $size = $simpleNumber.value }
    | '[' minSize=simpleNumber '..' maxSize=simpleNumber ']'
        {
            self.emitErrorMessage(self.getWarningHeaderForToken($minSize.start) +
                                  " Minimum lengths are deprecated and will be ignored.")
            $size = $maxSize.value
        }
    ;

typeIdentifier returns [typeObj]
    : IDENTIFIER
        {
            try:
                $typeObj = self.iface.findType($IDENTIFIER.text)
            except KeyError, e:
                self.compileErrors += 1
                self.emitErrorMessage(self.getErrorHeaderForToken($IDENTIFIER) +
                                      " Unknown type {}".format(e))
                $typeObj = interfaceIR.ERROR_TYPE
        }
    | SCOPED_IDENTIFIER
        {
            try:
                $typeObj = self.iface.findType($SCOPED_IDENTIFIER.text)
            except KeyError, e:
                self.compileErrors += 1
                self.emitErrorMessage(self.getErrorHeaderForToken($SCOPED_IDENTIFIER) +
                                      " Unknown type {}".format(e))
                $typeObj = interfaceIR.ERROR_TYPE
        }
    ;

docPostComments returns [comments]
    : { $comments = [] }
      ( DOC_POST_COMMENT
            { $comments.append(StripPostComment($DOC_POST_COMMENT.text)) }
      )*
    ;

docPreComment returns [comment]
    : DOC_PRE_COMMENT { $comment = StripPreComment($DOC_PRE_COMMENT.text) }
    ;

defineDecl returns [define]
    : DEFINE IDENTIFIER '=' valueExpression ';'
        {
            $define = interfaceIR.Definition($IDENTIFIER.text, $valueExpression.value)
        }
    ;

namedValue returns [namedValue]
    : IDENTIFIER ( '=' integer )? docPostComments
        {
            $namedValue = interfaceIR.NamedValue($IDENTIFIER.text,
                                                  $integer.value)
            $namedValue.comments = $docPostComments.comments
        }
    ;

namedValueList returns [values]
    : /*empty*/                                   { $values = [ ] }
    | namedValue                                  { $values = [ $namedValue.namedValue ] }
    | namedValue ',' docPostComments rest=namedValueList
        {
            $namedValue.namedValue.comments.extend($docPostComments.comments)
            $values = [ $namedValue.namedValue ]
            $values.extend($rest.values)
        }
    ;

enumDecl returns [enum]
    : ENUM IDENTIFIER '{' namedValueList '}' ';'
        {
            $enum = interfaceIR.EnumType($IDENTIFIER.text, $namedValueList.values)
        }
    ;

bitmaskDecl returns [bitmask]
    : BITMASK IDENTIFIER '{' namedValueList '}' ';'
        {
            $bitmask = interfaceIR.BitmaskType($IDENTIFIER.text, $namedValueList.values)
        }
    ;

referenceDecl returns [ref]
    : REFERENCE IDENTIFIER ';'
        {
            $ref = interfaceIR.ReferenceType($IDENTIFIER.text)
        }
    ;

compoundMember returns [member]
    : typeIdentifier IDENTIFIER arrayExpression? ';' docPostComments
        {
            if $typeIdentifier.typeObj == interfaceIR.OLD_HANDLER_TYPE or \
               isinstance($typeIdentifier.typeObj, interfaceIR.HandlerReferenceType) or \
               isinstance($typeIdentifier.typeObj, interfaceIR.HandlerType):
                self.compileErrors += 1
                self.emitErrorMessage(self.getErrorHeaderForToken($typeIdentifier.typeObj) +
                                      " Cannot include handlers in a structure")
                $member = None
            else:
                $member = interfaceIR.MakeStructMember(self.iface,
                                                       $typeIdentifier.typeObj,
                                                       $IDENTIFIER.text,
                                                       $arrayExpression.size)
        }
    ;

compoundMemberList returns [members]
    : compoundMember                    { $members = [ $compoundMember.member ] }
    | compoundMember rest=compoundMemberList
        {
            $members = [ $compoundMember.member ]
            $members.extend($rest.members)
        }
    ;

structDecl returns [struct]
    : STRUCT IDENTIFIER '{' compoundMemberList? '}' ';'
        {
            $struct = interfaceIR.StructType($IDENTIFIER.text, $compoundMemberList.members)
        }
    ;

formalParameter returns [parameter]
    : typeIdentifier IDENTIFIER arrayExpression? direction? docPostComments
        {
            if $typeIdentifier.typeObj == interfaceIR.OLD_HANDLER_TYPE:
                self.emitErrorMessage(self.getWarningHeaderForToken($IDENTIFIER) +
                                      " Parameters with type 'handler' are deprecated. "
                                      " Use the name of the handler instead: e.g. {} handler"
                                      .format($IDENTIFIER.text))

            $parameter = interfaceIR.MakeParameter(self.iface,
                                                   $typeIdentifier.typeObj,
                                                   $IDENTIFIER.text,
                                                   $arrayExpression.size,
                                                   $direction.direction)
            $parameter.comments = $docPostComments.comments
        }
    ;

formalParameterList returns [parameters]
    : formalParameter
        {
            $parameters = [ $formalParameter.parameter ]
        }
    | formalParameter ',' docPostComments rest=formalParameterList
        {
            $formalParameter.parameter.comments.extend($docPostComments.comments)
            $parameters = [ $formalParameter.parameter ]
            $parameters.extend($rest.parameters)
        }
    ;

functionDecl returns [function]
    : FUNCTION typeIdentifier? IDENTIFIER '(' formalParameterList? ')' ';'
        {
            if $formalParameterList.parameters == None:
                parameterList = []
            else:
                parameterList = $formalParameterList.parameters
            $function = interfaceIR.Function($typeIdentifier.typeObj,
                                              $IDENTIFIER.text,
                                              parameterList)
        }
    ;

handlerDecl returns [handler]
    : HANDLER IDENTIFIER '(' formalParameterList? ')' ';'
        {
            if $formalParameterList.parameters == None:
                parameterList = []
            else:
                parameterList = $formalParameterList.parameters
            $handler = interfaceIR.HandlerType($IDENTIFIER.text,
                                                parameterList)
        }
    ;

eventDecl returns [event]
    : EVENT IDENTIFIER '(' formalParameterList? ')' ';'
        {
            if $formalParameterList.parameters == None:
                parameterList = []
            else:
                parameterList = $formalParameterList.parameters
            $event = interfaceIR.Event($IDENTIFIER.text,
                                        parameterList)
        }
    ;

declaration returns [declaration]
    : enumDecl        { $declaration = $enumDecl.enum }
    | bitmaskDecl     { $declaration = $bitmaskDecl.bitmask }
    | referenceDecl   { $declaration = $referenceDecl.ref }
    | structDecl      { $declaration = $structDecl.struct }
    | functionDecl    { $declaration = $functionDecl.function }
    | handlerDecl     { $declaration = $handlerDecl.handler }
    | eventDecl       { $declaration = $eventDecl.event }
    | defineDecl      { $declaration = $defineDecl.define }
    ;
catch [RecognitionException, re]
{
    # Report recognition exceptions here with default handling.  Do not want them to get caught
    # by generic handler below
    self.reportError(re)
    self.recover(self.input, re)
}
catch [Exception, e]
{
    self.emitErrorMessage(self.getErrorHeaderForToken(self.getCurrentInputSymbol(self.input))
                          + " " + str(e))
    self.compileErrors += 1
}

documentedDeclaration returns [declaration]
    : docPreComment declaration
        {
            $declaration = $declaration.declaration
            if $declaration:
                $declaration.comment = $docPreComment.comment
        }
    ;

// Restrict filenames to identifiers since they will be used as identifiers elsewhere
// A file with an extension (usually '.api') will be treated as a SCOPED_IDENTIFIER by the lexer
filename returns [filename]
    : IDENTIFIER         { $filename = $IDENTIFIER.text }
    | SCOPED_IDENTIFIER  { $filename = $SCOPED_IDENTIFIER.text }
    ;

usetypesStmt
    : USETYPES filename ';'
        {
            if $filename.filename.endswith(".api"):
                basename=$filename.filename[:-4]
                fullFilename=$filename.filename
            else:
                basename=$filename.filename
                fullFilename=$filename.filename + ".api"

            self.iface.imports[basename] = ParseCode(fullFilename, self.searchPath)
        }
    ;

// An apiDocument starts with some file level comments, followed by a series of import statements
// and possibly documented declarations.  If it's ambiguous if a comment belongs to
// a declaration or the file, it belongs to the declaration
apiDocument returns [iface]
    : ( ( docPreComment { self.iface.comments.append($docPreComment.comment) } )+
            (  usetypesStmt
            | firstDecl=documentedDeclaration
                {
                     if $firstDecl.declaration:
                         self.iface.addDeclaration($firstDecl.declaration)
                } )
      )?
      ( usetypesStmt
        | declaration
             {
                if $declaration.declaration:
                     self.iface.addDeclaration($declaration.declaration)
             }
        | laterDecl=documentedDeclaration
             {
                  if $laterDecl.declaration:
                       self.iface.addDeclaration($laterDecl.declaration)
             }
      )* EOF { $iface = self.iface }
    ;
