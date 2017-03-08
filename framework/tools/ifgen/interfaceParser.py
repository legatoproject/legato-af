# $ANTLR 3.5.2 interface.g 2017-04-04 16:17:41

import sys
from antlr3 import *
from antlr3.compat import set, frozenset


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



# for convenience in actions
HIDDEN = BaseRecognizer.HIDDEN

# token types
EOF=-1
T__29=29
T__30=30
T__31=31
T__32=32
T__33=33
T__34=34
T__35=35
T__36=36
T__37=37
T__38=38
T__39=39
T__40=40
ALPHA=4
ALPHANUM=5
BITMASK=6
CPP_COMMENT=7
C_COMMENT=8
DEC_NUMBER=9
DEFINE=10
DOC_POST_COMMENT=11
DOC_PRE_COMMENT=12
ENUM=13
EVENT=14
FUNCTION=15
HANDLER=16
HEXNUM=17
HEX_NUMBER=18
IDENTIFIER=19
IN=20
NUM=21
OUT=22
QUOTED_STRING=23
REFERENCE=24
SCOPED_IDENTIFIER=25
SEMICOLON=26
USETYPES=27
WS=28

# token names
tokenNames = [
    "<invalid>", "<EOR>", "<DOWN>", "<UP>",
    "ALPHA", "ALPHANUM", "BITMASK", "CPP_COMMENT", "C_COMMENT", "DEC_NUMBER",
    "DEFINE", "DOC_POST_COMMENT", "DOC_PRE_COMMENT", "ENUM", "EVENT", "FUNCTION",
    "HANDLER", "HEXNUM", "HEX_NUMBER", "IDENTIFIER", "IN", "NUM", "OUT",
    "QUOTED_STRING", "REFERENCE", "SCOPED_IDENTIFIER", "SEMICOLON", "USETYPES",
    "WS", "'('", "')'", "'*'", "'+'", "','", "'-'", "'/'", "'='", "'['",
    "']'", "'{'", "'}'"
]




class interfaceParser(Parser):
    grammarFileName = "interface.g"
    api_version = 1
    tokenNames = tokenNames

    def __init__(self, input, state=None, *args, **kwargs):
        if state is None:
            state = RecognizerSharedState()

        super(interfaceParser, self).__init__(input, state, *args, **kwargs)

        self.dfa12 = self.DFA12(
            self, 12,
            eot = self.DFA12_eot,
            eof = self.DFA12_eof,
            min = self.DFA12_min,
            max = self.DFA12_max,
            accept = self.DFA12_accept,
            special = self.DFA12_special,
            transition = self.DFA12_transition
            )

        self.dfa15 = self.DFA15(
            self, 15,
            eot = self.DFA15_eot,
            eof = self.DFA15_eof,
            min = self.DFA15_min,
            max = self.DFA15_max,
            accept = self.DFA15_accept,
            special = self.DFA15_special,
            transition = self.DFA15_transition
            )




        self.iface = interfaceIR.Interface()
        self.searchPath = []
        self.compileErrors = 0


        self.delegates = []





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



    # $ANTLR start "direction"
    # interface.g:209:1: direction returns [direction] : ( IN | OUT );
    def direction(self, ):
        direction = None


        try:
            try:
                # interface.g:210:5: ( IN | OUT )
                alt1 = 2
                LA1_0 = self.input.LA(1)

                if (LA1_0 == IN) :
                    alt1 = 1
                elif (LA1_0 == OUT) :
                    alt1 = 2
                else:
                    nvae = NoViableAltException("", 1, 0, self.input)

                    raise nvae


                if alt1 == 1:
                    # interface.g:210:7: IN
                    pass
                    self.match(self.input, IN, self.FOLLOW_IN_in_direction512)

                    #action start
                    direction = interfaceIR.DIR_IN
                    #action end



                elif alt1 == 2:
                    # interface.g:211:7: OUT
                    pass
                    self.match(self.input, OUT, self.FOLLOW_OUT_in_direction526)

                    #action start
                    direction = interfaceIR.DIR_OUT
                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return direction

    # $ANTLR end "direction"



    # $ANTLR start "number"
    # interface.g:215:1: number returns [value] : ( DEC_NUMBER | HEX_NUMBER );
    def number(self, ):
        value = None


        DEC_NUMBER1 = None
        HEX_NUMBER2 = None

        try:
            try:
                # interface.g:216:5: ( DEC_NUMBER | HEX_NUMBER )
                alt2 = 2
                LA2_0 = self.input.LA(1)

                if (LA2_0 == DEC_NUMBER) :
                    alt2 = 1
                elif (LA2_0 == HEX_NUMBER) :
                    alt2 = 2
                else:
                    nvae = NoViableAltException("", 2, 0, self.input)

                    raise nvae


                if alt2 == 1:
                    # interface.g:216:7: DEC_NUMBER
                    pass
                    DEC_NUMBER1 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_number554)

                    #action start
                    value = int(DEC_NUMBER1.text, 10)
                    #action end



                elif alt2 == 2:
                    # interface.g:217:7: HEX_NUMBER
                    pass
                    HEX_NUMBER2 = self.match(self.input, HEX_NUMBER, self.FOLLOW_HEX_NUMBER_in_number567)

                    #action start
                    value = int(HEX_NUMBER2.text, 16)
                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "number"



    # $ANTLR start "integer"
    # interface.g:220:1: integer returns [value] : ( '+' DEC_NUMBER | '-' DEC_NUMBER | DEC_NUMBER | HEX_NUMBER );
    def integer(self, ):
        value = None


        DEC_NUMBER3 = None
        DEC_NUMBER4 = None
        DEC_NUMBER5 = None
        HEX_NUMBER6 = None

        try:
            try:
                # interface.g:221:5: ( '+' DEC_NUMBER | '-' DEC_NUMBER | DEC_NUMBER | HEX_NUMBER )
                alt3 = 4
                LA3 = self.input.LA(1)
                if LA3 == 32:
                    alt3 = 1
                elif LA3 == 34:
                    alt3 = 2
                elif LA3 == DEC_NUMBER:
                    alt3 = 3
                elif LA3 == HEX_NUMBER:
                    alt3 = 4
                else:
                    nvae = NoViableAltException("", 3, 0, self.input)

                    raise nvae


                if alt3 == 1:
                    # interface.g:221:7: '+' DEC_NUMBER
                    pass
                    self.match(self.input, 32, self.FOLLOW_32_in_integer593)

                    DEC_NUMBER3 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_integer595)

                    #action start
                    value = int(DEC_NUMBER3.text, 10)
                    #action end



                elif alt3 == 2:
                    # interface.g:222:7: '-' DEC_NUMBER
                    pass
                    self.match(self.input, 34, self.FOLLOW_34_in_integer607)

                    DEC_NUMBER4 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_integer609)

                    #action start
                    value = -int(DEC_NUMBER4.text, 10)
                    #action end



                elif alt3 == 3:
                    # interface.g:223:7: DEC_NUMBER
                    pass
                    DEC_NUMBER5 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_integer621)

                    #action start
                    value = int(DEC_NUMBER5.text, 10)
                    #action end



                elif alt3 == 4:
                    # interface.g:224:7: HEX_NUMBER
                    pass
                    HEX_NUMBER6 = self.match(self.input, HEX_NUMBER, self.FOLLOW_HEX_NUMBER_in_integer637)

                    #action start
                    value = int(HEX_NUMBER6.text, 16)
                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "integer"



    # $ANTLR start "simpleExpression"
    # interface.g:227:1: simpleExpression returns [value] : ( integer | IDENTIFIER | SCOPED_IDENTIFIER | '(' sumExpression ')' );
    def simpleExpression(self, ):
        value = None


        IDENTIFIER8 = None
        SCOPED_IDENTIFIER9 = None
        integer7 = None
        sumExpression10 = None

        try:
            try:
                # interface.g:228:5: ( integer | IDENTIFIER | SCOPED_IDENTIFIER | '(' sumExpression ')' )
                alt4 = 4
                LA4 = self.input.LA(1)
                if LA4 == DEC_NUMBER or LA4 == HEX_NUMBER or LA4 == 32 or LA4 == 34:
                    alt4 = 1
                elif LA4 == IDENTIFIER:
                    alt4 = 2
                elif LA4 == SCOPED_IDENTIFIER:
                    alt4 = 3
                elif LA4 == 29:
                    alt4 = 4
                else:
                    nvae = NoViableAltException("", 4, 0, self.input)

                    raise nvae


                if alt4 == 1:
                    # interface.g:228:7: integer
                    pass
                    self._state.following.append(self.FOLLOW_integer_in_simpleExpression666)
                    integer7 = self.integer()

                    self._state.following.pop()

                    #action start
                    value = integer7
                    #action end



                elif alt4 == 2:
                    # interface.g:229:7: IDENTIFIER
                    pass
                    IDENTIFIER8 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_simpleExpression685)

                    #action start
                    value = self.iface.findDefinition(IDENTIFIER8.text).value
                    #action end



                elif alt4 == 3:
                    # interface.g:230:7: SCOPED_IDENTIFIER
                    pass
                    SCOPED_IDENTIFIER9 = self.match(self.input, SCOPED_IDENTIFIER, self.FOLLOW_SCOPED_IDENTIFIER_in_simpleExpression701)

                    #action start
                    value = self.iface.findDefinition(SCOPED_IDENTIFIER9.text).value
                    #action end



                elif alt4 == 4:
                    # interface.g:231:7: '(' sumExpression ')'
                    pass
                    self.match(self.input, 29, self.FOLLOW_29_in_simpleExpression711)

                    self._state.following.append(self.FOLLOW_sumExpression_in_simpleExpression713)
                    sumExpression10 = self.sumExpression()

                    self._state.following.pop()

                    self.match(self.input, 30, self.FOLLOW_30_in_simpleExpression715)

                    #action start
                    value = sumExpression10
                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "simpleExpression"



    # $ANTLR start "productExpression"
    # interface.g:234:1: productExpression returns [value] : initialValue= simpleExpression ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )* ;
    def productExpression(self, ):
        value = None


        initialValue = None
        mulValue = None
        divValue = None

        try:
            try:
                # interface.g:235:5: (initialValue= simpleExpression ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )* )
                # interface.g:235:7: initialValue= simpleExpression ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )*
                pass
                self._state.following.append(self.FOLLOW_simpleExpression_in_productExpression740)
                initialValue = self.simpleExpression()

                self._state.following.pop()

                #action start
                value = initialValue
                #action end


                # interface.g:236:9: ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )*
                while True: #loop5
                    alt5 = 3
                    LA5_0 = self.input.LA(1)

                    if (LA5_0 == 31) :
                        alt5 = 1
                    elif (LA5_0 == 35) :
                        alt5 = 2


                    if alt5 == 1:
                        # interface.g:236:11: '*' mulValue= simpleExpression
                        pass
                        self.match(self.input, 31, self.FOLLOW_31_in_productExpression758)

                        self._state.following.append(self.FOLLOW_simpleExpression_in_productExpression762)
                        mulValue = self.simpleExpression()

                        self._state.following.pop()

                        #action start
                        value *= mulValue
                        #action end



                    elif alt5 == 2:
                        # interface.g:237:11: '/' divValue= simpleExpression
                        pass
                        self.match(self.input, 35, self.FOLLOW_35_in_productExpression776)

                        self._state.following.append(self.FOLLOW_simpleExpression_in_productExpression780)
                        divValue = self.simpleExpression()

                        self._state.following.pop()

                        #action start
                        value /= divValue
                        #action end



                    else:
                        break #loop5





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "productExpression"



    # $ANTLR start "sumExpression"
    # interface.g:241:1: sumExpression returns [value] : initialValue= productExpression ( '+' addValue= productExpression | '-' subValue= productExpression )* ;
    def sumExpression(self, ):
        value = None


        initialValue = None
        addValue = None
        subValue = None

        try:
            try:
                # interface.g:242:5: (initialValue= productExpression ( '+' addValue= productExpression | '-' subValue= productExpression )* )
                # interface.g:242:7: initialValue= productExpression ( '+' addValue= productExpression | '-' subValue= productExpression )*
                pass
                self._state.following.append(self.FOLLOW_productExpression_in_sumExpression816)
                initialValue = self.productExpression()

                self._state.following.pop()

                #action start
                value = initialValue
                #action end


                # interface.g:243:9: ( '+' addValue= productExpression | '-' subValue= productExpression )*
                while True: #loop6
                    alt6 = 3
                    LA6_0 = self.input.LA(1)

                    if (LA6_0 == 32) :
                        alt6 = 1
                    elif (LA6_0 == 34) :
                        alt6 = 2


                    if alt6 == 1:
                        # interface.g:243:11: '+' addValue= productExpression
                        pass
                        self.match(self.input, 32, self.FOLLOW_32_in_sumExpression834)

                        self._state.following.append(self.FOLLOW_productExpression_in_sumExpression838)
                        addValue = self.productExpression()

                        self._state.following.pop()

                        #action start
                        value += addValue
                        #action end



                    elif alt6 == 2:
                        # interface.g:244:11: '-' subValue= productExpression
                        pass
                        self.match(self.input, 34, self.FOLLOW_34_in_sumExpression852)

                        self._state.following.append(self.FOLLOW_productExpression_in_sumExpression856)
                        subValue = self.productExpression()

                        self._state.following.pop()

                        #action start
                        value -= subValue
                        #action end



                    else:
                        break #loop6





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "sumExpression"



    # $ANTLR start "valueExpression"
    # interface.g:248:1: valueExpression returns [value] : ( sumExpression | QUOTED_STRING );
    def valueExpression(self, ):
        value = None


        QUOTED_STRING12 = None
        sumExpression11 = None

        try:
            try:
                # interface.g:249:5: ( sumExpression | QUOTED_STRING )
                alt7 = 2
                LA7_0 = self.input.LA(1)

                if (LA7_0 == DEC_NUMBER or (HEX_NUMBER <= LA7_0 <= IDENTIFIER) or LA7_0 == SCOPED_IDENTIFIER or LA7_0 == 29 or LA7_0 == 32 or LA7_0 == 34) :
                    alt7 = 1
                elif (LA7_0 == QUOTED_STRING) :
                    alt7 = 2
                else:
                    nvae = NoViableAltException("", 7, 0, self.input)

                    raise nvae


                if alt7 == 1:
                    # interface.g:249:7: sumExpression
                    pass
                    self._state.following.append(self.FOLLOW_sumExpression_in_valueExpression890)
                    sumExpression11 = self.sumExpression()

                    self._state.following.pop()

                    #action start
                    value = sumExpression11
                    #action end



                elif alt7 == 2:
                    # interface.g:250:7: QUOTED_STRING
                    pass
                    QUOTED_STRING12 = self.match(self.input, QUOTED_STRING, self.FOLLOW_QUOTED_STRING_in_valueExpression903)

                    #action start
                    value = UnquoteString(QUOTED_STRING12.text)
                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "valueExpression"



    # $ANTLR start "simpleNumber"
    # interface.g:254:1: simpleNumber returns [value] : ( number | IDENTIFIER | SCOPED_IDENTIFIER );
    def simpleNumber(self, ):
        value = None


        IDENTIFIER14 = None
        SCOPED_IDENTIFIER15 = None
        number13 = None

        try:
            try:
                # interface.g:255:5: ( number | IDENTIFIER | SCOPED_IDENTIFIER )
                alt8 = 3
                LA8 = self.input.LA(1)
                if LA8 == DEC_NUMBER or LA8 == HEX_NUMBER:
                    alt8 = 1
                elif LA8 == IDENTIFIER:
                    alt8 = 2
                elif LA8 == SCOPED_IDENTIFIER:
                    alt8 = 3
                else:
                    nvae = NoViableAltException("", 8, 0, self.input)

                    raise nvae


                if alt8 == 1:
                    # interface.g:255:7: number
                    pass
                    self._state.following.append(self.FOLLOW_number_in_simpleNumber931)
                    number13 = self.number()

                    self._state.following.pop()

                    #action start
                    value = number13
                    #action end



                elif alt8 == 2:
                    # interface.g:256:7: IDENTIFIER
                    pass
                    IDENTIFIER14 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_simpleNumber952)

                    #action start
                    value = self.iface.findDefinition(IDENTIFIER14.text).value
                    #action end



                elif alt8 == 3:
                    # interface.g:257:7: SCOPED_IDENTIFIER
                    pass
                    SCOPED_IDENTIFIER15 = self.match(self.input, SCOPED_IDENTIFIER, self.FOLLOW_SCOPED_IDENTIFIER_in_simpleNumber969)

                    #action start
                    value = self.iface.findDefinition(SCOPED_IDENTIFIER15.text).value
                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "simpleNumber"



    # $ANTLR start "arrayExpression"
    # interface.g:263:1: arrayExpression returns [size] : '[' simpleNumber ']' ;
    def arrayExpression(self, ):
        size = None


        simpleNumber16 = None

        try:
            try:
                # interface.g:264:5: ( '[' simpleNumber ']' )
                # interface.g:264:7: '[' simpleNumber ']'
                pass
                self.match(self.input, 37, self.FOLLOW_37_in_arrayExpression995)

                self._state.following.append(self.FOLLOW_simpleNumber_in_arrayExpression997)
                simpleNumber16 = self.simpleNumber()

                self._state.following.pop()

                self.match(self.input, 38, self.FOLLOW_38_in_arrayExpression999)

                #action start
                size = simpleNumber16
                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return size

    # $ANTLR end "arrayExpression"



    # $ANTLR start "typeIdentifier"
    # interface.g:268:1: typeIdentifier returns [typeObj] : ( IDENTIFIER | SCOPED_IDENTIFIER );
    def typeIdentifier(self, ):
        typeObj = None


        IDENTIFIER17 = None
        SCOPED_IDENTIFIER18 = None

        try:
            try:
                # interface.g:269:5: ( IDENTIFIER | SCOPED_IDENTIFIER )
                alt9 = 2
                LA9_0 = self.input.LA(1)

                if (LA9_0 == IDENTIFIER) :
                    alt9 = 1
                elif (LA9_0 == SCOPED_IDENTIFIER) :
                    alt9 = 2
                else:
                    nvae = NoViableAltException("", 9, 0, self.input)

                    raise nvae


                if alt9 == 1:
                    # interface.g:269:7: IDENTIFIER
                    pass
                    IDENTIFIER17 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_typeIdentifier1030)

                    #action start

                    try:
                        typeObj = self.iface.findType(IDENTIFIER17.text)
                    except KeyError, e:
                        self.compileErrors += 1
                        self.emitErrorMessage(self.getErrorHeaderForToken(IDENTIFIER17) +
                                              " Unknown type {}".format(e))
                        typeObj = interfaceIR.ERROR_TYPE

                    #action end



                elif alt9 == 2:
                    # interface.g:279:7: SCOPED_IDENTIFIER
                    pass
                    SCOPED_IDENTIFIER18 = self.match(self.input, SCOPED_IDENTIFIER, self.FOLLOW_SCOPED_IDENTIFIER_in_typeIdentifier1048)

                    #action start

                    try:
                        typeObj = self.iface.findType(SCOPED_IDENTIFIER18.text)
                    except KeyError, e:
                        self.compileErrors += 1
                        self.emitErrorMessage(self.getErrorHeaderForToken(SCOPED_IDENTIFIER18) +
                                              " Unknown type {}".format(e))
                        typeObj = interfaceIR.ERROR_TYPE

                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return typeObj

    # $ANTLR end "typeIdentifier"



    # $ANTLR start "docPostComments"
    # interface.g:291:1: docPostComments returns [comments] : ( DOC_POST_COMMENT )* ;
    def docPostComments(self, ):
        comments = None


        DOC_POST_COMMENT19 = None

        try:
            try:
                # interface.g:292:5: ( ( DOC_POST_COMMENT )* )
                # interface.g:292:7: ( DOC_POST_COMMENT )*
                pass
                #action start
                comments = []
                #action end


                # interface.g:293:7: ( DOC_POST_COMMENT )*
                while True: #loop10
                    alt10 = 2
                    LA10_0 = self.input.LA(1)

                    if (LA10_0 == DOC_POST_COMMENT) :
                        alt10 = 1


                    if alt10 == 1:
                        # interface.g:293:9: DOC_POST_COMMENT
                        pass
                        DOC_POST_COMMENT19 = self.match(self.input, DOC_POST_COMMENT, self.FOLLOW_DOC_POST_COMMENT_in_docPostComments1089)

                        #action start
                        comments.append(StripPostComment(DOC_POST_COMMENT19.text))
                        #action end



                    else:
                        break #loop10





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return comments

    # $ANTLR end "docPostComments"



    # $ANTLR start "docPreComment"
    # interface.g:298:1: docPreComment returns [comment] : DOC_PRE_COMMENT ;
    def docPreComment(self, ):
        comment = None


        DOC_PRE_COMMENT20 = None

        try:
            try:
                # interface.g:299:5: ( DOC_PRE_COMMENT )
                # interface.g:299:7: DOC_PRE_COMMENT
                pass
                DOC_PRE_COMMENT20 = self.match(self.input, DOC_PRE_COMMENT, self.FOLLOW_DOC_PRE_COMMENT_in_docPreComment1133)

                #action start
                comment = StripPreComment(DOC_PRE_COMMENT20.text)
                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return comment

    # $ANTLR end "docPreComment"



    # $ANTLR start "defineDecl"
    # interface.g:302:1: defineDecl returns [define] : DEFINE IDENTIFIER '=' valueExpression ';' ;
    def defineDecl(self, ):
        define = None


        IDENTIFIER21 = None
        valueExpression22 = None

        try:
            try:
                # interface.g:303:5: ( DEFINE IDENTIFIER '=' valueExpression ';' )
                # interface.g:303:7: DEFINE IDENTIFIER '=' valueExpression ';'
                pass
                self.match(self.input, DEFINE, self.FOLLOW_DEFINE_in_defineDecl1156)

                IDENTIFIER21 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_defineDecl1158)

                self.match(self.input, 36, self.FOLLOW_36_in_defineDecl1160)

                self._state.following.append(self.FOLLOW_valueExpression_in_defineDecl1162)
                valueExpression22 = self.valueExpression()

                self._state.following.pop()

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_defineDecl1164)

                #action start

                define = interfaceIR.Definition(IDENTIFIER21.text, valueExpression22)

                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return define

    # $ANTLR end "defineDecl"



    # $ANTLR start "namedValue"
    # interface.g:309:1: namedValue returns [namedValue] : IDENTIFIER ( '=' integer )? docPostComments ;
    def namedValue(self, ):
        namedValue = None


        IDENTIFIER23 = None
        integer24 = None
        docPostComments25 = None

        try:
            try:
                # interface.g:310:5: ( IDENTIFIER ( '=' integer )? docPostComments )
                # interface.g:310:7: IDENTIFIER ( '=' integer )? docPostComments
                pass
                IDENTIFIER23 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_namedValue1195)

                # interface.g:310:18: ( '=' integer )?
                alt11 = 2
                LA11_0 = self.input.LA(1)

                if (LA11_0 == 36) :
                    alt11 = 1
                if alt11 == 1:
                    # interface.g:310:20: '=' integer
                    pass
                    self.match(self.input, 36, self.FOLLOW_36_in_namedValue1199)

                    self._state.following.append(self.FOLLOW_integer_in_namedValue1201)
                    integer24 = self.integer()

                    self._state.following.pop()




                self._state.following.append(self.FOLLOW_docPostComments_in_namedValue1206)
                docPostComments25 = self.docPostComments()

                self._state.following.pop()

                #action start

                namedValue = interfaceIR.NamedValue(IDENTIFIER23.text,
                                                      integer24)
                namedValue.comments = docPostComments25

                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return namedValue

    # $ANTLR end "namedValue"



    # $ANTLR start "namedValueList"
    # interface.g:318:1: namedValueList returns [values] : (| namedValue | namedValue ',' docPostComments rest= namedValueList );
    def namedValueList(self, ):
        values = None


        rest = None
        namedValue26 = None
        namedValue27 = None
        docPostComments28 = None

        try:
            try:
                # interface.g:319:5: (| namedValue | namedValue ',' docPostComments rest= namedValueList )
                alt12 = 3
                alt12 = self.dfa12.predict(self.input)
                if alt12 == 1:
                    # interface.g:319:51:
                    pass
                    #action start
                    values = [ ]
                    #action end



                elif alt12 == 2:
                    # interface.g:320:7: namedValue
                    pass
                    self._state.following.append(self.FOLLOW_namedValue_in_namedValueList1281)
                    namedValue26 = self.namedValue()

                    self._state.following.pop()

                    #action start
                    values = [ namedValue26 ]
                    #action end



                elif alt12 == 3:
                    # interface.g:321:7: namedValue ',' docPostComments rest= namedValueList
                    pass
                    self._state.following.append(self.FOLLOW_namedValue_in_namedValueList1324)
                    namedValue27 = self.namedValue()

                    self._state.following.pop()

                    self.match(self.input, 33, self.FOLLOW_33_in_namedValueList1326)

                    self._state.following.append(self.FOLLOW_docPostComments_in_namedValueList1328)
                    docPostComments28 = self.docPostComments()

                    self._state.following.pop()

                    self._state.following.append(self.FOLLOW_namedValueList_in_namedValueList1332)
                    rest = self.namedValueList()

                    self._state.following.pop()

                    #action start

                    namedValue27.comments.extend(docPostComments28)
                    values = [ namedValue27 ]
                    values.extend(rest)

                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return values

    # $ANTLR end "namedValueList"



    # $ANTLR start "enumDecl"
    # interface.g:329:1: enumDecl returns [enum] : ENUM IDENTIFIER '{' namedValueList '}' ';' ;
    def enumDecl(self, ):
        enum = None


        IDENTIFIER29 = None
        namedValueList30 = None

        try:
            try:
                # interface.g:330:5: ( ENUM IDENTIFIER '{' namedValueList '}' ';' )
                # interface.g:330:7: ENUM IDENTIFIER '{' namedValueList '}' ';'
                pass
                self.match(self.input, ENUM, self.FOLLOW_ENUM_in_enumDecl1363)

                IDENTIFIER29 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_enumDecl1365)

                self.match(self.input, 39, self.FOLLOW_39_in_enumDecl1367)

                self._state.following.append(self.FOLLOW_namedValueList_in_enumDecl1369)
                namedValueList30 = self.namedValueList()

                self._state.following.pop()

                self.match(self.input, 40, self.FOLLOW_40_in_enumDecl1371)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_enumDecl1373)

                #action start

                enum = interfaceIR.EnumType(IDENTIFIER29.text, namedValueList30)

                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return enum

    # $ANTLR end "enumDecl"



    # $ANTLR start "bitmaskDecl"
    # interface.g:336:1: bitmaskDecl returns [bitmask] : BITMASK IDENTIFIER '{' namedValueList '}' ';' ;
    def bitmaskDecl(self, ):
        bitmask = None


        IDENTIFIER31 = None
        namedValueList32 = None

        try:
            try:
                # interface.g:337:5: ( BITMASK IDENTIFIER '{' namedValueList '}' ';' )
                # interface.g:337:7: BITMASK IDENTIFIER '{' namedValueList '}' ';'
                pass
                self.match(self.input, BITMASK, self.FOLLOW_BITMASK_in_bitmaskDecl1404)

                IDENTIFIER31 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_bitmaskDecl1406)

                self.match(self.input, 39, self.FOLLOW_39_in_bitmaskDecl1408)

                self._state.following.append(self.FOLLOW_namedValueList_in_bitmaskDecl1410)
                namedValueList32 = self.namedValueList()

                self._state.following.pop()

                self.match(self.input, 40, self.FOLLOW_40_in_bitmaskDecl1412)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_bitmaskDecl1414)

                #action start

                bitmask = interfaceIR.BitmaskType(IDENTIFIER31.text, namedValueList32)

                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return bitmask

    # $ANTLR end "bitmaskDecl"



    # $ANTLR start "referenceDecl"
    # interface.g:343:1: referenceDecl returns [ref] : REFERENCE IDENTIFIER ';' ;
    def referenceDecl(self, ):
        ref = None


        IDENTIFIER33 = None

        try:
            try:
                # interface.g:344:5: ( REFERENCE IDENTIFIER ';' )
                # interface.g:344:7: REFERENCE IDENTIFIER ';'
                pass
                self.match(self.input, REFERENCE, self.FOLLOW_REFERENCE_in_referenceDecl1445)

                IDENTIFIER33 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_referenceDecl1447)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_referenceDecl1449)

                #action start

                ref = interfaceIR.ReferenceType(IDENTIFIER33.text)

                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return ref

    # $ANTLR end "referenceDecl"



    # $ANTLR start "formalParameter"
    # interface.g:350:1: formalParameter returns [parameter] : typeIdentifier IDENTIFIER ( arrayExpression )? ( direction )? docPostComments ;
    def formalParameter(self, ):
        parameter = None


        IDENTIFIER35 = None
        typeIdentifier34 = None
        arrayExpression36 = None
        direction37 = None
        docPostComments38 = None

        try:
            try:
                # interface.g:351:5: ( typeIdentifier IDENTIFIER ( arrayExpression )? ( direction )? docPostComments )
                # interface.g:351:7: typeIdentifier IDENTIFIER ( arrayExpression )? ( direction )? docPostComments
                pass
                self._state.following.append(self.FOLLOW_typeIdentifier_in_formalParameter1480)
                typeIdentifier34 = self.typeIdentifier()

                self._state.following.pop()

                IDENTIFIER35 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_formalParameter1482)

                # interface.g:351:33: ( arrayExpression )?
                alt13 = 2
                LA13_0 = self.input.LA(1)

                if (LA13_0 == 37) :
                    alt13 = 1
                if alt13 == 1:
                    # interface.g:351:33: arrayExpression
                    pass
                    self._state.following.append(self.FOLLOW_arrayExpression_in_formalParameter1484)
                    arrayExpression36 = self.arrayExpression()

                    self._state.following.pop()




                # interface.g:351:50: ( direction )?
                alt14 = 2
                LA14_0 = self.input.LA(1)

                if (LA14_0 == IN or LA14_0 == OUT) :
                    alt14 = 1
                if alt14 == 1:
                    # interface.g:351:50: direction
                    pass
                    self._state.following.append(self.FOLLOW_direction_in_formalParameter1487)
                    direction37 = self.direction()

                    self._state.following.pop()




                self._state.following.append(self.FOLLOW_docPostComments_in_formalParameter1490)
                docPostComments38 = self.docPostComments()

                self._state.following.pop()

                #action start

                parameter = interfaceIR.MakeParameter(typeIdentifier34,
                                                        IDENTIFIER35.text,
                                                        arrayExpression36,
                                                        direction37)
                parameter.comments = docPostComments38

                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return parameter

    # $ANTLR end "formalParameter"



    # $ANTLR start "formalParameterList"
    # interface.g:361:1: formalParameterList returns [parameters] : ( formalParameter | formalParameter ',' docPostComments rest= formalParameterList );
    def formalParameterList(self, ):
        parameters = None


        rest = None
        formalParameter39 = None
        formalParameter40 = None
        docPostComments41 = None

        try:
            try:
                # interface.g:362:5: ( formalParameter | formalParameter ',' docPostComments rest= formalParameterList )
                alt15 = 2
                alt15 = self.dfa15.predict(self.input)
                if alt15 == 1:
                    # interface.g:362:7: formalParameter
                    pass
                    self._state.following.append(self.FOLLOW_formalParameter_in_formalParameterList1521)
                    formalParameter39 = self.formalParameter()

                    self._state.following.pop()

                    #action start

                    parameters = [ formalParameter39 ]

                    #action end



                elif alt15 == 2:
                    # interface.g:366:7: formalParameter ',' docPostComments rest= formalParameterList
                    pass
                    self._state.following.append(self.FOLLOW_formalParameter_in_formalParameterList1539)
                    formalParameter40 = self.formalParameter()

                    self._state.following.pop()

                    self.match(self.input, 33, self.FOLLOW_33_in_formalParameterList1541)

                    self._state.following.append(self.FOLLOW_docPostComments_in_formalParameterList1543)
                    docPostComments41 = self.docPostComments()

                    self._state.following.pop()

                    self._state.following.append(self.FOLLOW_formalParameterList_in_formalParameterList1547)
                    rest = self.formalParameterList()

                    self._state.following.pop()

                    #action start

                    formalParameter40.comments.extend(docPostComments41)
                    parameters = [ formalParameter40 ]
                    parameters.extend(rest)

                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return parameters

    # $ANTLR end "formalParameterList"



    # $ANTLR start "functionDecl"
    # interface.g:374:1: functionDecl returns [function] : FUNCTION ( typeIdentifier )? IDENTIFIER '(' ( formalParameterList )? ')' ';' ;
    def functionDecl(self, ):
        function = None


        IDENTIFIER44 = None
        formalParameterList42 = None
        typeIdentifier43 = None

        try:
            try:
                # interface.g:375:5: ( FUNCTION ( typeIdentifier )? IDENTIFIER '(' ( formalParameterList )? ')' ';' )
                # interface.g:375:7: FUNCTION ( typeIdentifier )? IDENTIFIER '(' ( formalParameterList )? ')' ';'
                pass
                self.match(self.input, FUNCTION, self.FOLLOW_FUNCTION_in_functionDecl1578)

                # interface.g:375:16: ( typeIdentifier )?
                alt16 = 2
                LA16_0 = self.input.LA(1)

                if (LA16_0 == IDENTIFIER) :
                    LA16_1 = self.input.LA(2)

                    if (LA16_1 == IDENTIFIER) :
                        alt16 = 1
                elif (LA16_0 == SCOPED_IDENTIFIER) :
                    alt16 = 1
                if alt16 == 1:
                    # interface.g:375:16: typeIdentifier
                    pass
                    self._state.following.append(self.FOLLOW_typeIdentifier_in_functionDecl1580)
                    typeIdentifier43 = self.typeIdentifier()

                    self._state.following.pop()




                IDENTIFIER44 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_functionDecl1583)

                self.match(self.input, 29, self.FOLLOW_29_in_functionDecl1585)

                # interface.g:375:47: ( formalParameterList )?
                alt17 = 2
                LA17_0 = self.input.LA(1)

                if (LA17_0 == IDENTIFIER or LA17_0 == SCOPED_IDENTIFIER) :
                    alt17 = 1
                if alt17 == 1:
                    # interface.g:375:47: formalParameterList
                    pass
                    self._state.following.append(self.FOLLOW_formalParameterList_in_functionDecl1587)
                    formalParameterList42 = self.formalParameterList()

                    self._state.following.pop()




                self.match(self.input, 30, self.FOLLOW_30_in_functionDecl1590)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_functionDecl1592)

                #action start

                if formalParameterList42 == None:
                    parameterList = []
                else:
                    parameterList = formalParameterList42
                function = interfaceIR.Function(typeIdentifier43,
                                                  IDENTIFIER44.text,
                                                  parameterList)

                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return function

    # $ANTLR end "functionDecl"



    # $ANTLR start "handlerDecl"
    # interface.g:387:1: handlerDecl returns [handler] : HANDLER IDENTIFIER '(' ( formalParameterList )? ')' ';' ;
    def handlerDecl(self, ):
        handler = None


        IDENTIFIER46 = None
        formalParameterList45 = None

        try:
            try:
                # interface.g:388:5: ( HANDLER IDENTIFIER '(' ( formalParameterList )? ')' ';' )
                # interface.g:388:7: HANDLER IDENTIFIER '(' ( formalParameterList )? ')' ';'
                pass
                self.match(self.input, HANDLER, self.FOLLOW_HANDLER_in_handlerDecl1623)

                IDENTIFIER46 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_handlerDecl1625)

                self.match(self.input, 29, self.FOLLOW_29_in_handlerDecl1627)

                # interface.g:388:30: ( formalParameterList )?
                alt18 = 2
                LA18_0 = self.input.LA(1)

                if (LA18_0 == IDENTIFIER or LA18_0 == SCOPED_IDENTIFIER) :
                    alt18 = 1
                if alt18 == 1:
                    # interface.g:388:30: formalParameterList
                    pass
                    self._state.following.append(self.FOLLOW_formalParameterList_in_handlerDecl1629)
                    formalParameterList45 = self.formalParameterList()

                    self._state.following.pop()




                self.match(self.input, 30, self.FOLLOW_30_in_handlerDecl1632)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_handlerDecl1634)

                #action start

                if formalParameterList45 == None:
                    parameterList = []
                else:
                    parameterList = formalParameterList45
                handler = interfaceIR.HandlerType(IDENTIFIER46.text,
                                                    parameterList)

                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return handler

    # $ANTLR end "handlerDecl"



    # $ANTLR start "eventDecl"
    # interface.g:399:1: eventDecl returns [event] : EVENT IDENTIFIER '(' ( formalParameterList )? ')' ';' ;
    def eventDecl(self, ):
        event = None


        IDENTIFIER48 = None
        formalParameterList47 = None

        try:
            try:
                # interface.g:400:5: ( EVENT IDENTIFIER '(' ( formalParameterList )? ')' ';' )
                # interface.g:400:7: EVENT IDENTIFIER '(' ( formalParameterList )? ')' ';'
                pass
                self.match(self.input, EVENT, self.FOLLOW_EVENT_in_eventDecl1665)

                IDENTIFIER48 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_eventDecl1667)

                self.match(self.input, 29, self.FOLLOW_29_in_eventDecl1669)

                # interface.g:400:28: ( formalParameterList )?
                alt19 = 2
                LA19_0 = self.input.LA(1)

                if (LA19_0 == IDENTIFIER or LA19_0 == SCOPED_IDENTIFIER) :
                    alt19 = 1
                if alt19 == 1:
                    # interface.g:400:28: formalParameterList
                    pass
                    self._state.following.append(self.FOLLOW_formalParameterList_in_eventDecl1671)
                    formalParameterList47 = self.formalParameterList()

                    self._state.following.pop()




                self.match(self.input, 30, self.FOLLOW_30_in_eventDecl1674)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_eventDecl1676)

                #action start

                if formalParameterList47 == None:
                    parameterList = []
                else:
                    parameterList = formalParameterList47
                event = interfaceIR.Event(IDENTIFIER48.text,
                                            parameterList)

                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return event

    # $ANTLR end "eventDecl"



    # $ANTLR start "declaration"
    # interface.g:411:1: declaration returns [declaration] : ( enumDecl | bitmaskDecl | referenceDecl | functionDecl | handlerDecl | eventDecl | defineDecl );
    def declaration(self, ):
        declaration = None


        enumDecl49 = None
        bitmaskDecl50 = None
        referenceDecl51 = None
        functionDecl52 = None
        handlerDecl53 = None
        eventDecl54 = None
        defineDecl55 = None

        try:
            try:
                # interface.g:412:5: ( enumDecl | bitmaskDecl | referenceDecl | functionDecl | handlerDecl | eventDecl | defineDecl )
                alt20 = 7
                LA20 = self.input.LA(1)
                if LA20 == ENUM:
                    alt20 = 1
                elif LA20 == BITMASK:
                    alt20 = 2
                elif LA20 == REFERENCE:
                    alt20 = 3
                elif LA20 == FUNCTION:
                    alt20 = 4
                elif LA20 == HANDLER:
                    alt20 = 5
                elif LA20 == EVENT:
                    alt20 = 6
                elif LA20 == DEFINE:
                    alt20 = 7
                else:
                    nvae = NoViableAltException("", 20, 0, self.input)

                    raise nvae


                if alt20 == 1:
                    # interface.g:412:7: enumDecl
                    pass
                    self._state.following.append(self.FOLLOW_enumDecl_in_declaration1707)
                    enumDecl49 = self.enumDecl()

                    self._state.following.pop()

                    #action start
                    declaration = enumDecl49
                    #action end



                elif alt20 == 2:
                    # interface.g:413:7: bitmaskDecl
                    pass
                    self._state.following.append(self.FOLLOW_bitmaskDecl_in_declaration1724)
                    bitmaskDecl50 = self.bitmaskDecl()

                    self._state.following.pop()

                    #action start
                    declaration = bitmaskDecl50
                    #action end



                elif alt20 == 3:
                    # interface.g:414:7: referenceDecl
                    pass
                    self._state.following.append(self.FOLLOW_referenceDecl_in_declaration1738)
                    referenceDecl51 = self.referenceDecl()

                    self._state.following.pop()

                    #action start
                    declaration = referenceDecl51
                    #action end



                elif alt20 == 4:
                    # interface.g:415:7: functionDecl
                    pass
                    self._state.following.append(self.FOLLOW_functionDecl_in_declaration1750)
                    functionDecl52 = self.functionDecl()

                    self._state.following.pop()

                    #action start
                    declaration = functionDecl52
                    #action end



                elif alt20 == 5:
                    # interface.g:416:7: handlerDecl
                    pass
                    self._state.following.append(self.FOLLOW_handlerDecl_in_declaration1763)
                    handlerDecl53 = self.handlerDecl()

                    self._state.following.pop()

                    #action start
                    declaration = handlerDecl53
                    #action end



                elif alt20 == 6:
                    # interface.g:417:7: eventDecl
                    pass
                    self._state.following.append(self.FOLLOW_eventDecl_in_declaration1777)
                    eventDecl54 = self.eventDecl()

                    self._state.following.pop()

                    #action start
                    declaration = eventDecl54
                    #action end



                elif alt20 == 7:
                    # interface.g:418:7: defineDecl
                    pass
                    self._state.following.append(self.FOLLOW_defineDecl_in_declaration1793)
                    defineDecl55 = self.defineDecl()

                    self._state.following.pop()

                    #action start
                    declaration = defineDecl55
                    #action end




            except RecognitionException, re:

                # Report recognition exceptions here with default handling.  Do not want them to get caught
                # by generic handler below
                self.reportError(re)
                self.recover(self.input, re)


            except Exception, e:

                self.emitErrorMessage(self.getErrorHeaderForToken(self.getCurrentInputSymbol(self.input))
                                      + " " + str(e))
                self.compileErrors += 1



        finally:
            pass
        return declaration

    # $ANTLR end "declaration"



    # $ANTLR start "documentedDeclaration"
    # interface.g:434:1: documentedDeclaration returns [declaration] : docPreComment declaration ;
    def documentedDeclaration(self, ):
        declaration = None


        declaration56 = None
        docPreComment57 = None

        try:
            try:
                # interface.g:435:5: ( docPreComment declaration )
                # interface.g:435:7: docPreComment declaration
                pass
                self._state.following.append(self.FOLLOW_docPreComment_in_documentedDeclaration1833)
                docPreComment57 = self.docPreComment()

                self._state.following.pop()

                self._state.following.append(self.FOLLOW_declaration_in_documentedDeclaration1835)
                declaration56 = self.declaration()

                self._state.following.pop()

                #action start

                declaration = declaration56
                if declaration:
                    declaration.comment = docPreComment57

                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return declaration

    # $ANTLR end "documentedDeclaration"



    # $ANTLR start "filename"
    # interface.g:445:1: filename returns [filename] : ( IDENTIFIER | SCOPED_IDENTIFIER );
    def filename(self, ):
        filename = None


        IDENTIFIER58 = None
        SCOPED_IDENTIFIER59 = None

        try:
            try:
                # interface.g:446:5: ( IDENTIFIER | SCOPED_IDENTIFIER )
                alt21 = 2
                LA21_0 = self.input.LA(1)

                if (LA21_0 == IDENTIFIER) :
                    alt21 = 1
                elif (LA21_0 == SCOPED_IDENTIFIER) :
                    alt21 = 2
                else:
                    nvae = NoViableAltException("", 21, 0, self.input)

                    raise nvae


                if alt21 == 1:
                    # interface.g:446:7: IDENTIFIER
                    pass
                    IDENTIFIER58 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_filename1868)

                    #action start
                    filename = IDENTIFIER58.text
                    #action end



                elif alt21 == 2:
                    # interface.g:447:7: SCOPED_IDENTIFIER
                    pass
                    SCOPED_IDENTIFIER59 = self.match(self.input, SCOPED_IDENTIFIER, self.FOLLOW_SCOPED_IDENTIFIER_in_filename1886)

                    #action start
                    filename = SCOPED_IDENTIFIER59.text
                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return filename

    # $ANTLR end "filename"



    # $ANTLR start "usetypesStmt"
    # interface.g:450:1: usetypesStmt : USETYPES filename ';' ;
    def usetypesStmt(self, ):
        filename60 = None

        try:
            try:
                # interface.g:451:5: ( USETYPES filename ';' )
                # interface.g:451:7: USETYPES filename ';'
                pass
                self.match(self.input, USETYPES, self.FOLLOW_USETYPES_in_usetypesStmt1906)

                self._state.following.append(self.FOLLOW_filename_in_usetypesStmt1908)
                filename60 = self.filename()

                self._state.following.pop()

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_usetypesStmt1910)

                #action start

                if filename60.endswith(".api"):
                    basename=filename60[:-4]
                    fullFilename=filename60
                else:
                    basename=filename60
                    fullFilename=filename60 + ".api"

                self.iface.imports[basename] = ParseCode(fullFilename, self.searchPath)

                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return

    # $ANTLR end "usetypesStmt"



    # $ANTLR start "apiDocument"
    # interface.g:467:1: apiDocument returns [iface] : ( ( docPreComment )+ ( usetypesStmt |firstDecl= documentedDeclaration ) )? ( usetypesStmt | declaration |laterDecl= documentedDeclaration )* EOF ;
    def apiDocument(self, ):
        iface = None


        firstDecl = None
        laterDecl = None
        docPreComment61 = None
        declaration62 = None

        try:
            try:
                # interface.g:468:5: ( ( ( docPreComment )+ ( usetypesStmt |firstDecl= documentedDeclaration ) )? ( usetypesStmt | declaration |laterDecl= documentedDeclaration )* EOF )
                # interface.g:468:7: ( ( docPreComment )+ ( usetypesStmt |firstDecl= documentedDeclaration ) )? ( usetypesStmt | declaration |laterDecl= documentedDeclaration )* EOF
                pass
                # interface.g:468:7: ( ( docPreComment )+ ( usetypesStmt |firstDecl= documentedDeclaration ) )?
                alt24 = 2
                LA24_0 = self.input.LA(1)

                if (LA24_0 == DOC_PRE_COMMENT) :
                    LA24_1 = self.input.LA(2)

                    if (LA24_1 == DOC_PRE_COMMENT or LA24_1 == USETYPES) :
                        alt24 = 1
                if alt24 == 1:
                    # interface.g:468:9: ( docPreComment )+ ( usetypesStmt |firstDecl= documentedDeclaration )
                    pass
                    # interface.g:468:9: ( docPreComment )+
                    cnt22 = 0
                    while True: #loop22
                        alt22 = 2
                        LA22_0 = self.input.LA(1)

                        if (LA22_0 == DOC_PRE_COMMENT) :
                            LA22_2 = self.input.LA(2)

                            if (LA22_2 == DOC_PRE_COMMENT or LA22_2 == USETYPES) :
                                alt22 = 1




                        if alt22 == 1:
                            # interface.g:468:11: docPreComment
                            pass
                            self._state.following.append(self.FOLLOW_docPreComment_in_apiDocument1948)
                            docPreComment61 = self.docPreComment()

                            self._state.following.pop()

                            #action start
                            self.iface.comments.append(docPreComment61)
                            #action end



                        else:
                            if cnt22 >= 1:
                                break #loop22

                            eee = EarlyExitException(22, self.input)
                            raise eee

                        cnt22 += 1


                    # interface.g:469:13: ( usetypesStmt |firstDecl= documentedDeclaration )
                    alt23 = 2
                    LA23_0 = self.input.LA(1)

                    if (LA23_0 == USETYPES) :
                        alt23 = 1
                    elif (LA23_0 == DOC_PRE_COMMENT) :
                        alt23 = 2
                    else:
                        nvae = NoViableAltException("", 23, 0, self.input)

                        raise nvae


                    if alt23 == 1:
                        # interface.g:469:16: usetypesStmt
                        pass
                        self._state.following.append(self.FOLLOW_usetypesStmt_in_apiDocument1970)
                        self.usetypesStmt()

                        self._state.following.pop()


                    elif alt23 == 2:
                        # interface.g:470:15: firstDecl= documentedDeclaration
                        pass
                        self._state.following.append(self.FOLLOW_documentedDeclaration_in_apiDocument1988)
                        firstDecl = self.documentedDeclaration()

                        self._state.following.pop()

                        #action start

                        if firstDecl:
                            self.iface.addDeclaration(firstDecl)

                        #action end








                # interface.g:476:7: ( usetypesStmt | declaration |laterDecl= documentedDeclaration )*
                while True: #loop25
                    alt25 = 4
                    LA25 = self.input.LA(1)
                    if LA25 == USETYPES:
                        alt25 = 1
                    elif LA25 == BITMASK or LA25 == DEFINE or LA25 == ENUM or LA25 == EVENT or LA25 == FUNCTION or LA25 == HANDLER or LA25 == REFERENCE:
                        alt25 = 2
                    elif LA25 == DOC_PRE_COMMENT:
                        alt25 = 3

                    if alt25 == 1:
                        # interface.g:476:9: usetypesStmt
                        pass
                        self._state.following.append(self.FOLLOW_usetypesStmt_in_apiDocument2027)
                        self.usetypesStmt()

                        self._state.following.pop()


                    elif alt25 == 2:
                        # interface.g:477:11: declaration
                        pass
                        self._state.following.append(self.FOLLOW_declaration_in_apiDocument2039)
                        declaration62 = self.declaration()

                        self._state.following.pop()

                        #action start

                        if declaration62:
                             self.iface.addDeclaration(declaration62)

                        #action end



                    elif alt25 == 3:
                        # interface.g:482:11: laterDecl= documentedDeclaration
                        pass
                        self._state.following.append(self.FOLLOW_documentedDeclaration_in_apiDocument2068)
                        laterDecl = self.documentedDeclaration()

                        self._state.following.pop()

                        #action start

                        if laterDecl:
                             self.iface.addDeclaration(laterDecl)

                        #action end



                    else:
                        break #loop25


                self.match(self.input, EOF, self.FOLLOW_EOF_in_apiDocument2094)

                #action start
                iface = self.iface
                #action end





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return iface

    # $ANTLR end "apiDocument"



    # lookup tables for DFA #12

    DFA12_eot = DFA.unpack(
        u"\15\uffff"
        )

    DFA12_eof = DFA.unpack(
        u"\15\uffff"
        )

    DFA12_min = DFA.unpack(
        u"\1\23\1\uffff\1\13\1\11\1\13\2\uffff\2\11\4\13"
        )

    DFA12_max = DFA.unpack(
        u"\1\50\1\uffff\1\50\1\42\1\50\2\uffff\2\11\4\50"
        )

    DFA12_accept = DFA.unpack(
        u"\1\uffff\1\1\3\uffff\1\2\1\3\6\uffff"
        )

    DFA12_special = DFA.unpack(
        u"\15\uffff"
        )


    DFA12_transition = [
        DFA.unpack(u"\1\2\24\uffff\1\1"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\4\25\uffff\1\6\2\uffff\1\3\3\uffff\1\5"),
        DFA.unpack(u"\1\11\10\uffff\1\12\15\uffff\1\7\1\uffff\1\10"),
        DFA.unpack(u"\1\4\25\uffff\1\6\6\uffff\1\5"),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u"\1\13"),
        DFA.unpack(u"\1\14"),
        DFA.unpack(u"\1\4\25\uffff\1\6\6\uffff\1\5"),
        DFA.unpack(u"\1\4\25\uffff\1\6\6\uffff\1\5"),
        DFA.unpack(u"\1\4\25\uffff\1\6\6\uffff\1\5"),
        DFA.unpack(u"\1\4\25\uffff\1\6\6\uffff\1\5")
    ]

    # class definition for DFA #12

    class DFA12(DFA):
        pass


    # lookup tables for DFA #15

    DFA15_eot = DFA.unpack(
        u"\17\uffff"
        )

    DFA15_eof = DFA.unpack(
        u"\17\uffff"
        )

    DFA15_min = DFA.unpack(
        u"\3\23\1\13\1\11\3\13\2\uffff\4\46\1\13"
        )

    DFA15_max = DFA.unpack(
        u"\1\31\2\23\1\45\1\31\3\41\2\uffff\4\46\1\41"
        )

    DFA15_accept = DFA.unpack(
        u"\10\uffff\1\1\1\2\5\uffff"
        )

    DFA15_special = DFA.unpack(
        u"\17\uffff"
        )


    DFA15_transition = [
        DFA.unpack(u"\1\1\5\uffff\1\2"),
        DFA.unpack(u"\1\3"),
        DFA.unpack(u"\1\3"),
        DFA.unpack(u"\1\7\10\uffff\1\5\1\uffff\1\6\7\uffff\1\10\2\uffff"
        u"\1\11\3\uffff\1\4"),
        DFA.unpack(u"\1\12\10\uffff\1\13\1\14\5\uffff\1\15"),
        DFA.unpack(u"\1\7\22\uffff\1\10\2\uffff\1\11"),
        DFA.unpack(u"\1\7\22\uffff\1\10\2\uffff\1\11"),
        DFA.unpack(u"\1\7\22\uffff\1\10\2\uffff\1\11"),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u"\1\16"),
        DFA.unpack(u"\1\16"),
        DFA.unpack(u"\1\16"),
        DFA.unpack(u"\1\16"),
        DFA.unpack(u"\1\7\10\uffff\1\5\1\uffff\1\6\7\uffff\1\10\2\uffff"
        u"\1\11")
    ]

    # class definition for DFA #15

    class DFA15(DFA):
        pass




    FOLLOW_IN_in_direction512 = frozenset([1])
    FOLLOW_OUT_in_direction526 = frozenset([1])
    FOLLOW_DEC_NUMBER_in_number554 = frozenset([1])
    FOLLOW_HEX_NUMBER_in_number567 = frozenset([1])
    FOLLOW_32_in_integer593 = frozenset([9])
    FOLLOW_DEC_NUMBER_in_integer595 = frozenset([1])
    FOLLOW_34_in_integer607 = frozenset([9])
    FOLLOW_DEC_NUMBER_in_integer609 = frozenset([1])
    FOLLOW_DEC_NUMBER_in_integer621 = frozenset([1])
    FOLLOW_HEX_NUMBER_in_integer637 = frozenset([1])
    FOLLOW_integer_in_simpleExpression666 = frozenset([1])
    FOLLOW_IDENTIFIER_in_simpleExpression685 = frozenset([1])
    FOLLOW_SCOPED_IDENTIFIER_in_simpleExpression701 = frozenset([1])
    FOLLOW_29_in_simpleExpression711 = frozenset([9, 18, 19, 25, 29, 32, 34])
    FOLLOW_sumExpression_in_simpleExpression713 = frozenset([30])
    FOLLOW_30_in_simpleExpression715 = frozenset([1])
    FOLLOW_simpleExpression_in_productExpression740 = frozenset([1, 31, 35])
    FOLLOW_31_in_productExpression758 = frozenset([9, 18, 19, 25, 29, 32, 34])
    FOLLOW_simpleExpression_in_productExpression762 = frozenset([1, 31, 35])
    FOLLOW_35_in_productExpression776 = frozenset([9, 18, 19, 25, 29, 32, 34])
    FOLLOW_simpleExpression_in_productExpression780 = frozenset([1, 31, 35])
    FOLLOW_productExpression_in_sumExpression816 = frozenset([1, 32, 34])
    FOLLOW_32_in_sumExpression834 = frozenset([9, 18, 19, 25, 29, 32, 34])
    FOLLOW_productExpression_in_sumExpression838 = frozenset([1, 32, 34])
    FOLLOW_34_in_sumExpression852 = frozenset([9, 18, 19, 25, 29, 32, 34])
    FOLLOW_productExpression_in_sumExpression856 = frozenset([1, 32, 34])
    FOLLOW_sumExpression_in_valueExpression890 = frozenset([1])
    FOLLOW_QUOTED_STRING_in_valueExpression903 = frozenset([1])
    FOLLOW_number_in_simpleNumber931 = frozenset([1])
    FOLLOW_IDENTIFIER_in_simpleNumber952 = frozenset([1])
    FOLLOW_SCOPED_IDENTIFIER_in_simpleNumber969 = frozenset([1])
    FOLLOW_37_in_arrayExpression995 = frozenset([9, 18, 19, 25])
    FOLLOW_simpleNumber_in_arrayExpression997 = frozenset([38])
    FOLLOW_38_in_arrayExpression999 = frozenset([1])
    FOLLOW_IDENTIFIER_in_typeIdentifier1030 = frozenset([1])
    FOLLOW_SCOPED_IDENTIFIER_in_typeIdentifier1048 = frozenset([1])
    FOLLOW_DOC_POST_COMMENT_in_docPostComments1089 = frozenset([1, 11])
    FOLLOW_DOC_PRE_COMMENT_in_docPreComment1133 = frozenset([1])
    FOLLOW_DEFINE_in_defineDecl1156 = frozenset([19])
    FOLLOW_IDENTIFIER_in_defineDecl1158 = frozenset([36])
    FOLLOW_36_in_defineDecl1160 = frozenset([9, 18, 19, 23, 25, 29, 32, 34])
    FOLLOW_valueExpression_in_defineDecl1162 = frozenset([26])
    FOLLOW_SEMICOLON_in_defineDecl1164 = frozenset([1])
    FOLLOW_IDENTIFIER_in_namedValue1195 = frozenset([11, 36])
    FOLLOW_36_in_namedValue1199 = frozenset([9, 18, 32, 34])
    FOLLOW_integer_in_namedValue1201 = frozenset([11])
    FOLLOW_docPostComments_in_namedValue1206 = frozenset([1])
    FOLLOW_namedValue_in_namedValueList1281 = frozenset([1])
    FOLLOW_namedValue_in_namedValueList1324 = frozenset([33])
    FOLLOW_33_in_namedValueList1326 = frozenset([11, 19])
    FOLLOW_docPostComments_in_namedValueList1328 = frozenset([19])
    FOLLOW_namedValueList_in_namedValueList1332 = frozenset([1])
    FOLLOW_ENUM_in_enumDecl1363 = frozenset([19])
    FOLLOW_IDENTIFIER_in_enumDecl1365 = frozenset([39])
    FOLLOW_39_in_enumDecl1367 = frozenset([19, 40])
    FOLLOW_namedValueList_in_enumDecl1369 = frozenset([40])
    FOLLOW_40_in_enumDecl1371 = frozenset([26])
    FOLLOW_SEMICOLON_in_enumDecl1373 = frozenset([1])
    FOLLOW_BITMASK_in_bitmaskDecl1404 = frozenset([19])
    FOLLOW_IDENTIFIER_in_bitmaskDecl1406 = frozenset([39])
    FOLLOW_39_in_bitmaskDecl1408 = frozenset([19, 40])
    FOLLOW_namedValueList_in_bitmaskDecl1410 = frozenset([40])
    FOLLOW_40_in_bitmaskDecl1412 = frozenset([26])
    FOLLOW_SEMICOLON_in_bitmaskDecl1414 = frozenset([1])
    FOLLOW_REFERENCE_in_referenceDecl1445 = frozenset([19])
    FOLLOW_IDENTIFIER_in_referenceDecl1447 = frozenset([26])
    FOLLOW_SEMICOLON_in_referenceDecl1449 = frozenset([1])
    FOLLOW_typeIdentifier_in_formalParameter1480 = frozenset([19])
    FOLLOW_IDENTIFIER_in_formalParameter1482 = frozenset([11, 20, 22, 37])
    FOLLOW_arrayExpression_in_formalParameter1484 = frozenset([11, 20, 22])
    FOLLOW_direction_in_formalParameter1487 = frozenset([11])
    FOLLOW_docPostComments_in_formalParameter1490 = frozenset([1])
    FOLLOW_formalParameter_in_formalParameterList1521 = frozenset([1])
    FOLLOW_formalParameter_in_formalParameterList1539 = frozenset([33])
    FOLLOW_33_in_formalParameterList1541 = frozenset([11, 19, 25])
    FOLLOW_docPostComments_in_formalParameterList1543 = frozenset([19, 25])
    FOLLOW_formalParameterList_in_formalParameterList1547 = frozenset([1])
    FOLLOW_FUNCTION_in_functionDecl1578 = frozenset([19, 25])
    FOLLOW_typeIdentifier_in_functionDecl1580 = frozenset([19])
    FOLLOW_IDENTIFIER_in_functionDecl1583 = frozenset([29])
    FOLLOW_29_in_functionDecl1585 = frozenset([19, 25, 30])
    FOLLOW_formalParameterList_in_functionDecl1587 = frozenset([30])
    FOLLOW_30_in_functionDecl1590 = frozenset([26])
    FOLLOW_SEMICOLON_in_functionDecl1592 = frozenset([1])
    FOLLOW_HANDLER_in_handlerDecl1623 = frozenset([19])
    FOLLOW_IDENTIFIER_in_handlerDecl1625 = frozenset([29])
    FOLLOW_29_in_handlerDecl1627 = frozenset([19, 25, 30])
    FOLLOW_formalParameterList_in_handlerDecl1629 = frozenset([30])
    FOLLOW_30_in_handlerDecl1632 = frozenset([26])
    FOLLOW_SEMICOLON_in_handlerDecl1634 = frozenset([1])
    FOLLOW_EVENT_in_eventDecl1665 = frozenset([19])
    FOLLOW_IDENTIFIER_in_eventDecl1667 = frozenset([29])
    FOLLOW_29_in_eventDecl1669 = frozenset([19, 25, 30])
    FOLLOW_formalParameterList_in_eventDecl1671 = frozenset([30])
    FOLLOW_30_in_eventDecl1674 = frozenset([26])
    FOLLOW_SEMICOLON_in_eventDecl1676 = frozenset([1])
    FOLLOW_enumDecl_in_declaration1707 = frozenset([1])
    FOLLOW_bitmaskDecl_in_declaration1724 = frozenset([1])
    FOLLOW_referenceDecl_in_declaration1738 = frozenset([1])
    FOLLOW_functionDecl_in_declaration1750 = frozenset([1])
    FOLLOW_handlerDecl_in_declaration1763 = frozenset([1])
    FOLLOW_eventDecl_in_declaration1777 = frozenset([1])
    FOLLOW_defineDecl_in_declaration1793 = frozenset([1])
    FOLLOW_docPreComment_in_documentedDeclaration1833 = frozenset([6, 10, 13, 14, 15, 16, 24])
    FOLLOW_declaration_in_documentedDeclaration1835 = frozenset([1])
    FOLLOW_IDENTIFIER_in_filename1868 = frozenset([1])
    FOLLOW_SCOPED_IDENTIFIER_in_filename1886 = frozenset([1])
    FOLLOW_USETYPES_in_usetypesStmt1906 = frozenset([19, 25])
    FOLLOW_filename_in_usetypesStmt1908 = frozenset([26])
    FOLLOW_SEMICOLON_in_usetypesStmt1910 = frozenset([1])
    FOLLOW_docPreComment_in_apiDocument1948 = frozenset([12, 27])
    FOLLOW_usetypesStmt_in_apiDocument1970 = frozenset([6, 10, 12, 13, 14, 15, 16, 24, 27])
    FOLLOW_documentedDeclaration_in_apiDocument1988 = frozenset([6, 10, 12, 13, 14, 15, 16, 24, 27])
    FOLLOW_usetypesStmt_in_apiDocument2027 = frozenset([6, 10, 12, 13, 14, 15, 16, 24, 27])
    FOLLOW_declaration_in_apiDocument2039 = frozenset([6, 10, 12, 13, 14, 15, 16, 24, 27])
    FOLLOW_documentedDeclaration_in_apiDocument2068 = frozenset([6, 10, 12, 13, 14, 15, 16, 24, 27])
    FOLLOW_EOF_in_apiDocument2094 = frozenset([1])



def main(argv, stdin=sys.stdin, stdout=sys.stdout, stderr=sys.stderr):
    from antlr3.main import ParserMain
    main = ParserMain("interfaceLexer", interfaceParser)

    main.stdin = stdin
    main.stdout = stdout
    main.stderr = stderr
    main.execute(argv)



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
    if ifaceName == None:
        parser.iface.name = os.path.splitext(os.path.basename(apiPath))[0]
    else:
        parser.iface.name = ifaceName
    parser.iface.path = apiPath

    iface = parser.apiDocument()

    if (parser.getNumberOfSyntaxErrors() > 0 or
        parser.compileErrors > 0):
        return None

    iface.text= ''.join([ token.text
                          for token in tokens.getTokens()
                          if token.type not in frozenset([ C_COMMENT,
                                                           CPP_COMMENT,
                                                           DOC_PRE_COMMENT,
                                                           DOC_POST_COMMENT ]) ])

    return iface


if __name__ == '__main__':
    main(sys.argv)
