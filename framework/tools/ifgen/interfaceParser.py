# $ANTLR 3.5.2 interface.g 2017-04-11 10:25:53

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

        self.dfa13 = self.DFA13(
            self, 13,
            eot = self.DFA13_eot,
            eof = self.DFA13_eof,
            min = self.DFA13_min,
            max = self.DFA13_max,
            accept = self.DFA13_accept,
            special = self.DFA13_special,
            transition = self.DFA13_transition
            )

        self.dfa16 = self.DFA16(
            self, 16,
            eot = self.DFA16_eot,
            eof = self.DFA16_eof,
            min = self.DFA16_min,
            max = self.DFA16_max,
            accept = self.DFA16_accept,
            special = self.DFA16_special,
            transition = self.DFA16_transition
            )




        self.iface = interfaceIR.Interface()
        self.searchPath = []
        self.compileErrors = 0


        self.delegates = []





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



    # $ANTLR start "direction"
    # interface.g:211:1: direction returns [direction] : ( IN | OUT );
    def direction(self, ):
        direction = None


        try:
            try:
                # interface.g:212:5: ( IN | OUT )
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
                    # interface.g:212:7: IN
                    pass 
                    self.match(self.input, IN, self.FOLLOW_IN_in_direction512)

                    #action start
                    direction = interfaceIR.DIR_IN 
                    #action end



                elif alt1 == 2:
                    # interface.g:213:7: OUT
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
    # interface.g:217:1: number returns [value] : ( DEC_NUMBER | HEX_NUMBER );
    def number(self, ):
        value = None


        DEC_NUMBER1 = None
        HEX_NUMBER2 = None

        try:
            try:
                # interface.g:218:5: ( DEC_NUMBER | HEX_NUMBER )
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
                    # interface.g:218:7: DEC_NUMBER
                    pass 
                    DEC_NUMBER1 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_number554)

                    #action start
                    value = int(DEC_NUMBER1.text, 10) 
                    #action end



                elif alt2 == 2:
                    # interface.g:219:7: HEX_NUMBER
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
    # interface.g:222:1: integer returns [value] : ( '+' DEC_NUMBER | '-' DEC_NUMBER | DEC_NUMBER | HEX_NUMBER );
    def integer(self, ):
        value = None


        DEC_NUMBER3 = None
        DEC_NUMBER4 = None
        DEC_NUMBER5 = None
        HEX_NUMBER6 = None

        try:
            try:
                # interface.g:223:5: ( '+' DEC_NUMBER | '-' DEC_NUMBER | DEC_NUMBER | HEX_NUMBER )
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
                    # interface.g:223:7: '+' DEC_NUMBER
                    pass 
                    self.match(self.input, 32, self.FOLLOW_32_in_integer593)

                    DEC_NUMBER3 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_integer595)

                    #action start
                    value = int(DEC_NUMBER3.text, 10) 
                    #action end



                elif alt3 == 2:
                    # interface.g:224:7: '-' DEC_NUMBER
                    pass 
                    self.match(self.input, 34, self.FOLLOW_34_in_integer607)

                    DEC_NUMBER4 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_integer609)

                    #action start
                    value = -int(DEC_NUMBER4.text, 10) 
                    #action end



                elif alt3 == 3:
                    # interface.g:225:7: DEC_NUMBER
                    pass 
                    DEC_NUMBER5 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_integer621)

                    #action start
                    value = int(DEC_NUMBER5.text, 10) 
                    #action end



                elif alt3 == 4:
                    # interface.g:226:7: HEX_NUMBER
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


    class defineValue_return(ParserRuleReturnScope):
        def __init__(self):
            super(interfaceParser.defineValue_return, self).__init__()

            self.value = None





    # $ANTLR start "defineValue"
    # interface.g:229:1: defineValue returns [value] : ( IDENTIFIER | SCOPED_IDENTIFIER );
    def defineValue(self, ):
        retval = self.defineValue_return()
        retval.start = self.input.LT(1)


        IDENTIFIER7 = None
        SCOPED_IDENTIFIER8 = None

        try:
            try:
                # interface.g:230:5: ( IDENTIFIER | SCOPED_IDENTIFIER )
                alt4 = 2
                LA4_0 = self.input.LA(1)

                if (LA4_0 == IDENTIFIER) :
                    alt4 = 1
                elif (LA4_0 == SCOPED_IDENTIFIER) :
                    alt4 = 2
                else:
                    nvae = NoViableAltException("", 4, 0, self.input)

                    raise nvae


                if alt4 == 1:
                    # interface.g:230:7: IDENTIFIER
                    pass 
                    IDENTIFIER7 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_defineValue666)

                    #action start
                    retval.value = int(self.iface.findDefinition(IDENTIFIER7.text).value) 
                    #action end



                elif alt4 == 2:
                    # interface.g:231:7: SCOPED_IDENTIFIER
                    pass 
                    SCOPED_IDENTIFIER8 = self.match(self.input, SCOPED_IDENTIFIER, self.FOLLOW_SCOPED_IDENTIFIER_in_defineValue683)

                    #action start
                    retval.value = int(self.iface.findDefinition(SCOPED_IDENTIFIER8.text).value) 
                    #action end



                retval.stop = self.input.LT(-1)



            except KeyError, e:

                self.emitErrorMessage(self.getErrorHeaderForToken(retval.start) +
                                      " No definition for {}".format(self.input.toString(retval.start, self.input.LT(-1))))
                self.compileErrors += 1


            except TypeError, e:

                self.emitErrorMessage(self.getErrorHeaderForToken(retval.start) +
                                      " {} is not an integer definition".format(self.input.toString(retval.start, self.input.LT(-1))))
                self.compileErrors += 1



        finally:
            pass
        return retval

    # $ANTLR end "defineValue"



    # $ANTLR start "simpleNumber"
    # interface.g:247:1: simpleNumber returns [value] : ( number | defineValue );
    def simpleNumber(self, ):
        value = None


        number9 = None
        defineValue10 = None

        try:
            try:
                # interface.g:248:5: ( number | defineValue )
                alt5 = 2
                LA5_0 = self.input.LA(1)

                if (LA5_0 == DEC_NUMBER or LA5_0 == HEX_NUMBER) :
                    alt5 = 1
                elif (LA5_0 == IDENTIFIER or LA5_0 == SCOPED_IDENTIFIER) :
                    alt5 = 2
                else:
                    nvae = NoViableAltException("", 5, 0, self.input)

                    raise nvae


                if alt5 == 1:
                    # interface.g:248:7: number
                    pass 
                    self._state.following.append(self.FOLLOW_number_in_simpleNumber720)
                    number9 = self.number()

                    self._state.following.pop()

                    #action start
                    value = number9 
                    #action end



                elif alt5 == 2:
                    # interface.g:249:7: defineValue
                    pass 
                    self._state.following.append(self.FOLLOW_defineValue_in_simpleNumber741)
                    defineValue10 = self.defineValue()

                    self._state.following.pop()

                    #action start
                    value = ((defineValue10 is not None) and [defineValue10.value] or [None])[0] 
                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "simpleNumber"



    # $ANTLR start "simpleExpression"
    # interface.g:252:1: simpleExpression returns [value] : ( integer | defineValue | '(' sumExpression ')' );
    def simpleExpression(self, ):
        value = None


        integer11 = None
        defineValue12 = None
        sumExpression13 = None

        try:
            try:
                # interface.g:253:5: ( integer | defineValue | '(' sumExpression ')' )
                alt6 = 3
                LA6 = self.input.LA(1)
                if LA6 == DEC_NUMBER or LA6 == HEX_NUMBER or LA6 == 32 or LA6 == 34:
                    alt6 = 1
                elif LA6 == IDENTIFIER or LA6 == SCOPED_IDENTIFIER:
                    alt6 = 2
                elif LA6 == 29:
                    alt6 = 3
                else:
                    nvae = NoViableAltException("", 6, 0, self.input)

                    raise nvae


                if alt6 == 1:
                    # interface.g:253:7: integer
                    pass 
                    self._state.following.append(self.FOLLOW_integer_in_simpleExpression770)
                    integer11 = self.integer()

                    self._state.following.pop()

                    #action start
                    value = integer11 
                    #action end



                elif alt6 == 2:
                    # interface.g:254:7: defineValue
                    pass 
                    self._state.following.append(self.FOLLOW_defineValue_in_simpleExpression794)
                    defineValue12 = self.defineValue()

                    self._state.following.pop()

                    #action start
                    value = ((defineValue12 is not None) and [defineValue12.value] or [None])[0] 
                    #action end



                elif alt6 == 3:
                    # interface.g:255:7: '(' sumExpression ')'
                    pass 
                    self.match(self.input, 29, self.FOLLOW_29_in_simpleExpression814)

                    self._state.following.append(self.FOLLOW_sumExpression_in_simpleExpression816)
                    sumExpression13 = self.sumExpression()

                    self._state.following.pop()

                    self.match(self.input, 30, self.FOLLOW_30_in_simpleExpression818)

                    #action start
                    value = sumExpression13 
                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "simpleExpression"



    # $ANTLR start "productExpression"
    # interface.g:258:1: productExpression returns [value] : initialValue= simpleExpression ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )* ;
    def productExpression(self, ):
        value = None


        initialValue = None
        mulValue = None
        divValue = None

        try:
            try:
                # interface.g:259:5: (initialValue= simpleExpression ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )* )
                # interface.g:259:7: initialValue= simpleExpression ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )*
                pass 
                self._state.following.append(self.FOLLOW_simpleExpression_in_productExpression843)
                initialValue = self.simpleExpression()

                self._state.following.pop()

                #action start
                value = initialValue 
                #action end


                # interface.g:260:9: ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )*
                while True: #loop7
                    alt7 = 3
                    LA7_0 = self.input.LA(1)

                    if (LA7_0 == 31) :
                        alt7 = 1
                    elif (LA7_0 == 35) :
                        alt7 = 2


                    if alt7 == 1:
                        # interface.g:260:11: '*' mulValue= simpleExpression
                        pass 
                        self.match(self.input, 31, self.FOLLOW_31_in_productExpression861)

                        self._state.following.append(self.FOLLOW_simpleExpression_in_productExpression865)
                        mulValue = self.simpleExpression()

                        self._state.following.pop()

                        #action start
                        value *= mulValue 
                        #action end



                    elif alt7 == 2:
                        # interface.g:261:11: '/' divValue= simpleExpression
                        pass 
                        self.match(self.input, 35, self.FOLLOW_35_in_productExpression879)

                        self._state.following.append(self.FOLLOW_simpleExpression_in_productExpression883)
                        divValue = self.simpleExpression()

                        self._state.following.pop()

                        #action start
                        value /= divValue 
                        #action end



                    else:
                        break #loop7





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "productExpression"



    # $ANTLR start "sumExpression"
    # interface.g:265:1: sumExpression returns [value] : initialValue= productExpression ( '+' addValue= productExpression | '-' subValue= productExpression )* ;
    def sumExpression(self, ):
        value = None


        initialValue = None
        addValue = None
        subValue = None

        try:
            try:
                # interface.g:266:5: (initialValue= productExpression ( '+' addValue= productExpression | '-' subValue= productExpression )* )
                # interface.g:266:7: initialValue= productExpression ( '+' addValue= productExpression | '-' subValue= productExpression )*
                pass 
                self._state.following.append(self.FOLLOW_productExpression_in_sumExpression919)
                initialValue = self.productExpression()

                self._state.following.pop()

                #action start
                value = initialValue 
                #action end


                # interface.g:267:9: ( '+' addValue= productExpression | '-' subValue= productExpression )*
                while True: #loop8
                    alt8 = 3
                    LA8_0 = self.input.LA(1)

                    if (LA8_0 == 32) :
                        alt8 = 1
                    elif (LA8_0 == 34) :
                        alt8 = 2


                    if alt8 == 1:
                        # interface.g:267:11: '+' addValue= productExpression
                        pass 
                        self.match(self.input, 32, self.FOLLOW_32_in_sumExpression937)

                        self._state.following.append(self.FOLLOW_productExpression_in_sumExpression941)
                        addValue = self.productExpression()

                        self._state.following.pop()

                        #action start
                        value += addValue 
                        #action end



                    elif alt8 == 2:
                        # interface.g:268:11: '-' subValue= productExpression
                        pass 
                        self.match(self.input, 34, self.FOLLOW_34_in_sumExpression955)

                        self._state.following.append(self.FOLLOW_productExpression_in_sumExpression959)
                        subValue = self.productExpression()

                        self._state.following.pop()

                        #action start
                        value -= subValue 
                        #action end



                    else:
                        break #loop8





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "sumExpression"



    # $ANTLR start "valueExpression"
    # interface.g:272:1: valueExpression returns [value] : ( sumExpression | QUOTED_STRING );
    def valueExpression(self, ):
        value = None


        QUOTED_STRING15 = None
        sumExpression14 = None

        try:
            try:
                # interface.g:273:5: ( sumExpression | QUOTED_STRING )
                alt9 = 2
                LA9_0 = self.input.LA(1)

                if (LA9_0 == DEC_NUMBER or (HEX_NUMBER <= LA9_0 <= IDENTIFIER) or LA9_0 == SCOPED_IDENTIFIER or LA9_0 == 29 or LA9_0 == 32 or LA9_0 == 34) :
                    alt9 = 1
                elif (LA9_0 == QUOTED_STRING) :
                    alt9 = 2
                else:
                    nvae = NoViableAltException("", 9, 0, self.input)

                    raise nvae


                if alt9 == 1:
                    # interface.g:273:7: sumExpression
                    pass 
                    self._state.following.append(self.FOLLOW_sumExpression_in_valueExpression993)
                    sumExpression14 = self.sumExpression()

                    self._state.following.pop()

                    #action start
                    value = sumExpression14 
                    #action end



                elif alt9 == 2:
                    # interface.g:274:7: QUOTED_STRING
                    pass 
                    QUOTED_STRING15 = self.match(self.input, QUOTED_STRING, self.FOLLOW_QUOTED_STRING_in_valueExpression1006)

                    #action start
                    value = UnquoteString(QUOTED_STRING15.text) 
                    #action end




            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "valueExpression"



    # $ANTLR start "arrayExpression"
    # interface.g:280:1: arrayExpression returns [size] : '[' simpleNumber ']' ;
    def arrayExpression(self, ):
        size = None


        simpleNumber16 = None

        try:
            try:
                # interface.g:281:5: ( '[' simpleNumber ']' )
                # interface.g:281:7: '[' simpleNumber ']'
                pass 
                self.match(self.input, 37, self.FOLLOW_37_in_arrayExpression1035)

                self._state.following.append(self.FOLLOW_simpleNumber_in_arrayExpression1037)
                simpleNumber16 = self.simpleNumber()

                self._state.following.pop()

                self.match(self.input, 38, self.FOLLOW_38_in_arrayExpression1039)

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
    # interface.g:285:1: typeIdentifier returns [typeObj] : ( IDENTIFIER | SCOPED_IDENTIFIER );
    def typeIdentifier(self, ):
        typeObj = None


        IDENTIFIER17 = None
        SCOPED_IDENTIFIER18 = None

        try:
            try:
                # interface.g:286:5: ( IDENTIFIER | SCOPED_IDENTIFIER )
                alt10 = 2
                LA10_0 = self.input.LA(1)

                if (LA10_0 == IDENTIFIER) :
                    alt10 = 1
                elif (LA10_0 == SCOPED_IDENTIFIER) :
                    alt10 = 2
                else:
                    nvae = NoViableAltException("", 10, 0, self.input)

                    raise nvae


                if alt10 == 1:
                    # interface.g:286:7: IDENTIFIER
                    pass 
                    IDENTIFIER17 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_typeIdentifier1070)

                    #action start
                            
                    try:
                        typeObj = self.iface.findType(IDENTIFIER17.text)
                    except KeyError, e:
                        self.compileErrors += 1
                        self.emitErrorMessage(self.getErrorHeaderForToken(IDENTIFIER17) +
                                              " Unknown type {}".format(e))
                        typeObj = interfaceIR.ERROR_TYPE
                            
                    #action end



                elif alt10 == 2:
                    # interface.g:296:7: SCOPED_IDENTIFIER
                    pass 
                    SCOPED_IDENTIFIER18 = self.match(self.input, SCOPED_IDENTIFIER, self.FOLLOW_SCOPED_IDENTIFIER_in_typeIdentifier1088)

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
    # interface.g:308:1: docPostComments returns [comments] : ( DOC_POST_COMMENT )* ;
    def docPostComments(self, ):
        comments = None


        DOC_POST_COMMENT19 = None

        try:
            try:
                # interface.g:309:5: ( ( DOC_POST_COMMENT )* )
                # interface.g:309:7: ( DOC_POST_COMMENT )*
                pass 
                #action start
                comments = [] 
                #action end


                # interface.g:310:7: ( DOC_POST_COMMENT )*
                while True: #loop11
                    alt11 = 2
                    LA11_0 = self.input.LA(1)

                    if (LA11_0 == DOC_POST_COMMENT) :
                        alt11 = 1


                    if alt11 == 1:
                        # interface.g:310:9: DOC_POST_COMMENT
                        pass 
                        DOC_POST_COMMENT19 = self.match(self.input, DOC_POST_COMMENT, self.FOLLOW_DOC_POST_COMMENT_in_docPostComments1129)

                        #action start
                        comments.append(StripPostComment(DOC_POST_COMMENT19.text)) 
                        #action end



                    else:
                        break #loop11





            except RecognitionException, re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return comments

    # $ANTLR end "docPostComments"



    # $ANTLR start "docPreComment"
    # interface.g:315:1: docPreComment returns [comment] : DOC_PRE_COMMENT ;
    def docPreComment(self, ):
        comment = None


        DOC_PRE_COMMENT20 = None

        try:
            try:
                # interface.g:316:5: ( DOC_PRE_COMMENT )
                # interface.g:316:7: DOC_PRE_COMMENT
                pass 
                DOC_PRE_COMMENT20 = self.match(self.input, DOC_PRE_COMMENT, self.FOLLOW_DOC_PRE_COMMENT_in_docPreComment1173)

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
    # interface.g:319:1: defineDecl returns [define] : DEFINE IDENTIFIER '=' valueExpression ';' ;
    def defineDecl(self, ):
        define = None


        IDENTIFIER21 = None
        valueExpression22 = None

        try:
            try:
                # interface.g:320:5: ( DEFINE IDENTIFIER '=' valueExpression ';' )
                # interface.g:320:7: DEFINE IDENTIFIER '=' valueExpression ';'
                pass 
                self.match(self.input, DEFINE, self.FOLLOW_DEFINE_in_defineDecl1196)

                IDENTIFIER21 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_defineDecl1198)

                self.match(self.input, 36, self.FOLLOW_36_in_defineDecl1200)

                self._state.following.append(self.FOLLOW_valueExpression_in_defineDecl1202)
                valueExpression22 = self.valueExpression()

                self._state.following.pop()

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_defineDecl1204)

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
    # interface.g:326:1: namedValue returns [namedValue] : IDENTIFIER ( '=' integer )? docPostComments ;
    def namedValue(self, ):
        namedValue = None


        IDENTIFIER23 = None
        integer24 = None
        docPostComments25 = None

        try:
            try:
                # interface.g:327:5: ( IDENTIFIER ( '=' integer )? docPostComments )
                # interface.g:327:7: IDENTIFIER ( '=' integer )? docPostComments
                pass 
                IDENTIFIER23 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_namedValue1235)

                # interface.g:327:18: ( '=' integer )?
                alt12 = 2
                LA12_0 = self.input.LA(1)

                if (LA12_0 == 36) :
                    alt12 = 1
                if alt12 == 1:
                    # interface.g:327:20: '=' integer
                    pass 
                    self.match(self.input, 36, self.FOLLOW_36_in_namedValue1239)

                    self._state.following.append(self.FOLLOW_integer_in_namedValue1241)
                    integer24 = self.integer()

                    self._state.following.pop()




                self._state.following.append(self.FOLLOW_docPostComments_in_namedValue1246)
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
    # interface.g:335:1: namedValueList returns [values] : (| namedValue | namedValue ',' docPostComments rest= namedValueList );
    def namedValueList(self, ):
        values = None


        rest = None
        namedValue26 = None
        namedValue27 = None
        docPostComments28 = None

        try:
            try:
                # interface.g:336:5: (| namedValue | namedValue ',' docPostComments rest= namedValueList )
                alt13 = 3
                alt13 = self.dfa13.predict(self.input)
                if alt13 == 1:
                    # interface.g:336:51: 
                    pass 
                    #action start
                    values = [ ] 
                    #action end



                elif alt13 == 2:
                    # interface.g:337:7: namedValue
                    pass 
                    self._state.following.append(self.FOLLOW_namedValue_in_namedValueList1321)
                    namedValue26 = self.namedValue()

                    self._state.following.pop()

                    #action start
                    values = [ namedValue26 ] 
                    #action end



                elif alt13 == 3:
                    # interface.g:338:7: namedValue ',' docPostComments rest= namedValueList
                    pass 
                    self._state.following.append(self.FOLLOW_namedValue_in_namedValueList1364)
                    namedValue27 = self.namedValue()

                    self._state.following.pop()

                    self.match(self.input, 33, self.FOLLOW_33_in_namedValueList1366)

                    self._state.following.append(self.FOLLOW_docPostComments_in_namedValueList1368)
                    docPostComments28 = self.docPostComments()

                    self._state.following.pop()

                    self._state.following.append(self.FOLLOW_namedValueList_in_namedValueList1372)
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
    # interface.g:346:1: enumDecl returns [enum] : ENUM IDENTIFIER '{' namedValueList '}' ';' ;
    def enumDecl(self, ):
        enum = None


        IDENTIFIER29 = None
        namedValueList30 = None

        try:
            try:
                # interface.g:347:5: ( ENUM IDENTIFIER '{' namedValueList '}' ';' )
                # interface.g:347:7: ENUM IDENTIFIER '{' namedValueList '}' ';'
                pass 
                self.match(self.input, ENUM, self.FOLLOW_ENUM_in_enumDecl1403)

                IDENTIFIER29 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_enumDecl1405)

                self.match(self.input, 39, self.FOLLOW_39_in_enumDecl1407)

                self._state.following.append(self.FOLLOW_namedValueList_in_enumDecl1409)
                namedValueList30 = self.namedValueList()

                self._state.following.pop()

                self.match(self.input, 40, self.FOLLOW_40_in_enumDecl1411)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_enumDecl1413)

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
    # interface.g:353:1: bitmaskDecl returns [bitmask] : BITMASK IDENTIFIER '{' namedValueList '}' ';' ;
    def bitmaskDecl(self, ):
        bitmask = None


        IDENTIFIER31 = None
        namedValueList32 = None

        try:
            try:
                # interface.g:354:5: ( BITMASK IDENTIFIER '{' namedValueList '}' ';' )
                # interface.g:354:7: BITMASK IDENTIFIER '{' namedValueList '}' ';'
                pass 
                self.match(self.input, BITMASK, self.FOLLOW_BITMASK_in_bitmaskDecl1444)

                IDENTIFIER31 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_bitmaskDecl1446)

                self.match(self.input, 39, self.FOLLOW_39_in_bitmaskDecl1448)

                self._state.following.append(self.FOLLOW_namedValueList_in_bitmaskDecl1450)
                namedValueList32 = self.namedValueList()

                self._state.following.pop()

                self.match(self.input, 40, self.FOLLOW_40_in_bitmaskDecl1452)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_bitmaskDecl1454)

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
    # interface.g:360:1: referenceDecl returns [ref] : REFERENCE IDENTIFIER ';' ;
    def referenceDecl(self, ):
        ref = None


        IDENTIFIER33 = None

        try:
            try:
                # interface.g:361:5: ( REFERENCE IDENTIFIER ';' )
                # interface.g:361:7: REFERENCE IDENTIFIER ';'
                pass 
                self.match(self.input, REFERENCE, self.FOLLOW_REFERENCE_in_referenceDecl1485)

                IDENTIFIER33 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_referenceDecl1487)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_referenceDecl1489)

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
    # interface.g:367:1: formalParameter returns [parameter] : typeIdentifier IDENTIFIER ( arrayExpression )? ( direction )? docPostComments ;
    def formalParameter(self, ):
        parameter = None


        IDENTIFIER35 = None
        typeIdentifier34 = None
        arrayExpression36 = None
        direction37 = None
        docPostComments38 = None

        try:
            try:
                # interface.g:368:5: ( typeIdentifier IDENTIFIER ( arrayExpression )? ( direction )? docPostComments )
                # interface.g:368:7: typeIdentifier IDENTIFIER ( arrayExpression )? ( direction )? docPostComments
                pass 
                self._state.following.append(self.FOLLOW_typeIdentifier_in_formalParameter1520)
                typeIdentifier34 = self.typeIdentifier()

                self._state.following.pop()

                IDENTIFIER35 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_formalParameter1522)

                # interface.g:368:33: ( arrayExpression )?
                alt14 = 2
                LA14_0 = self.input.LA(1)

                if (LA14_0 == 37) :
                    alt14 = 1
                if alt14 == 1:
                    # interface.g:368:33: arrayExpression
                    pass 
                    self._state.following.append(self.FOLLOW_arrayExpression_in_formalParameter1524)
                    arrayExpression36 = self.arrayExpression()

                    self._state.following.pop()




                # interface.g:368:50: ( direction )?
                alt15 = 2
                LA15_0 = self.input.LA(1)

                if (LA15_0 == IN or LA15_0 == OUT) :
                    alt15 = 1
                if alt15 == 1:
                    # interface.g:368:50: direction
                    pass 
                    self._state.following.append(self.FOLLOW_direction_in_formalParameter1527)
                    direction37 = self.direction()

                    self._state.following.pop()




                self._state.following.append(self.FOLLOW_docPostComments_in_formalParameter1530)
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
    # interface.g:378:1: formalParameterList returns [parameters] : ( formalParameter | formalParameter ',' docPostComments rest= formalParameterList );
    def formalParameterList(self, ):
        parameters = None


        rest = None
        formalParameter39 = None
        formalParameter40 = None
        docPostComments41 = None

        try:
            try:
                # interface.g:379:5: ( formalParameter | formalParameter ',' docPostComments rest= formalParameterList )
                alt16 = 2
                alt16 = self.dfa16.predict(self.input)
                if alt16 == 1:
                    # interface.g:379:7: formalParameter
                    pass 
                    self._state.following.append(self.FOLLOW_formalParameter_in_formalParameterList1561)
                    formalParameter39 = self.formalParameter()

                    self._state.following.pop()

                    #action start
                            
                    parameters = [ formalParameter39 ]
                            
                    #action end



                elif alt16 == 2:
                    # interface.g:383:7: formalParameter ',' docPostComments rest= formalParameterList
                    pass 
                    self._state.following.append(self.FOLLOW_formalParameter_in_formalParameterList1579)
                    formalParameter40 = self.formalParameter()

                    self._state.following.pop()

                    self.match(self.input, 33, self.FOLLOW_33_in_formalParameterList1581)

                    self._state.following.append(self.FOLLOW_docPostComments_in_formalParameterList1583)
                    docPostComments41 = self.docPostComments()

                    self._state.following.pop()

                    self._state.following.append(self.FOLLOW_formalParameterList_in_formalParameterList1587)
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
    # interface.g:391:1: functionDecl returns [function] : FUNCTION ( typeIdentifier )? IDENTIFIER '(' ( formalParameterList )? ')' ';' ;
    def functionDecl(self, ):
        function = None


        IDENTIFIER44 = None
        formalParameterList42 = None
        typeIdentifier43 = None

        try:
            try:
                # interface.g:392:5: ( FUNCTION ( typeIdentifier )? IDENTIFIER '(' ( formalParameterList )? ')' ';' )
                # interface.g:392:7: FUNCTION ( typeIdentifier )? IDENTIFIER '(' ( formalParameterList )? ')' ';'
                pass 
                self.match(self.input, FUNCTION, self.FOLLOW_FUNCTION_in_functionDecl1618)

                # interface.g:392:16: ( typeIdentifier )?
                alt17 = 2
                LA17_0 = self.input.LA(1)

                if (LA17_0 == IDENTIFIER) :
                    LA17_1 = self.input.LA(2)

                    if (LA17_1 == IDENTIFIER) :
                        alt17 = 1
                elif (LA17_0 == SCOPED_IDENTIFIER) :
                    alt17 = 1
                if alt17 == 1:
                    # interface.g:392:16: typeIdentifier
                    pass 
                    self._state.following.append(self.FOLLOW_typeIdentifier_in_functionDecl1620)
                    typeIdentifier43 = self.typeIdentifier()

                    self._state.following.pop()




                IDENTIFIER44 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_functionDecl1623)

                self.match(self.input, 29, self.FOLLOW_29_in_functionDecl1625)

                # interface.g:392:47: ( formalParameterList )?
                alt18 = 2
                LA18_0 = self.input.LA(1)

                if (LA18_0 == IDENTIFIER or LA18_0 == SCOPED_IDENTIFIER) :
                    alt18 = 1
                if alt18 == 1:
                    # interface.g:392:47: formalParameterList
                    pass 
                    self._state.following.append(self.FOLLOW_formalParameterList_in_functionDecl1627)
                    formalParameterList42 = self.formalParameterList()

                    self._state.following.pop()




                self.match(self.input, 30, self.FOLLOW_30_in_functionDecl1630)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_functionDecl1632)

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
    # interface.g:404:1: handlerDecl returns [handler] : HANDLER IDENTIFIER '(' ( formalParameterList )? ')' ';' ;
    def handlerDecl(self, ):
        handler = None


        IDENTIFIER46 = None
        formalParameterList45 = None

        try:
            try:
                # interface.g:405:5: ( HANDLER IDENTIFIER '(' ( formalParameterList )? ')' ';' )
                # interface.g:405:7: HANDLER IDENTIFIER '(' ( formalParameterList )? ')' ';'
                pass 
                self.match(self.input, HANDLER, self.FOLLOW_HANDLER_in_handlerDecl1663)

                IDENTIFIER46 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_handlerDecl1665)

                self.match(self.input, 29, self.FOLLOW_29_in_handlerDecl1667)

                # interface.g:405:30: ( formalParameterList )?
                alt19 = 2
                LA19_0 = self.input.LA(1)

                if (LA19_0 == IDENTIFIER or LA19_0 == SCOPED_IDENTIFIER) :
                    alt19 = 1
                if alt19 == 1:
                    # interface.g:405:30: formalParameterList
                    pass 
                    self._state.following.append(self.FOLLOW_formalParameterList_in_handlerDecl1669)
                    formalParameterList45 = self.formalParameterList()

                    self._state.following.pop()




                self.match(self.input, 30, self.FOLLOW_30_in_handlerDecl1672)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_handlerDecl1674)

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
    # interface.g:416:1: eventDecl returns [event] : EVENT IDENTIFIER '(' ( formalParameterList )? ')' ';' ;
    def eventDecl(self, ):
        event = None


        IDENTIFIER48 = None
        formalParameterList47 = None

        try:
            try:
                # interface.g:417:5: ( EVENT IDENTIFIER '(' ( formalParameterList )? ')' ';' )
                # interface.g:417:7: EVENT IDENTIFIER '(' ( formalParameterList )? ')' ';'
                pass 
                self.match(self.input, EVENT, self.FOLLOW_EVENT_in_eventDecl1705)

                IDENTIFIER48 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_eventDecl1707)

                self.match(self.input, 29, self.FOLLOW_29_in_eventDecl1709)

                # interface.g:417:28: ( formalParameterList )?
                alt20 = 2
                LA20_0 = self.input.LA(1)

                if (LA20_0 == IDENTIFIER or LA20_0 == SCOPED_IDENTIFIER) :
                    alt20 = 1
                if alt20 == 1:
                    # interface.g:417:28: formalParameterList
                    pass 
                    self._state.following.append(self.FOLLOW_formalParameterList_in_eventDecl1711)
                    formalParameterList47 = self.formalParameterList()

                    self._state.following.pop()




                self.match(self.input, 30, self.FOLLOW_30_in_eventDecl1714)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_eventDecl1716)

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
    # interface.g:428:1: declaration returns [declaration] : ( enumDecl | bitmaskDecl | referenceDecl | functionDecl | handlerDecl | eventDecl | defineDecl );
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
                # interface.g:429:5: ( enumDecl | bitmaskDecl | referenceDecl | functionDecl | handlerDecl | eventDecl | defineDecl )
                alt21 = 7
                LA21 = self.input.LA(1)
                if LA21 == ENUM:
                    alt21 = 1
                elif LA21 == BITMASK:
                    alt21 = 2
                elif LA21 == REFERENCE:
                    alt21 = 3
                elif LA21 == FUNCTION:
                    alt21 = 4
                elif LA21 == HANDLER:
                    alt21 = 5
                elif LA21 == EVENT:
                    alt21 = 6
                elif LA21 == DEFINE:
                    alt21 = 7
                else:
                    nvae = NoViableAltException("", 21, 0, self.input)

                    raise nvae


                if alt21 == 1:
                    # interface.g:429:7: enumDecl
                    pass 
                    self._state.following.append(self.FOLLOW_enumDecl_in_declaration1747)
                    enumDecl49 = self.enumDecl()

                    self._state.following.pop()

                    #action start
                    declaration = enumDecl49 
                    #action end



                elif alt21 == 2:
                    # interface.g:430:7: bitmaskDecl
                    pass 
                    self._state.following.append(self.FOLLOW_bitmaskDecl_in_declaration1764)
                    bitmaskDecl50 = self.bitmaskDecl()

                    self._state.following.pop()

                    #action start
                    declaration = bitmaskDecl50 
                    #action end



                elif alt21 == 3:
                    # interface.g:431:7: referenceDecl
                    pass 
                    self._state.following.append(self.FOLLOW_referenceDecl_in_declaration1778)
                    referenceDecl51 = self.referenceDecl()

                    self._state.following.pop()

                    #action start
                    declaration = referenceDecl51 
                    #action end



                elif alt21 == 4:
                    # interface.g:432:7: functionDecl
                    pass 
                    self._state.following.append(self.FOLLOW_functionDecl_in_declaration1790)
                    functionDecl52 = self.functionDecl()

                    self._state.following.pop()

                    #action start
                    declaration = functionDecl52 
                    #action end



                elif alt21 == 5:
                    # interface.g:433:7: handlerDecl
                    pass 
                    self._state.following.append(self.FOLLOW_handlerDecl_in_declaration1803)
                    handlerDecl53 = self.handlerDecl()

                    self._state.following.pop()

                    #action start
                    declaration = handlerDecl53 
                    #action end



                elif alt21 == 6:
                    # interface.g:434:7: eventDecl
                    pass 
                    self._state.following.append(self.FOLLOW_eventDecl_in_declaration1817)
                    eventDecl54 = self.eventDecl()

                    self._state.following.pop()

                    #action start
                    declaration = eventDecl54 
                    #action end



                elif alt21 == 7:
                    # interface.g:435:7: defineDecl
                    pass 
                    self._state.following.append(self.FOLLOW_defineDecl_in_declaration1833)
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
    # interface.g:451:1: documentedDeclaration returns [declaration] : docPreComment declaration ;
    def documentedDeclaration(self, ):
        declaration = None


        declaration56 = None
        docPreComment57 = None

        try:
            try:
                # interface.g:452:5: ( docPreComment declaration )
                # interface.g:452:7: docPreComment declaration
                pass 
                self._state.following.append(self.FOLLOW_docPreComment_in_documentedDeclaration1873)
                docPreComment57 = self.docPreComment()

                self._state.following.pop()

                self._state.following.append(self.FOLLOW_declaration_in_documentedDeclaration1875)
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
    # interface.g:462:1: filename returns [filename] : ( IDENTIFIER | SCOPED_IDENTIFIER );
    def filename(self, ):
        filename = None


        IDENTIFIER58 = None
        SCOPED_IDENTIFIER59 = None

        try:
            try:
                # interface.g:463:5: ( IDENTIFIER | SCOPED_IDENTIFIER )
                alt22 = 2
                LA22_0 = self.input.LA(1)

                if (LA22_0 == IDENTIFIER) :
                    alt22 = 1
                elif (LA22_0 == SCOPED_IDENTIFIER) :
                    alt22 = 2
                else:
                    nvae = NoViableAltException("", 22, 0, self.input)

                    raise nvae


                if alt22 == 1:
                    # interface.g:463:7: IDENTIFIER
                    pass 
                    IDENTIFIER58 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_filename1908)

                    #action start
                    filename = IDENTIFIER58.text 
                    #action end



                elif alt22 == 2:
                    # interface.g:464:7: SCOPED_IDENTIFIER
                    pass 
                    SCOPED_IDENTIFIER59 = self.match(self.input, SCOPED_IDENTIFIER, self.FOLLOW_SCOPED_IDENTIFIER_in_filename1926)

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
    # interface.g:467:1: usetypesStmt : USETYPES filename ';' ;
    def usetypesStmt(self, ):
        filename60 = None

        try:
            try:
                # interface.g:468:5: ( USETYPES filename ';' )
                # interface.g:468:7: USETYPES filename ';'
                pass 
                self.match(self.input, USETYPES, self.FOLLOW_USETYPES_in_usetypesStmt1946)

                self._state.following.append(self.FOLLOW_filename_in_usetypesStmt1948)
                filename60 = self.filename()

                self._state.following.pop()

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_usetypesStmt1950)

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
    # interface.g:484:1: apiDocument returns [iface] : ( ( docPreComment )+ ( usetypesStmt |firstDecl= documentedDeclaration ) )? ( usetypesStmt | declaration |laterDecl= documentedDeclaration )* EOF ;
    def apiDocument(self, ):
        iface = None


        firstDecl = None
        laterDecl = None
        docPreComment61 = None
        declaration62 = None

        try:
            try:
                # interface.g:485:5: ( ( ( docPreComment )+ ( usetypesStmt |firstDecl= documentedDeclaration ) )? ( usetypesStmt | declaration |laterDecl= documentedDeclaration )* EOF )
                # interface.g:485:7: ( ( docPreComment )+ ( usetypesStmt |firstDecl= documentedDeclaration ) )? ( usetypesStmt | declaration |laterDecl= documentedDeclaration )* EOF
                pass 
                # interface.g:485:7: ( ( docPreComment )+ ( usetypesStmt |firstDecl= documentedDeclaration ) )?
                alt25 = 2
                LA25_0 = self.input.LA(1)

                if (LA25_0 == DOC_PRE_COMMENT) :
                    LA25_1 = self.input.LA(2)

                    if (LA25_1 == DOC_PRE_COMMENT or LA25_1 == USETYPES) :
                        alt25 = 1
                if alt25 == 1:
                    # interface.g:485:9: ( docPreComment )+ ( usetypesStmt |firstDecl= documentedDeclaration )
                    pass 
                    # interface.g:485:9: ( docPreComment )+
                    cnt23 = 0
                    while True: #loop23
                        alt23 = 2
                        LA23_0 = self.input.LA(1)

                        if (LA23_0 == DOC_PRE_COMMENT) :
                            LA23_2 = self.input.LA(2)

                            if (LA23_2 == DOC_PRE_COMMENT or LA23_2 == USETYPES) :
                                alt23 = 1




                        if alt23 == 1:
                            # interface.g:485:11: docPreComment
                            pass 
                            self._state.following.append(self.FOLLOW_docPreComment_in_apiDocument1988)
                            docPreComment61 = self.docPreComment()

                            self._state.following.pop()

                            #action start
                            self.iface.comments.append(docPreComment61) 
                            #action end



                        else:
                            if cnt23 >= 1:
                                break #loop23

                            eee = EarlyExitException(23, self.input)
                            raise eee

                        cnt23 += 1


                    # interface.g:486:13: ( usetypesStmt |firstDecl= documentedDeclaration )
                    alt24 = 2
                    LA24_0 = self.input.LA(1)

                    if (LA24_0 == USETYPES) :
                        alt24 = 1
                    elif (LA24_0 == DOC_PRE_COMMENT) :
                        alt24 = 2
                    else:
                        nvae = NoViableAltException("", 24, 0, self.input)

                        raise nvae


                    if alt24 == 1:
                        # interface.g:486:16: usetypesStmt
                        pass 
                        self._state.following.append(self.FOLLOW_usetypesStmt_in_apiDocument2010)
                        self.usetypesStmt()

                        self._state.following.pop()


                    elif alt24 == 2:
                        # interface.g:487:15: firstDecl= documentedDeclaration
                        pass 
                        self._state.following.append(self.FOLLOW_documentedDeclaration_in_apiDocument2028)
                        firstDecl = self.documentedDeclaration()

                        self._state.following.pop()

                        #action start
                                        
                        if firstDecl:
                            self.iface.addDeclaration(firstDecl)
                                        
                        #action end








                # interface.g:493:7: ( usetypesStmt | declaration |laterDecl= documentedDeclaration )*
                while True: #loop26
                    alt26 = 4
                    LA26 = self.input.LA(1)
                    if LA26 == USETYPES:
                        alt26 = 1
                    elif LA26 == BITMASK or LA26 == DEFINE or LA26 == ENUM or LA26 == EVENT or LA26 == FUNCTION or LA26 == HANDLER or LA26 == REFERENCE:
                        alt26 = 2
                    elif LA26 == DOC_PRE_COMMENT:
                        alt26 = 3

                    if alt26 == 1:
                        # interface.g:493:9: usetypesStmt
                        pass 
                        self._state.following.append(self.FOLLOW_usetypesStmt_in_apiDocument2067)
                        self.usetypesStmt()

                        self._state.following.pop()


                    elif alt26 == 2:
                        # interface.g:494:11: declaration
                        pass 
                        self._state.following.append(self.FOLLOW_declaration_in_apiDocument2079)
                        declaration62 = self.declaration()

                        self._state.following.pop()

                        #action start
                                     
                        if declaration62:
                             self.iface.addDeclaration(declaration62)
                                     
                        #action end



                    elif alt26 == 3:
                        # interface.g:499:11: laterDecl= documentedDeclaration
                        pass 
                        self._state.following.append(self.FOLLOW_documentedDeclaration_in_apiDocument2108)
                        laterDecl = self.documentedDeclaration()

                        self._state.following.pop()

                        #action start
                                     
                        if laterDecl:
                             self.iface.addDeclaration(laterDecl)
                                     
                        #action end



                    else:
                        break #loop26


                self.match(self.input, EOF, self.FOLLOW_EOF_in_apiDocument2134)

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



    # lookup tables for DFA #13

    DFA13_eot = DFA.unpack(
        u"\15\uffff"
        )

    DFA13_eof = DFA.unpack(
        u"\15\uffff"
        )

    DFA13_min = DFA.unpack(
        u"\1\23\1\uffff\1\13\1\11\1\13\2\uffff\2\11\4\13"
        )

    DFA13_max = DFA.unpack(
        u"\1\50\1\uffff\1\50\1\42\1\50\2\uffff\2\11\4\50"
        )

    DFA13_accept = DFA.unpack(
        u"\1\uffff\1\1\3\uffff\1\2\1\3\6\uffff"
        )

    DFA13_special = DFA.unpack(
        u"\15\uffff"
        )


    DFA13_transition = [
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

    # class definition for DFA #13

    class DFA13(DFA):
        pass


    # lookup tables for DFA #16

    DFA16_eot = DFA.unpack(
        u"\17\uffff"
        )

    DFA16_eof = DFA.unpack(
        u"\17\uffff"
        )

    DFA16_min = DFA.unpack(
        u"\3\23\1\13\1\11\3\13\2\uffff\4\46\1\13"
        )

    DFA16_max = DFA.unpack(
        u"\1\31\2\23\1\45\1\31\3\41\2\uffff\4\46\1\41"
        )

    DFA16_accept = DFA.unpack(
        u"\10\uffff\1\1\1\2\5\uffff"
        )

    DFA16_special = DFA.unpack(
        u"\17\uffff"
        )


    DFA16_transition = [
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

    # class definition for DFA #16

    class DFA16(DFA):
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
    FOLLOW_IDENTIFIER_in_defineValue666 = frozenset([1])
    FOLLOW_SCOPED_IDENTIFIER_in_defineValue683 = frozenset([1])
    FOLLOW_number_in_simpleNumber720 = frozenset([1])
    FOLLOW_defineValue_in_simpleNumber741 = frozenset([1])
    FOLLOW_integer_in_simpleExpression770 = frozenset([1])
    FOLLOW_defineValue_in_simpleExpression794 = frozenset([1])
    FOLLOW_29_in_simpleExpression814 = frozenset([9, 18, 19, 25, 29, 32, 34])
    FOLLOW_sumExpression_in_simpleExpression816 = frozenset([30])
    FOLLOW_30_in_simpleExpression818 = frozenset([1])
    FOLLOW_simpleExpression_in_productExpression843 = frozenset([1, 31, 35])
    FOLLOW_31_in_productExpression861 = frozenset([9, 18, 19, 25, 29, 32, 34])
    FOLLOW_simpleExpression_in_productExpression865 = frozenset([1, 31, 35])
    FOLLOW_35_in_productExpression879 = frozenset([9, 18, 19, 25, 29, 32, 34])
    FOLLOW_simpleExpression_in_productExpression883 = frozenset([1, 31, 35])
    FOLLOW_productExpression_in_sumExpression919 = frozenset([1, 32, 34])
    FOLLOW_32_in_sumExpression937 = frozenset([9, 18, 19, 25, 29, 32, 34])
    FOLLOW_productExpression_in_sumExpression941 = frozenset([1, 32, 34])
    FOLLOW_34_in_sumExpression955 = frozenset([9, 18, 19, 25, 29, 32, 34])
    FOLLOW_productExpression_in_sumExpression959 = frozenset([1, 32, 34])
    FOLLOW_sumExpression_in_valueExpression993 = frozenset([1])
    FOLLOW_QUOTED_STRING_in_valueExpression1006 = frozenset([1])
    FOLLOW_37_in_arrayExpression1035 = frozenset([9, 18, 19, 25])
    FOLLOW_simpleNumber_in_arrayExpression1037 = frozenset([38])
    FOLLOW_38_in_arrayExpression1039 = frozenset([1])
    FOLLOW_IDENTIFIER_in_typeIdentifier1070 = frozenset([1])
    FOLLOW_SCOPED_IDENTIFIER_in_typeIdentifier1088 = frozenset([1])
    FOLLOW_DOC_POST_COMMENT_in_docPostComments1129 = frozenset([1, 11])
    FOLLOW_DOC_PRE_COMMENT_in_docPreComment1173 = frozenset([1])
    FOLLOW_DEFINE_in_defineDecl1196 = frozenset([19])
    FOLLOW_IDENTIFIER_in_defineDecl1198 = frozenset([36])
    FOLLOW_36_in_defineDecl1200 = frozenset([9, 18, 19, 23, 25, 29, 32, 34])
    FOLLOW_valueExpression_in_defineDecl1202 = frozenset([26])
    FOLLOW_SEMICOLON_in_defineDecl1204 = frozenset([1])
    FOLLOW_IDENTIFIER_in_namedValue1235 = frozenset([11, 36])
    FOLLOW_36_in_namedValue1239 = frozenset([9, 18, 32, 34])
    FOLLOW_integer_in_namedValue1241 = frozenset([11])
    FOLLOW_docPostComments_in_namedValue1246 = frozenset([1])
    FOLLOW_namedValue_in_namedValueList1321 = frozenset([1])
    FOLLOW_namedValue_in_namedValueList1364 = frozenset([33])
    FOLLOW_33_in_namedValueList1366 = frozenset([11, 19])
    FOLLOW_docPostComments_in_namedValueList1368 = frozenset([19])
    FOLLOW_namedValueList_in_namedValueList1372 = frozenset([1])
    FOLLOW_ENUM_in_enumDecl1403 = frozenset([19])
    FOLLOW_IDENTIFIER_in_enumDecl1405 = frozenset([39])
    FOLLOW_39_in_enumDecl1407 = frozenset([19, 40])
    FOLLOW_namedValueList_in_enumDecl1409 = frozenset([40])
    FOLLOW_40_in_enumDecl1411 = frozenset([26])
    FOLLOW_SEMICOLON_in_enumDecl1413 = frozenset([1])
    FOLLOW_BITMASK_in_bitmaskDecl1444 = frozenset([19])
    FOLLOW_IDENTIFIER_in_bitmaskDecl1446 = frozenset([39])
    FOLLOW_39_in_bitmaskDecl1448 = frozenset([19, 40])
    FOLLOW_namedValueList_in_bitmaskDecl1450 = frozenset([40])
    FOLLOW_40_in_bitmaskDecl1452 = frozenset([26])
    FOLLOW_SEMICOLON_in_bitmaskDecl1454 = frozenset([1])
    FOLLOW_REFERENCE_in_referenceDecl1485 = frozenset([19])
    FOLLOW_IDENTIFIER_in_referenceDecl1487 = frozenset([26])
    FOLLOW_SEMICOLON_in_referenceDecl1489 = frozenset([1])
    FOLLOW_typeIdentifier_in_formalParameter1520 = frozenset([19])
    FOLLOW_IDENTIFIER_in_formalParameter1522 = frozenset([11, 20, 22, 37])
    FOLLOW_arrayExpression_in_formalParameter1524 = frozenset([11, 20, 22])
    FOLLOW_direction_in_formalParameter1527 = frozenset([11])
    FOLLOW_docPostComments_in_formalParameter1530 = frozenset([1])
    FOLLOW_formalParameter_in_formalParameterList1561 = frozenset([1])
    FOLLOW_formalParameter_in_formalParameterList1579 = frozenset([33])
    FOLLOW_33_in_formalParameterList1581 = frozenset([11, 19, 25])
    FOLLOW_docPostComments_in_formalParameterList1583 = frozenset([19, 25])
    FOLLOW_formalParameterList_in_formalParameterList1587 = frozenset([1])
    FOLLOW_FUNCTION_in_functionDecl1618 = frozenset([19, 25])
    FOLLOW_typeIdentifier_in_functionDecl1620 = frozenset([19])
    FOLLOW_IDENTIFIER_in_functionDecl1623 = frozenset([29])
    FOLLOW_29_in_functionDecl1625 = frozenset([19, 25, 30])
    FOLLOW_formalParameterList_in_functionDecl1627 = frozenset([30])
    FOLLOW_30_in_functionDecl1630 = frozenset([26])
    FOLLOW_SEMICOLON_in_functionDecl1632 = frozenset([1])
    FOLLOW_HANDLER_in_handlerDecl1663 = frozenset([19])
    FOLLOW_IDENTIFIER_in_handlerDecl1665 = frozenset([29])
    FOLLOW_29_in_handlerDecl1667 = frozenset([19, 25, 30])
    FOLLOW_formalParameterList_in_handlerDecl1669 = frozenset([30])
    FOLLOW_30_in_handlerDecl1672 = frozenset([26])
    FOLLOW_SEMICOLON_in_handlerDecl1674 = frozenset([1])
    FOLLOW_EVENT_in_eventDecl1705 = frozenset([19])
    FOLLOW_IDENTIFIER_in_eventDecl1707 = frozenset([29])
    FOLLOW_29_in_eventDecl1709 = frozenset([19, 25, 30])
    FOLLOW_formalParameterList_in_eventDecl1711 = frozenset([30])
    FOLLOW_30_in_eventDecl1714 = frozenset([26])
    FOLLOW_SEMICOLON_in_eventDecl1716 = frozenset([1])
    FOLLOW_enumDecl_in_declaration1747 = frozenset([1])
    FOLLOW_bitmaskDecl_in_declaration1764 = frozenset([1])
    FOLLOW_referenceDecl_in_declaration1778 = frozenset([1])
    FOLLOW_functionDecl_in_declaration1790 = frozenset([1])
    FOLLOW_handlerDecl_in_declaration1803 = frozenset([1])
    FOLLOW_eventDecl_in_declaration1817 = frozenset([1])
    FOLLOW_defineDecl_in_declaration1833 = frozenset([1])
    FOLLOW_docPreComment_in_documentedDeclaration1873 = frozenset([6, 10, 13, 14, 15, 16, 24])
    FOLLOW_declaration_in_documentedDeclaration1875 = frozenset([1])
    FOLLOW_IDENTIFIER_in_filename1908 = frozenset([1])
    FOLLOW_SCOPED_IDENTIFIER_in_filename1926 = frozenset([1])
    FOLLOW_USETYPES_in_usetypesStmt1946 = frozenset([19, 25])
    FOLLOW_filename_in_usetypesStmt1948 = frozenset([26])
    FOLLOW_SEMICOLON_in_usetypesStmt1950 = frozenset([1])
    FOLLOW_docPreComment_in_apiDocument1988 = frozenset([12, 27])
    FOLLOW_usetypesStmt_in_apiDocument2010 = frozenset([6, 10, 12, 13, 14, 15, 16, 24, 27])
    FOLLOW_documentedDeclaration_in_apiDocument2028 = frozenset([6, 10, 12, 13, 14, 15, 16, 24, 27])
    FOLLOW_usetypesStmt_in_apiDocument2067 = frozenset([6, 10, 12, 13, 14, 15, 16, 24, 27])
    FOLLOW_declaration_in_apiDocument2079 = frozenset([6, 10, 12, 13, 14, 15, 16, 24, 27])
    FOLLOW_documentedDeclaration_in_apiDocument2108 = frozenset([6, 10, 12, 13, 14, 15, 16, 24, 27])
    FOLLOW_EOF_in_apiDocument2134 = frozenset([1])



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
