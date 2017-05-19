# $ANTLR 3.5.2 interface.g 2017-05-19 10:42:33

import sys
from antlr3 import *
from antlr3.compat import set, frozenset



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


class interfaceLexer(Lexer):

    grammarFileName = "interface.g"
    api_version = 1

    def __init__(self, input=None, state=None):
        if state is None:
            state = RecognizerSharedState()
        super(interfaceLexer, self).__init__(input, state)

        self.delegates = []

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




    # $ANTLR start "T__29"
    def mT__29(self, ):
        try:
            _type = T__29
            _channel = DEFAULT_CHANNEL

            # interface.g:27:7: ( '(' )
            # interface.g:27:9: '('
            pass 
            self.match(40)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__29"



    # $ANTLR start "T__30"
    def mT__30(self, ):
        try:
            _type = T__30
            _channel = DEFAULT_CHANNEL

            # interface.g:28:7: ( ')' )
            # interface.g:28:9: ')'
            pass 
            self.match(41)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__30"



    # $ANTLR start "T__31"
    def mT__31(self, ):
        try:
            _type = T__31
            _channel = DEFAULT_CHANNEL

            # interface.g:29:7: ( '*' )
            # interface.g:29:9: '*'
            pass 
            self.match(42)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__31"



    # $ANTLR start "T__32"
    def mT__32(self, ):
        try:
            _type = T__32
            _channel = DEFAULT_CHANNEL

            # interface.g:30:7: ( '+' )
            # interface.g:30:9: '+'
            pass 
            self.match(43)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__32"



    # $ANTLR start "T__33"
    def mT__33(self, ):
        try:
            _type = T__33
            _channel = DEFAULT_CHANNEL

            # interface.g:31:7: ( ',' )
            # interface.g:31:9: ','
            pass 
            self.match(44)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__33"



    # $ANTLR start "T__34"
    def mT__34(self, ):
        try:
            _type = T__34
            _channel = DEFAULT_CHANNEL

            # interface.g:32:7: ( '-' )
            # interface.g:32:9: '-'
            pass 
            self.match(45)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__34"



    # $ANTLR start "T__35"
    def mT__35(self, ):
        try:
            _type = T__35
            _channel = DEFAULT_CHANNEL

            # interface.g:33:7: ( '/' )
            # interface.g:33:9: '/'
            pass 
            self.match(47)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__35"



    # $ANTLR start "T__36"
    def mT__36(self, ):
        try:
            _type = T__36
            _channel = DEFAULT_CHANNEL

            # interface.g:34:7: ( '=' )
            # interface.g:34:9: '='
            pass 
            self.match(61)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__36"



    # $ANTLR start "T__37"
    def mT__37(self, ):
        try:
            _type = T__37
            _channel = DEFAULT_CHANNEL

            # interface.g:35:7: ( '[' )
            # interface.g:35:9: '['
            pass 
            self.match(91)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__37"



    # $ANTLR start "T__38"
    def mT__38(self, ):
        try:
            _type = T__38
            _channel = DEFAULT_CHANNEL

            # interface.g:36:7: ( ']' )
            # interface.g:36:9: ']'
            pass 
            self.match(93)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__38"



    # $ANTLR start "T__39"
    def mT__39(self, ):
        try:
            _type = T__39
            _channel = DEFAULT_CHANNEL

            # interface.g:37:7: ( '{' )
            # interface.g:37:9: '{'
            pass 
            self.match(123)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__39"



    # $ANTLR start "T__40"
    def mT__40(self, ):
        try:
            _type = T__40
            _channel = DEFAULT_CHANNEL

            # interface.g:38:7: ( '}' )
            # interface.g:38:9: '}'
            pass 
            self.match(125)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "T__40"



    # $ANTLR start "ALPHA"
    def mALPHA(self, ):
        try:
            # interface.g:170:16: ( 'a' .. 'z' | 'A' .. 'Z' )
            # interface.g:
            pass 
            if (65 <= self.input.LA(1) <= 90) or (97 <= self.input.LA(1) <= 122):
                self.input.consume()
            else:
                mse = MismatchedSetException(None, self.input)
                self.recover(mse)
                raise mse






        finally:
            pass

    # $ANTLR end "ALPHA"



    # $ANTLR start "NUM"
    def mNUM(self, ):
        try:
            # interface.g:171:14: ( '0' .. '9' )
            # interface.g:
            pass 
            if (48 <= self.input.LA(1) <= 57):
                self.input.consume()
            else:
                mse = MismatchedSetException(None, self.input)
                self.recover(mse)
                raise mse






        finally:
            pass

    # $ANTLR end "NUM"



    # $ANTLR start "HEXNUM"
    def mHEXNUM(self, ):
        try:
            # interface.g:172:17: ( NUM | 'a' .. 'f' | 'A' .. 'F' )
            # interface.g:
            pass 
            if (48 <= self.input.LA(1) <= 57) or (65 <= self.input.LA(1) <= 70) or (97 <= self.input.LA(1) <= 102):
                self.input.consume()
            else:
                mse = MismatchedSetException(None, self.input)
                self.recover(mse)
                raise mse






        finally:
            pass

    # $ANTLR end "HEXNUM"



    # $ANTLR start "ALPHANUM"
    def mALPHANUM(self, ):
        try:
            # interface.g:173:19: ( ALPHA | NUM )
            # interface.g:
            pass 
            if (48 <= self.input.LA(1) <= 57) or (65 <= self.input.LA(1) <= 90) or (97 <= self.input.LA(1) <= 122):
                self.input.consume()
            else:
                mse = MismatchedSetException(None, self.input)
                self.recover(mse)
                raise mse






        finally:
            pass

    # $ANTLR end "ALPHANUM"



    # $ANTLR start "SEMICOLON"
    def mSEMICOLON(self, ):
        try:
            _type = SEMICOLON
            _channel = DEFAULT_CHANNEL

            # interface.g:179:11: ( ';' )
            # interface.g:179:13: ';'
            pass 
            self.match(59)



            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "SEMICOLON"



    # $ANTLR start "IN"
    def mIN(self, ):
        try:
            _type = IN
            _channel = DEFAULT_CHANNEL

            # interface.g:182:4: ( 'IN' )
            # interface.g:182:6: 'IN'
            pass 
            self.match("IN")




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "IN"



    # $ANTLR start "OUT"
    def mOUT(self, ):
        try:
            _type = OUT
            _channel = DEFAULT_CHANNEL

            # interface.g:183:5: ( 'OUT' )
            # interface.g:183:7: 'OUT'
            pass 
            self.match("OUT")




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "OUT"



    # $ANTLR start "FUNCTION"
    def mFUNCTION(self, ):
        try:
            _type = FUNCTION
            _channel = DEFAULT_CHANNEL

            # interface.g:184:10: ( 'FUNCTION' )
            # interface.g:184:12: 'FUNCTION'
            pass 
            self.match("FUNCTION")




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "FUNCTION"



    # $ANTLR start "HANDLER"
    def mHANDLER(self, ):
        try:
            _type = HANDLER
            _channel = DEFAULT_CHANNEL

            # interface.g:185:9: ( 'HANDLER' )
            # interface.g:185:11: 'HANDLER'
            pass 
            self.match("HANDLER")




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "HANDLER"



    # $ANTLR start "EVENT"
    def mEVENT(self, ):
        try:
            _type = EVENT
            _channel = DEFAULT_CHANNEL

            # interface.g:186:7: ( 'EVENT' )
            # interface.g:186:9: 'EVENT'
            pass 
            self.match("EVENT")




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "EVENT"



    # $ANTLR start "REFERENCE"
    def mREFERENCE(self, ):
        try:
            _type = REFERENCE
            _channel = DEFAULT_CHANNEL

            # interface.g:187:11: ( 'REFERENCE' )
            # interface.g:187:13: 'REFERENCE'
            pass 
            self.match("REFERENCE")




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "REFERENCE"



    # $ANTLR start "DEFINE"
    def mDEFINE(self, ):
        try:
            _type = DEFINE
            _channel = DEFAULT_CHANNEL

            # interface.g:188:8: ( 'DEFINE' )
            # interface.g:188:10: 'DEFINE'
            pass 
            self.match("DEFINE")




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "DEFINE"



    # $ANTLR start "ENUM"
    def mENUM(self, ):
        try:
            _type = ENUM
            _channel = DEFAULT_CHANNEL

            # interface.g:189:6: ( 'ENUM' )
            # interface.g:189:8: 'ENUM'
            pass 
            self.match("ENUM")




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "ENUM"



    # $ANTLR start "BITMASK"
    def mBITMASK(self, ):
        try:
            _type = BITMASK
            _channel = DEFAULT_CHANNEL

            # interface.g:190:9: ( 'BITMASK' )
            # interface.g:190:11: 'BITMASK'
            pass 
            self.match("BITMASK")




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "BITMASK"



    # $ANTLR start "USETYPES"
    def mUSETYPES(self, ):
        try:
            _type = USETYPES
            _channel = DEFAULT_CHANNEL

            # interface.g:191:10: ( 'USETYPES' )
            # interface.g:191:12: 'USETYPES'
            pass 
            self.match("USETYPES")




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "USETYPES"



    # $ANTLR start "IDENTIFIER"
    def mIDENTIFIER(self, ):
        try:
            _type = IDENTIFIER
            _channel = DEFAULT_CHANNEL

            # interface.g:195:12: ( ALPHA ( ALPHANUM | '_' )* )
            # interface.g:195:14: ALPHA ( ALPHANUM | '_' )*
            pass 
            self.mALPHA()


            # interface.g:195:20: ( ALPHANUM | '_' )*
            while True: #loop1
                alt1 = 2
                LA1_0 = self.input.LA(1)

                if ((48 <= LA1_0 <= 57) or (65 <= LA1_0 <= 90) or LA1_0 == 95 or (97 <= LA1_0 <= 122)) :
                    alt1 = 1


                if alt1 == 1:
                    # interface.g:
                    pass 
                    if (48 <= self.input.LA(1) <= 57) or (65 <= self.input.LA(1) <= 90) or self.input.LA(1) == 95 or (97 <= self.input.LA(1) <= 122):
                        self.input.consume()
                    else:
                        mse = MismatchedSetException(None, self.input)
                        self.recover(mse)
                        raise mse




                else:
                    break #loop1




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "IDENTIFIER"



    # $ANTLR start "SCOPED_IDENTIFIER"
    def mSCOPED_IDENTIFIER(self, ):
        try:
            _type = SCOPED_IDENTIFIER
            _channel = DEFAULT_CHANNEL

            # interface.g:198:19: ( IDENTIFIER '.' IDENTIFIER )
            # interface.g:198:21: IDENTIFIER '.' IDENTIFIER
            pass 
            self.mIDENTIFIER()


            self.match(46)

            self.mIDENTIFIER()




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "SCOPED_IDENTIFIER"



    # $ANTLR start "DEC_NUMBER"
    def mDEC_NUMBER(self, ):
        try:
            _type = DEC_NUMBER
            _channel = DEFAULT_CHANNEL

            number = None

            # interface.g:201:12: (number= ( '0' | '1' .. '9' ( NUM )* ) )
            # interface.g:201:14: number= ( '0' | '1' .. '9' ( NUM )* )
            pass 
            # interface.g:201:21: ( '0' | '1' .. '9' ( NUM )* )
            alt3 = 2
            LA3_0 = self.input.LA(1)

            if (LA3_0 == 48) :
                alt3 = 1
            elif ((49 <= LA3_0 <= 57)) :
                alt3 = 2
            else:
                nvae = NoViableAltException("", 3, 0, self.input)

                raise nvae


            if alt3 == 1:
                # interface.g:201:23: '0'
                pass 
                number = self.input.LA(1)

                self.match(48)


            elif alt3 == 2:
                # interface.g:201:29: '1' .. '9' ( NUM )*
                pass 
                number = self.input.LA(1)

                self.matchRange(49, 57)

                # interface.g:201:38: ( NUM )*
                while True: #loop2
                    alt2 = 2
                    LA2_0 = self.input.LA(1)

                    if ((48 <= LA2_0 <= 57)) :
                        alt2 = 1


                    if alt2 == 1:
                        # interface.g:
                        pass 
                        if (48 <= self.input.LA(1) <= 57):
                            self.input.consume()
                        else:
                            mse = MismatchedSetException(None, self.input)
                            self.recover(mse)
                            raise mse




                    else:
                        break #loop2







            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "DEC_NUMBER"



    # $ANTLR start "HEX_NUMBER"
    def mHEX_NUMBER(self, ):
        try:
            _type = HEX_NUMBER
            _channel = DEFAULT_CHANNEL

            # interface.g:201:12: ( '0' ( 'x' | 'X' ) ( HEXNUM )+ )
            # interface.g:201:14: '0' ( 'x' | 'X' ) ( HEXNUM )+
            pass 
            self.match(48)

            if self.input.LA(1) == 88 or self.input.LA(1) == 120:
                self.input.consume()
            else:
                mse = MismatchedSetException(None, self.input)
                self.recover(mse)
                raise mse



            # interface.g:201:31: ( HEXNUM )+
            cnt4 = 0
            while True: #loop4
                alt4 = 2
                LA4_0 = self.input.LA(1)

                if ((48 <= LA4_0 <= 57) or (65 <= LA4_0 <= 70) or (97 <= LA4_0 <= 102)) :
                    alt4 = 1


                if alt4 == 1:
                    # interface.g:
                    pass 
                    if (48 <= self.input.LA(1) <= 57) or (65 <= self.input.LA(1) <= 70) or (97 <= self.input.LA(1) <= 102):
                        self.input.consume()
                    else:
                        mse = MismatchedSetException(None, self.input)
                        self.recover(mse)
                        raise mse




                else:
                    if cnt4 >= 1:
                        break #loop4

                    eee = EarlyExitException(4, self.input)
                    raise eee

                cnt4 += 1




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "HEX_NUMBER"



    # $ANTLR start "QUOTED_STRING"
    def mQUOTED_STRING(self, ):
        try:
            _type = QUOTED_STRING
            _channel = DEFAULT_CHANNEL

            # interface.g:204:15: ( '\"' (~ ( '\\\"' | '\\\\' ) | '\\\\' . )* '\"' | '\\'' (~ ( '\\'' | '\\\\' ) | '\\\\' . )* '\\'' )
            alt7 = 2
            LA7_0 = self.input.LA(1)

            if (LA7_0 == 34) :
                alt7 = 1
            elif (LA7_0 == 39) :
                alt7 = 2
            else:
                nvae = NoViableAltException("", 7, 0, self.input)

                raise nvae


            if alt7 == 1:
                # interface.g:204:17: '\"' (~ ( '\\\"' | '\\\\' ) | '\\\\' . )* '\"'
                pass 
                self.match(34)

                # interface.g:204:21: (~ ( '\\\"' | '\\\\' ) | '\\\\' . )*
                while True: #loop5
                    alt5 = 3
                    LA5_0 = self.input.LA(1)

                    if ((0 <= LA5_0 <= 33) or (35 <= LA5_0 <= 91) or (93 <= LA5_0 <= 65535)) :
                        alt5 = 1
                    elif (LA5_0 == 92) :
                        alt5 = 2


                    if alt5 == 1:
                        # interface.g:204:23: ~ ( '\\\"' | '\\\\' )
                        pass 
                        if (0 <= self.input.LA(1) <= 33) or (35 <= self.input.LA(1) <= 91) or (93 <= self.input.LA(1) <= 65535):
                            self.input.consume()
                        else:
                            mse = MismatchedSetException(None, self.input)
                            self.recover(mse)
                            raise mse




                    elif alt5 == 2:
                        # interface.g:204:40: '\\\\' .
                        pass 
                        self.match(92)

                        self.matchAny()


                    else:
                        break #loop5


                self.match(34)


            elif alt7 == 2:
                # interface.g:205:7: '\\'' (~ ( '\\'' | '\\\\' ) | '\\\\' . )* '\\''
                pass 
                self.match(39)

                # interface.g:205:12: (~ ( '\\'' | '\\\\' ) | '\\\\' . )*
                while True: #loop6
                    alt6 = 3
                    LA6_0 = self.input.LA(1)

                    if ((0 <= LA6_0 <= 38) or (40 <= LA6_0 <= 91) or (93 <= LA6_0 <= 65535)) :
                        alt6 = 1
                    elif (LA6_0 == 92) :
                        alt6 = 2


                    if alt6 == 1:
                        # interface.g:205:14: ~ ( '\\'' | '\\\\' )
                        pass 
                        if (0 <= self.input.LA(1) <= 38) or (40 <= self.input.LA(1) <= 91) or (93 <= self.input.LA(1) <= 65535):
                            self.input.consume()
                        else:
                            mse = MismatchedSetException(None, self.input)
                            self.recover(mse)
                            raise mse




                    elif alt6 == 2:
                        # interface.g:205:31: '\\\\' .
                        pass 
                        self.match(92)

                        self.matchAny()


                    else:
                        break #loop6


                self.match(39)


            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "QUOTED_STRING"



    # $ANTLR start "DOC_PRE_COMMENT"
    def mDOC_PRE_COMMENT(self, ):
        try:
            _type = DOC_PRE_COMMENT
            _channel = DEFAULT_CHANNEL

            # interface.g:209:17: ( '/**' ~ '*' (~ '*' | ( '*' )+ ~ ( '/' | '*' ) )+ '*/' )
            # interface.g:209:19: '/**' ~ '*' (~ '*' | ( '*' )+ ~ ( '/' | '*' ) )+ '*/'
            pass 
            self.match("/**")


            if (0 <= self.input.LA(1) <= 41) or (43 <= self.input.LA(1) <= 65535):
                self.input.consume()
            else:
                mse = MismatchedSetException(None, self.input)
                self.recover(mse)
                raise mse



            # interface.g:209:30: (~ '*' | ( '*' )+ ~ ( '/' | '*' ) )+
            cnt9 = 0
            while True: #loop9
                alt9 = 3
                LA9_0 = self.input.LA(1)

                if (LA9_0 == 42) :
                    LA9_1 = self.input.LA(2)

                    if ((0 <= LA9_1 <= 46) or (48 <= LA9_1 <= 65535)) :
                        alt9 = 2


                elif ((0 <= LA9_0 <= 41) or (43 <= LA9_0 <= 65535)) :
                    alt9 = 1


                if alt9 == 1:
                    # interface.g:209:31: ~ '*'
                    pass 
                    if (0 <= self.input.LA(1) <= 41) or (43 <= self.input.LA(1) <= 65535):
                        self.input.consume()
                    else:
                        mse = MismatchedSetException(None, self.input)
                        self.recover(mse)
                        raise mse




                elif alt9 == 2:
                    # interface.g:209:38: ( '*' )+ ~ ( '/' | '*' )
                    pass 
                    # interface.g:209:38: ( '*' )+
                    cnt8 = 0
                    while True: #loop8
                        alt8 = 2
                        LA8_0 = self.input.LA(1)

                        if (LA8_0 == 42) :
                            alt8 = 1


                        if alt8 == 1:
                            # interface.g:209:38: '*'
                            pass 
                            self.match(42)


                        else:
                            if cnt8 >= 1:
                                break #loop8

                            eee = EarlyExitException(8, self.input)
                            raise eee

                        cnt8 += 1


                    if (0 <= self.input.LA(1) <= 41) or (43 <= self.input.LA(1) <= 46) or (48 <= self.input.LA(1) <= 65535):
                        self.input.consume()
                    else:
                        mse = MismatchedSetException(None, self.input)
                        self.recover(mse)
                        raise mse




                else:
                    if cnt9 >= 1:
                        break #loop9

                    eee = EarlyExitException(9, self.input)
                    raise eee

                cnt9 += 1


            self.match("*/")




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "DOC_PRE_COMMENT"



    # $ANTLR start "DOC_POST_COMMENT"
    def mDOC_POST_COMMENT(self, ):
        try:
            _type = DOC_POST_COMMENT
            _channel = DEFAULT_CHANNEL

            # interface.g:212:18: ( '///<' (~ '\\n' )* )
            # interface.g:212:20: '///<' (~ '\\n' )*
            pass 
            self.match("///<")


            # interface.g:212:27: (~ '\\n' )*
            while True: #loop10
                alt10 = 2
                LA10_0 = self.input.LA(1)

                if ((0 <= LA10_0 <= 9) or (11 <= LA10_0 <= 65535)) :
                    alt10 = 1


                if alt10 == 1:
                    # interface.g:
                    pass 
                    if (0 <= self.input.LA(1) <= 9) or (11 <= self.input.LA(1) <= 65535):
                        self.input.consume()
                    else:
                        mse = MismatchedSetException(None, self.input)
                        self.recover(mse)
                        raise mse




                else:
                    break #loop10




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "DOC_POST_COMMENT"



    # $ANTLR start "WS"
    def mWS(self, ):
        try:
            _type = WS
            _channel = DEFAULT_CHANNEL

            # interface.g:215:4: ( ( ' ' | '\\t' | '\\n' )+ )
            # interface.g:215:6: ( ' ' | '\\t' | '\\n' )+
            pass 
            # interface.g:215:6: ( ' ' | '\\t' | '\\n' )+
            cnt11 = 0
            while True: #loop11
                alt11 = 2
                LA11_0 = self.input.LA(1)

                if ((9 <= LA11_0 <= 10) or LA11_0 == 32) :
                    alt11 = 1


                if alt11 == 1:
                    # interface.g:
                    pass 
                    if (9 <= self.input.LA(1) <= 10) or self.input.LA(1) == 32:
                        self.input.consume()
                    else:
                        mse = MismatchedSetException(None, self.input)
                        self.recover(mse)
                        raise mse




                else:
                    if cnt11 >= 1:
                        break #loop11

                    eee = EarlyExitException(11, self.input)
                    raise eee

                cnt11 += 1


            #action start
            self.skip() 
            #action end




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "WS"



    # $ANTLR start "C_COMMENT"
    def mC_COMMENT(self, ):
        try:
            _type = C_COMMENT
            _channel = DEFAULT_CHANNEL

            # interface.g:218:11: ( '/*' (~ '*' | ( '*' )+ ~ ( '/' | '*' ) )+ ( '*' )+ '/' )
            # interface.g:218:13: '/*' (~ '*' | ( '*' )+ ~ ( '/' | '*' ) )+ ( '*' )+ '/'
            pass 
            self.match("/*")


            # interface.g:218:18: (~ '*' | ( '*' )+ ~ ( '/' | '*' ) )+
            cnt13 = 0
            while True: #loop13
                alt13 = 3
                alt13 = self.dfa13.predict(self.input)
                if alt13 == 1:
                    # interface.g:218:19: ~ '*'
                    pass 
                    if (0 <= self.input.LA(1) <= 41) or (43 <= self.input.LA(1) <= 65535):
                        self.input.consume()
                    else:
                        mse = MismatchedSetException(None, self.input)
                        self.recover(mse)
                        raise mse




                elif alt13 == 2:
                    # interface.g:218:26: ( '*' )+ ~ ( '/' | '*' )
                    pass 
                    # interface.g:218:26: ( '*' )+
                    cnt12 = 0
                    while True: #loop12
                        alt12 = 2
                        LA12_0 = self.input.LA(1)

                        if (LA12_0 == 42) :
                            alt12 = 1


                        if alt12 == 1:
                            # interface.g:218:26: '*'
                            pass 
                            self.match(42)


                        else:
                            if cnt12 >= 1:
                                break #loop12

                            eee = EarlyExitException(12, self.input)
                            raise eee

                        cnt12 += 1


                    if (0 <= self.input.LA(1) <= 41) or (43 <= self.input.LA(1) <= 46) or (48 <= self.input.LA(1) <= 65535):
                        self.input.consume()
                    else:
                        mse = MismatchedSetException(None, self.input)
                        self.recover(mse)
                        raise mse




                else:
                    if cnt13 >= 1:
                        break #loop13

                    eee = EarlyExitException(13, self.input)
                    raise eee

                cnt13 += 1


            # interface.g:218:47: ( '*' )+
            cnt14 = 0
            while True: #loop14
                alt14 = 2
                LA14_0 = self.input.LA(1)

                if (LA14_0 == 42) :
                    alt14 = 1


                if alt14 == 1:
                    # interface.g:218:47: '*'
                    pass 
                    self.match(42)


                else:
                    if cnt14 >= 1:
                        break #loop14

                    eee = EarlyExitException(14, self.input)
                    raise eee

                cnt14 += 1


            self.match(47)

            #action start
            self.skip() 
            #action end




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "C_COMMENT"



    # $ANTLR start "CPP_COMMENT"
    def mCPP_COMMENT(self, ):
        try:
            _type = CPP_COMMENT
            _channel = DEFAULT_CHANNEL

            # interface.g:221:13: ( '//' (~ '\\n' )* )
            # interface.g:221:15: '//' (~ '\\n' )*
            pass 
            self.match("//")


            # interface.g:221:20: (~ '\\n' )*
            while True: #loop15
                alt15 = 2
                LA15_0 = self.input.LA(1)

                if ((0 <= LA15_0 <= 9) or (11 <= LA15_0 <= 65535)) :
                    alt15 = 1


                if alt15 == 1:
                    # interface.g:
                    pass 
                    if (0 <= self.input.LA(1) <= 9) or (11 <= self.input.LA(1) <= 65535):
                        self.input.consume()
                    else:
                        mse = MismatchedSetException(None, self.input)
                        self.recover(mse)
                        raise mse




                else:
                    break #loop15


            #action start
            self.skip() 
            #action end




            self._state.type = _type
            self._state.channel = _channel
        finally:
            pass

    # $ANTLR end "CPP_COMMENT"



    def mTokens(self):
        # interface.g:1:8: ( T__29 | T__30 | T__31 | T__32 | T__33 | T__34 | T__35 | T__36 | T__37 | T__38 | T__39 | T__40 | SEMICOLON | IN | OUT | FUNCTION | HANDLER | EVENT | REFERENCE | DEFINE | ENUM | BITMASK | USETYPES | IDENTIFIER | SCOPED_IDENTIFIER | DEC_NUMBER | HEX_NUMBER | QUOTED_STRING | DOC_PRE_COMMENT | DOC_POST_COMMENT | WS | C_COMMENT | CPP_COMMENT )
        alt16 = 33
        alt16 = self.dfa16.predict(self.input)
        if alt16 == 1:
            # interface.g:1:10: T__29
            pass 
            self.mT__29()



        elif alt16 == 2:
            # interface.g:1:16: T__30
            pass 
            self.mT__30()



        elif alt16 == 3:
            # interface.g:1:22: T__31
            pass 
            self.mT__31()



        elif alt16 == 4:
            # interface.g:1:28: T__32
            pass 
            self.mT__32()



        elif alt16 == 5:
            # interface.g:1:34: T__33
            pass 
            self.mT__33()



        elif alt16 == 6:
            # interface.g:1:40: T__34
            pass 
            self.mT__34()



        elif alt16 == 7:
            # interface.g:1:46: T__35
            pass 
            self.mT__35()



        elif alt16 == 8:
            # interface.g:1:52: T__36
            pass 
            self.mT__36()



        elif alt16 == 9:
            # interface.g:1:58: T__37
            pass 
            self.mT__37()



        elif alt16 == 10:
            # interface.g:1:64: T__38
            pass 
            self.mT__38()



        elif alt16 == 11:
            # interface.g:1:70: T__39
            pass 
            self.mT__39()



        elif alt16 == 12:
            # interface.g:1:76: T__40
            pass 
            self.mT__40()



        elif alt16 == 13:
            # interface.g:1:82: SEMICOLON
            pass 
            self.mSEMICOLON()



        elif alt16 == 14:
            # interface.g:1:92: IN
            pass 
            self.mIN()



        elif alt16 == 15:
            # interface.g:1:95: OUT
            pass 
            self.mOUT()



        elif alt16 == 16:
            # interface.g:1:99: FUNCTION
            pass 
            self.mFUNCTION()



        elif alt16 == 17:
            # interface.g:1:108: HANDLER
            pass 
            self.mHANDLER()



        elif alt16 == 18:
            # interface.g:1:116: EVENT
            pass 
            self.mEVENT()



        elif alt16 == 19:
            # interface.g:1:122: REFERENCE
            pass 
            self.mREFERENCE()



        elif alt16 == 20:
            # interface.g:1:132: DEFINE
            pass 
            self.mDEFINE()



        elif alt16 == 21:
            # interface.g:1:139: ENUM
            pass 
            self.mENUM()



        elif alt16 == 22:
            # interface.g:1:144: BITMASK
            pass 
            self.mBITMASK()



        elif alt16 == 23:
            # interface.g:1:152: USETYPES
            pass 
            self.mUSETYPES()



        elif alt16 == 24:
            # interface.g:1:161: IDENTIFIER
            pass 
            self.mIDENTIFIER()



        elif alt16 == 25:
            # interface.g:1:172: SCOPED_IDENTIFIER
            pass 
            self.mSCOPED_IDENTIFIER()



        elif alt16 == 26:
            # interface.g:1:190: DEC_NUMBER
            pass 
            self.mDEC_NUMBER()



        elif alt16 == 27:
            # interface.g:1:201: HEX_NUMBER
            pass 
            self.mHEX_NUMBER()



        elif alt16 == 28:
            # interface.g:1:212: QUOTED_STRING
            pass 
            self.mQUOTED_STRING()



        elif alt16 == 29:
            # interface.g:1:226: DOC_PRE_COMMENT
            pass 
            self.mDOC_PRE_COMMENT()



        elif alt16 == 30:
            # interface.g:1:242: DOC_POST_COMMENT
            pass 
            self.mDOC_POST_COMMENT()



        elif alt16 == 31:
            # interface.g:1:259: WS
            pass 
            self.mWS()



        elif alt16 == 32:
            # interface.g:1:262: C_COMMENT
            pass 
            self.mC_COMMENT()



        elif alt16 == 33:
            # interface.g:1:272: CPP_COMMENT
            pass 
            self.mCPP_COMMENT()








    # lookup tables for DFA #13

    DFA13_eot = DFA.unpack(
        u"\5\uffff"
        )

    DFA13_eof = DFA.unpack(
        u"\5\uffff"
        )

    DFA13_min = DFA.unpack(
        u"\2\0\3\uffff"
        )

    DFA13_max = DFA.unpack(
        u"\2\uffff\3\uffff"
        )

    DFA13_accept = DFA.unpack(
        u"\2\uffff\1\1\1\3\1\2"
        )

    DFA13_special = DFA.unpack(
        u"\1\1\1\0\3\uffff"
        )


    DFA13_transition = [
        DFA.unpack(u"\52\2\1\1\uffd5\2"),
        DFA.unpack(u"\52\4\1\1\4\4\1\3\uffd0\4"),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u"")
    ]

    # class definition for DFA #13

    class DFA13(DFA):
        pass


        def specialStateTransition(self_, s, input):
            # convince pylint that my self_ magic is ok ;)
            # pylint: disable-msg=E0213

            # pretend we are a member of the recognizer
            # thus semantic predicates can be evaluated
            self = self_.recognizer

            _s = s

            if s == 0: 
                LA13_1 = input.LA(1)

                s = -1
                if (LA13_1 == 47):
                    s = 3

                elif (LA13_1 == 42):
                    s = 1

                elif ((0 <= LA13_1 <= 41) or (43 <= LA13_1 <= 46) or (48 <= LA13_1 <= 65535)):
                    s = 4

                if s >= 0:
                    return s
            elif s == 1: 
                LA13_0 = input.LA(1)

                s = -1
                if (LA13_0 == 42):
                    s = 1

                elif ((0 <= LA13_0 <= 41) or (43 <= LA13_0 <= 65535)):
                    s = 2

                if s >= 0:
                    return s

            nvae = NoViableAltException(self_.getDescription(), 13, _s, input)
            self_.error(nvae)
            raise nvae

    # lookup tables for DFA #16

    DFA16_eot = DFA.unpack(
        u"\7\uffff\1\36\6\uffff\12\40\1\31\4\uffff\1\60\1\uffff\1\61\1\uffff"
        u"\1\40\1\uffff\11\40\3\uffff\1\60\2\uffff\1\76\10\40\2\uffff\1\112"
        u"\1\uffff\3\40\1\116\4\40\2\uffff\1\112\1\uffff\2\40\1\127\1\uffff"
        u"\4\40\2\uffff\2\40\1\uffff\1\40\1\140\2\40\1\uffff\1\40\1\144\1"
        u"\40\1\uffff\1\146\1\40\1\150\1\uffff\1\40\1\uffff\1\152\1\uffff"
        u"\1\153\2\uffff"
        )

    DFA16_eof = DFA.unpack(
        u"\154\uffff"
        )

    DFA16_min = DFA.unpack(
        u"\1\11\6\uffff\1\52\6\uffff\12\56\1\130\3\uffff\1\0\1\57\1\uffff"
        u"\1\56\1\uffff\1\56\1\uffff\11\56\1\uffff\1\0\1\uffff\1\74\2\uffff"
        u"\11\56\1\0\1\uffff\1\0\1\uffff\10\56\3\0\1\uffff\3\56\1\uffff\4"
        u"\56\2\0\2\56\1\uffff\4\56\1\uffff\3\56\1\uffff\3\56\1\uffff\1\56"
        u"\1\uffff\1\56\1\uffff\1\56\2\uffff"
        )

    DFA16_max = DFA.unpack(
        u"\1\175\6\uffff\1\57\6\uffff\12\172\1\170\3\uffff\1\uffff\1\57\1"
        u"\uffff\1\172\1\uffff\1\172\1\uffff\11\172\1\uffff\1\uffff\1\uffff"
        u"\1\74\2\uffff\11\172\1\uffff\1\uffff\1\uffff\1\uffff\10\172\3\uffff"
        u"\1\uffff\3\172\1\uffff\4\172\2\uffff\2\172\1\uffff\4\172\1\uffff"
        u"\3\172\1\uffff\3\172\1\uffff\1\172\1\uffff\1\172\1\uffff\1\172"
        u"\2\uffff"
        )

    DFA16_accept = DFA.unpack(
        u"\1\uffff\1\1\1\2\1\3\1\4\1\5\1\6\1\uffff\1\10\1\11\1\12\1\13\1"
        u"\14\1\15\13\uffff\1\32\1\34\1\37\2\uffff\1\7\1\uffff\1\30\1\uffff"
        u"\1\31\11\uffff\1\33\1\uffff\1\40\1\uffff\1\41\1\16\12\uffff\1\35"
        u"\1\uffff\1\17\13\uffff\1\36\3\uffff\1\25\10\uffff\1\22\4\uffff"
        u"\1\35\3\uffff\1\24\3\uffff\1\21\1\uffff\1\26\1\uffff\1\20\1\uffff"
        u"\1\27\1\23"
        )

    DFA16_special = DFA.unpack(
        u"\34\uffff\1\4\20\uffff\1\0\15\uffff\1\1\1\uffff\1\10\11\uffff\1"
        u"\5\1\7\1\3\11\uffff\1\2\1\6\27\uffff"
        )


    DFA16_transition = [
        DFA.unpack(u"\2\33\25\uffff\1\33\1\uffff\1\32\4\uffff\1\32\1\1\1"
        u"\2\1\3\1\4\1\5\1\6\1\uffff\1\7\1\30\11\31\1\uffff\1\15\1\uffff"
        u"\1\10\3\uffff\1\27\1\25\1\27\1\24\1\22\1\20\1\27\1\21\1\16\5\27"
        u"\1\17\2\27\1\23\2\27\1\26\5\27\1\11\1\uffff\1\12\3\uffff\32\27"
        u"\1\13\1\uffff\1\14"),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u"\1\34\4\uffff\1\35"),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\15\41\1\37\14\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\24\41\1\43\5\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\24\41\1\44\5\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\1\45\31\41\4\uffff\1\41"
        u"\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\15\41\1\47\7\41\1\46\4"
        u"\41\4\uffff\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\4\41\1\50\25\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\4\41\1\51\25\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\10\41\1\52\21\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\22\41\1\53\7\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u"\1\54\37\uffff\1\54"),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u"\52\56\1\55\uffd5\56"),
        DFA.unpack(u"\1\57"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\23\41\1\62\6\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\15\41\1\63\14\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\15\41\1\64\14\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\4\41\1\65\25\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\24\41\1\66\5\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\5\41\1\67\24\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\5\41\1\70\24\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\23\41\1\71\6\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\4\41\1\72\25\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u""),
        DFA.unpack(u"\52\73\1\56\4\73\1\74\uffd0\73"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\75"),
        DFA.unpack(u""),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\2\41\1\77\27\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\3\41\1\100\26\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\15\41\1\101\14\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\14\41\1\102\15\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\4\41\1\103\25\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\10\41\1\104\21\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\14\41\1\105\15\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\23\41\1\106\6\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\52\107\1\110\uffd5\107"),
        DFA.unpack(u""),
        DFA.unpack(u"\12\111\1\uffff\ufff5\111"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\23\41\1\113\6\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\13\41\1\114\16\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\23\41\1\115\6\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\21\41\1\117\10\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\15\41\1\120\14\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\1\121\31\41\4\uffff\1\41"
        u"\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\30\41\1\122\1\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\52\107\1\123\uffd5\107"),
        DFA.unpack(u"\52\124\1\110\4\124\1\56\uffd0\124"),
        DFA.unpack(u"\12\111\1\uffff\ufff5\111"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\10\41\1\125\21\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\4\41\1\126\25\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\4\41\1\130\25\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\4\41\1\131\25\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\22\41\1\132\7\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\17\41\1\133\12\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\52\124\1\110\4\124\1\134\uffd0\124"),
        DFA.unpack(u"\52\107\1\123\uffd5\107"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\16\41\1\135\13\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\21\41\1\136\10\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\15\41\1\137\14\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\12\41\1\141\17\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\4\41\1\142\25\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\15\41\1\143\14\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\2\41\1\145\27\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\22\41\1\147\7\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\4\41\1\151\25\41\4\uffff"
        u"\1\41\1\uffff\32\41"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u""),
        DFA.unpack(u"\1\42\1\uffff\12\41\7\uffff\32\41\4\uffff\1\41\1\uffff"
        u"\32\41"),
        DFA.unpack(u""),
        DFA.unpack(u"")
    ]

    # class definition for DFA #16

    class DFA16(DFA):
        pass


        def specialStateTransition(self_, s, input):
            # convince pylint that my self_ magic is ok ;)
            # pylint: disable-msg=E0213

            # pretend we are a member of the recognizer
            # thus semantic predicates can be evaluated
            self = self_.recognizer

            _s = s

            if s == 0: 
                LA16_45 = input.LA(1)

                s = -1
                if ((0 <= LA16_45 <= 41) or (43 <= LA16_45 <= 46) or (48 <= LA16_45 <= 65535)):
                    s = 59

                elif (LA16_45 == 47):
                    s = 60

                elif (LA16_45 == 42):
                    s = 46

                if s >= 0:
                    return s
            elif s == 1: 
                LA16_59 = input.LA(1)

                s = -1
                if ((0 <= LA16_59 <= 41) or (43 <= LA16_59 <= 65535)):
                    s = 71

                elif (LA16_59 == 42):
                    s = 72

                if s >= 0:
                    return s
            elif s == 2: 
                LA16_83 = input.LA(1)

                s = -1
                if (LA16_83 == 47):
                    s = 92

                elif ((0 <= LA16_83 <= 41) or (43 <= LA16_83 <= 46) or (48 <= LA16_83 <= 65535)):
                    s = 84

                elif (LA16_83 == 42):
                    s = 72

                if s >= 0:
                    return s
            elif s == 3: 
                LA16_73 = input.LA(1)

                s = -1
                if ((0 <= LA16_73 <= 9) or (11 <= LA16_73 <= 65535)):
                    s = 73

                else:
                    s = 74

                if s >= 0:
                    return s
            elif s == 4: 
                LA16_28 = input.LA(1)

                s = -1
                if (LA16_28 == 42):
                    s = 45

                elif ((0 <= LA16_28 <= 41) or (43 <= LA16_28 <= 65535)):
                    s = 46

                if s >= 0:
                    return s
            elif s == 5: 
                LA16_71 = input.LA(1)

                s = -1
                if (LA16_71 == 42):
                    s = 83

                elif ((0 <= LA16_71 <= 41) or (43 <= LA16_71 <= 65535)):
                    s = 71

                if s >= 0:
                    return s
            elif s == 6: 
                LA16_84 = input.LA(1)

                s = -1
                if (LA16_84 == 42):
                    s = 83

                elif ((0 <= LA16_84 <= 41) or (43 <= LA16_84 <= 65535)):
                    s = 71

                if s >= 0:
                    return s
            elif s == 7: 
                LA16_72 = input.LA(1)

                s = -1
                if ((0 <= LA16_72 <= 41) or (43 <= LA16_72 <= 46) or (48 <= LA16_72 <= 65535)):
                    s = 84

                elif (LA16_72 == 42):
                    s = 72

                elif (LA16_72 == 47):
                    s = 46

                if s >= 0:
                    return s
            elif s == 8: 
                LA16_61 = input.LA(1)

                s = -1
                if ((0 <= LA16_61 <= 9) or (11 <= LA16_61 <= 65535)):
                    s = 73

                else:
                    s = 74

                if s >= 0:
                    return s

            nvae = NoViableAltException(self_.getDescription(), 16, _s, input)
            self_.error(nvae)
            raise nvae

 



def main(argv, stdin=sys.stdin, stdout=sys.stdout, stderr=sys.stderr):
    from antlr3.main import LexerMain
    main = LexerMain(interfaceLexer)

    main.stdin = stdin
    main.stdout = stdout
    main.stderr = stderr
    main.execute(argv)



if __name__ == '__main__':
    main(sys.argv)
