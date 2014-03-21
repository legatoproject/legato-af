/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Cuero Bugot for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "smspdu.h"
#ifdef FOR_LUA
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#endif

/* Define Non-Printable Characters as a question mark */
#define NPC7    63
#define NPC8    '?'

/****************************************************************************
 This lookup table converts from ISO-8859-1 8-bit ASCII to the
 7 bit "default alphabet" as defined in ETSI GSM 03.38

 ISO-characters that don't have any corresponding character in the
 7-bit alphabet is replaced with the NPC7-character.  If there's
 a close match between the ISO-char and a 7-bit character (for example
 the letter i with a circumflex and the plain i-character) a substitution
 is done.

 There are some character (for example the curly brace "}") that must
 be converted into a 2 byte 7-bit sequence.  These characters are
 marked in the table by having 128 added to its value.
 ****************************************************************************/
uint8_t lookup_ascii8to7[]={
    NPC7,       /*     0      null [NUL]                              */
    NPC7,       /*     1      start of heading [SOH]                  */
    NPC7,       /*     2      start of text [STX]                     */
    NPC7,       /*     3      end of text [ETX]                       */
    NPC7,       /*     4      end of transmission [EOT]               */
    NPC7,       /*     5      enquiry [ENQ]                           */
    NPC7,       /*     6      acknowledge [ACK]                       */
    NPC7,       /*     7      bell [BEL]                              */
    NPC7,       /*     8      backspace [BS]                          */
    NPC7,       /*     9      horizontal tab [HT]                     */
    10,         /*    10      line feed [LF]                          */
    NPC7,       /*    11      vertical tab [VT]                       */
    10+128,     /*    12      form feed [FF]                          */
    13,         /*    13      carriage return [CR]                    */
    NPC7,       /*    14      shift out [SO]                          */
    NPC7,       /*    15      shift in [SI]                           */
    NPC7,       /*    16      data link escape [DLE]                  */
    NPC7,       /*    17      device control 1 [DC1]                  */
    NPC7,       /*    18      device control 2 [DC2]                  */
    NPC7,       /*    19      device control 3 [DC3]                  */
    NPC7,       /*    20      device control 4 [DC4]                  */
    NPC7,       /*    21      negative acknowledge [NAK]              */
    NPC7,       /*    22      synchronous idle [SYN]                  */
    NPC7,       /*    23      end of trans. block [ETB]               */
    NPC7,       /*    24      cancel [CAN]                            */
    NPC7,       /*    25      end of medium [EM]                      */
    NPC7,       /*    26      substitute [SUB]                        */
    NPC7,       /*    27      escape [ESC]                            */
    NPC7,       /*    28      file separator [FS]                     */
    NPC7,       /*    29      group separator [GS]                    */
    NPC7,       /*    30      record separator [RS]                   */
    NPC7,       /*    31      unit separator [US]                     */
    32,         /*    32      space                                   */
    33,         /*    33    ! exclamation mark                        */
    34,         /*    34    " double quotation mark                   */
    35,         /*    35    # number sign                             */
    2,          /*    36    $ dollar sign                             */
    37,         /*    37    % percent sign                            */
    38,         /*    38    & ampersand                               */
    39,         /*    39    ' apostrophe                              */
    40,         /*    40    ( left parenthesis                        */
    41,         /*    41    ) right parenthesis                       */
    42,         /*    42    * asterisk                                */
    43,         /*    43    + plus sign                               */
    44,         /*    44    , comma                                   */
    45,         /*    45    - hyphen                                  */
    46,         /*    46    . period                                  */
    47,         /*    47    / slash,                                  */
    48,         /*    48    0 digit 0                                 */
    49,         /*    49    1 digit 1                                 */
    50,         /*    50    2 digit 2                                 */
    51,         /*    51    3 digit 3                                 */
    52,         /*    52    4 digit 4                                 */
    53,         /*    53    5 digit 5                                 */
    54,         /*    54    6 digit 6                                 */
    55,         /*    55    7 digit 7                                 */
    56,         /*    56    8 digit 8                                 */
    57,         /*    57    9 digit 9                                 */
    58,         /*    58    : colon                                   */
    59,         /*    59    ; semicolon                               */
    60,         /*    60    < less-than sign                          */
    61,         /*    61    = equal sign                              */
    62,         /*    62    > greater-than sign                       */
    63,         /*    63    ? question mark                           */
    0,          /*    64    @ commercial at sign                      */
    65,         /*    65    A uppercase A                             */
    66,         /*    66    B uppercase B                             */
    67,         /*    67    C uppercase C                             */
    68,         /*    68    D uppercase D                             */
    69,         /*    69    E uppercase E                             */
    70,         /*    70    F uppercase F                             */
    71,         /*    71    G uppercase G                             */
    72,         /*    72    H uppercase H                             */
    73,         /*    73    I uppercase I                             */
    74,         /*    74    J uppercase J                             */
    75,         /*    75    K uppercase K                             */
    76,         /*    76    L uppercase L                             */
    77,         /*    77    M uppercase M                             */
    78,         /*    78    N uppercase N                             */
    79,         /*    79    O uppercase O                             */
    80,         /*    80    P uppercase P                             */
    81,         /*    81    Q uppercase Q                             */
    82,         /*    82    R uppercase R                             */
    83,         /*    83    S uppercase S                             */
    84,         /*    84    T uppercase T                             */
    85,         /*    85    U uppercase U                             */
    86,         /*    86    V uppercase V                             */
    87,         /*    87    W uppercase W                             */
    88,         /*    88    X uppercase X                             */
    89,         /*    89    Y uppercase Y                             */
    90,         /*    90    Z uppercase Z                             */
    60+128,     /*    91    [ left square bracket                     */
    47+128,     /*    92    \ backslash                               */
    62+128,     /*    93    ] right square bracket                    */
    20+128,     /*    94    ^ circumflex accent                       */
    17,         /*    95    _ underscore                              */
    -39,        /*    96    ` back apostrophe                         */
    97,         /*    97    a lowercase a                             */
    98,         /*    98    b lowercase b                             */
    99,         /*    99    c lowercase c                             */
    100,        /*   100    d lowercase d                             */
    101,        /*   101    e lowercase e                             */
    102,        /*   102    f lowercase f                             */
    103,        /*   103    g lowercase g                             */
    104,        /*   104    h lowercase h                             */
    105,        /*   105    i lowercase i                             */
    106,        /*   106    j lowercase j                             */
    107,        /*   107    k lowercase k                             */
    108,        /*   108    l lowercase l                             */
    109,        /*   109    m lowercase m                             */
    110,        /*   110    n lowercase n                             */
    111,        /*   111    o lowercase o                             */
    112,        /*   112    p lowercase p                             */
    113,        /*   113    q lowercase q                             */
    114,        /*   114    r lowercase r                             */
    115,        /*   115    s lowercase s                             */
    116,        /*   116    t lowercase t                             */
    117,        /*   117    u lowercase u                             */
    118,        /*   118    v lowercase v                             */
    119,        /*   119    w lowercase w                             */
    120,        /*   120    x lowercase x                             */
    121,        /*   121    y lowercase y                             */
    122,        /*   122    z lowercase z                             */
    40+128,     /*   123    { left brace                              */
    64+128,     /*   124    | vertical bar                            */
    41+128,     /*   125    } right brace                             */
    61+128,     /*   126    ~ tilde accent                            */
    NPC7,       /*   127      delete [DEL]                            */
    NPC7,       /*   128                                              */
    NPC7,       /*   129                                              */
    39,         /*   130      low left rising single quote            */
    102,        /*   131      lowercase italic f                      */
    34,         /*   132      low left rising double quote            */
    NPC7,       /*   133      low horizontal ellipsis                 */
    NPC7,       /*   134      dagger mark                             */
    NPC7,       /*   135      double dagger mark                      */
    NPC7,       /*   136      letter modifying circumflex             */
    NPC7,       /*   137      per thousand (mille) sign               */
    83,         /*   138      uppercase S caron or hacek              */
    39,         /*   139      left single angle quote mark            */
    214,        /*   140      uppercase OE ligature                   */
    NPC7,       /*   141                                              */
    NPC7,       /*   142                                              */
    NPC7,       /*   143                                              */
    NPC7,       /*   144                                              */
    39,         /*   145      left single quotation mark              */
    39,         /*   146      right single quote mark                 */
    34,         /*   147      left double quotation mark              */
    34,         /*   148      right double quote mark                 */
    42,         /*   149      round filled bullet                     */
    45,         /*   150      en dash                                 */
    45,         /*   151      em dash                                 */
    39,         /*   152      small spacing tilde accent              */
    NPC7,       /*   153      trademark sign                          */
    115,        /*   154      lowercase s caron or hacek              */
    39,         /*   155      right single angle quote mark           */
    111,        /*   156      lowercase oe ligature                   */
    NPC7,       /*   157                                              */
    NPC7,       /*   158                                              */
    89,         /*   159      uppercase Y dieresis or umlaut          */
    32,         /*   160      non-breaking space                      */
    64,         /*   161    ¡ inverted exclamation mark               */
    99,         /*   162    ¢ cent sign                               */
    1,          /*   163    £ pound sterling sign                     */
    36,         /*   164    € general currency sign                   */
    3,          /*   165    ¥ yen sign                                */
    33,         /*   166    Š broken vertical bar                     */
    95,         /*   167    § section sign                            */
    34,         /*   168    š spacing dieresis or umlaut              */
    NPC7,       /*   169    © copyright sign                          */
    NPC7,       /*   170    ª feminine ordinal indicator              */
    60,         /*   171    « left (double) angle quote               */
    NPC7,       /*   172    ¬ logical not sign                        */
    45,         /*   173    ­ soft hyphen                             */
    NPC7,       /*   174    ® registered trademark sign               */
    NPC7,       /*   175    ¯ spacing macron (long) accent            */
    NPC7,       /*   176    ° degree sign                             */
    NPC7,       /*   177    ± plus-or-minus sign                      */
    50,         /*   178    ² superscript 2                           */
    51,         /*   179    ³ superscript 3                           */
    39,         /*   180    Ž spacing acute accent                    */
    117,        /*   181    µ micro sign                              */
    NPC7,       /*   182    ¶ paragraph sign, pilcrow sign            */
    NPC7,       /*   183    · middle dot, centered dot                */
    NPC7,       /*   184    ž spacing cedilla                         */
    49,         /*   185    ¹ superscript 1                           */
    NPC7,       /*   186    º masculine ordinal indicator             */
    62,         /*   187    » right (double) angle quote (guillemet)  */
    NPC7,       /*   188    Œ fraction 1/4                            */
    NPC7,       /*   189    œ fraction 1/2                            */
    NPC7,       /*   190    Ÿ fraction 3/4                            */
    96,         /*   191    ¿ inverted question mark                  */
    65,         /*   192    À uppercase A grave                       */
    65,         /*   193    Á uppercase A acute                       */
    65,         /*   194    Â uppercase A circumflex                  */
    65,         /*   195    Ã uppercase A tilde                       */
    91,         /*   196    Ä uppercase A dieresis or umlaut          */
    14,         /*   197    Å uppercase A ring                        */
    28,         /*   198    Æ uppercase AE ligature                   */
    9,          /*   199    Ç uppercase C cedilla                     */
    31,         /*   200    È uppercase E grave                       */
    31,         /*   201    É uppercase E acute                       */
    31,         /*   202    Ê uppercase E circumflex                  */
    31,         /*   203    Ë uppercase E dieresis or umlaut          */
    73,         /*   204    Ì uppercase I grave                       */
    73,         /*   205    Í uppercase I acute                       */
    73,         /*   206    Î uppercase I circumflex                  */
    73,         /*   207    Ï uppercase I dieresis or umlaut          */
    68,         /*   208    Ð uppercase ETH                           */
    93,         /*   209    Ñ uppercase N tilde                       */
    79,         /*   210    Ò uppercase O grave                       */
    79,         /*   211    Ó uppercase O acute                       */
    79,         /*   212    Ô uppercase O circumflex                  */
    79,         /*   213    Õ uppercase O tilde                       */
    92,         /*   214    Ö uppercase O dieresis or umlaut          */
    42,         /*   215    × multiplication sign                     */
    11,         /*   216    Ø uppercase O slash                       */
    85,         /*   217    Ù uppercase U grave                       */
    85,         /*   218    Ú uppercase U acute                       */
    85,         /*   219    Û uppercase U circumflex                  */
    94,         /*   220    Ü uppercase U dieresis or umlaut          */
    89,         /*   221    Ý uppercase Y acute                       */
    NPC7,       /*   222    Þ uppercase THORN                         */
    30,         /*   223    ß lowercase sharp s, sz ligature          */
    127,        /*   224    à lowercase a grave                       */
    97,         /*   225    á lowercase a acute                       */
    97,         /*   226    â lowercase a circumflex                  */
    97,         /*   227    ã lowercase a tilde                       */
    123,        /*   228    ä lowercase a dieresis or umlaut          */
    15,         /*   229    å lowercase a ring                        */
    29,         /*   230    æ lowercase ae ligature                   */
    9,          /*   231    ç lowercase c cedilla                     */
    4,          /*   232    è lowercase e grave                       */
    5,          /*   233    é lowercase e acute                       */
    101,        /*   234    ê lowercase e circumflex                  */
    101,        /*   235    ë lowercase e dieresis or umlaut          */
    7,          /*   236    ì lowercase i grave                       */
    7,          /*   237    í lowercase i acute                       */
    105,        /*   238    î lowercase i circumflex                  */
    105,        /*   239    ï lowercase i dieresis or umlaut          */
    NPC7,       /*   240    ð lowercase eth                           */
    125,        /*   241    ñ lowercase n tilde                       */
    8,          /*   242    ò lowercase o grave                       */
    111,        /*   243    ó lowercase o acute                       */
    111,        /*   244    ô lowercase o circumflex                  */
    111,        /*   245    õ lowercase o tilde                       */
    24,         /*   246    ö lowercase o dieresis or umlaut          */
    47,         /*   247    ÷ division sign                           */
    12,         /*   248    ø lowercase o slash                       */
    6,          /*   249    ù lowercase u grave                       */
    117,        /*   250    ú lowercase u acute                       */
    117,        /*   251    û lowercase u circumflex                  */
    126,        /*   252    ü lowercase u dieresis or umlaut          */
    121,        /*   253    ý lowercase y acute                       */
    NPC7,       /*   254    þ lowercase thorn                         */
    121         /*   255    ÿ lowercase y dieresis or umlaut          */
};



/****************************************************************************
   This lookup table converts from the 7 bit "default alphabet" as
    defined in ETSI GSM 03.38 to a standard ISO-8859-1 8-bit ASCII.

    Some characters in the 7-bit alphabet does not exist in the ISO
    character set, they are replaced by the NPC8-character.

    If the character is decimal 27 (ESC) the following character have
    a special meaning and must be handled separately.
****************************************************************************/

uint8_t lookup_ascii7to8[]={
    64,         /*  0      @  COMMERCIAL AT                           */
    163,        /*  1      £  POUND SIGN                              */
    36,         /*  2      $  DOLLAR SIGN                             */
    165,        /*  3      ¥  YEN SIGN                                */
    232,        /*  4      è  LATIN SMALL LETTER E WITH GRAVE         */
    233,        /*  5      é  LATIN SMALL LETTER E WITH ACUTE         */
    249,        /*  6      ù  LATIN SMALL LETTER U WITH GRAVE         */
    236,        /*  7      ì  LATIN SMALL LETTER I WITH GRAVE         */
    242,        /*  8      ò  LATIN SMALL LETTER O WITH GRAVE         */
    199,        /*  9      Ç  LATIN CAPITAL LETTER C WITH CEDILLA     */
    10,         /*  10        LINE FEED                               */
    216,        /*  11     Ø  LATIN CAPITAL LETTER O WITH STROKE      */
    248,        /*  12     ø  LATIN SMALL LETTER O WITH STROKE        */
    13,         /*  13        CARRIAGE RETURN                         */
    197,        /*  14     Å  LATIN CAPITAL LETTER A WITH RING ABOVE  */
    229,        /*  15     å  LATIN SMALL LETTER A WITH RING ABOVE    */
    NPC8,       /*  16        GREEK CAPITAL LETTER DELTA              */
    95,         /*  17     _  LOW LINE                                */
    NPC8,       /*  18        GREEK CAPITAL LETTER PHI                */
    NPC8,       /*  19        GREEK CAPITAL LETTER GAMMA              */
    NPC8,       /*  20        GREEK CAPITAL LETTER LAMBDA             */
    NPC8,       /*  21        GREEK CAPITAL LETTER OMEGA              */
    NPC8,       /*  22        GREEK CAPITAL LETTER PI                 */
    NPC8,       /*  23        GREEK CAPITAL LETTER PSI                */
    NPC8,       /*  24        GREEK CAPITAL LETTER SIGMA              */
    NPC8,       /*  25        GREEK CAPITAL LETTER THETA              */
    NPC8,       /*  26        GREEK CAPITAL LETTER XI                 */
    27,         /*  27        ESCAPE TO EXTENSION TABLE               */
    198,        /*  28     Æ  LATIN CAPITAL LETTER AE                 */
    230,        /*  29     æ  LATIN SMALL LETTER AE                   */
    223,        /*  30     ß  LATIN SMALL LETTER SHARP S (German)     */
    201,        /*  31     É  LATIN CAPITAL LETTER E WITH ACUTE       */
    32,         /*  32        SPACE                                   */
    33,         /*  33     !  EXCLAMATION MARK                        */
    34,         /*  34     "  QUOTATION MARK                          */
    35,         /*  35     #  NUMBER SIGN                             */
    164,        /*  36     €  CURRENCY SIGN                           */
    37,         /*  37     %  PERCENT SIGN                            */
    38,         /*  38     &  AMPERSAND                               */
    39,         /*  39     '  APOSTROPHE                              */
    40,         /*  40     (  LEFT PARENTHESIS                        */
    41,         /*  41     )  RIGHT PARENTHESIS                       */
    42,         /*  42     *  ASTERISK                                */
    43,         /*  43     +  PLUS SIGN                               */
    44,         /*  44     ,  COMMA                                   */
    45,         /*  45     -  HYPHEN-MINUS                            */
    46,         /*  46     .  FULL STOP                               */
    47,         /*  47     /  SOLIDUS (SLASH)                         */
    48,         /*  48     0  DIGIT ZERO                              */
    49,         /*  49     1  DIGIT ONE                               */
    50,         /*  50     2  DIGIT TWO                               */
    51,         /*  51     3  DIGIT THREE                             */
    52,         /*  52     4  DIGIT FOUR                              */
    53,         /*  53     5  DIGIT FIVE                              */
    54,         /*  54     6  DIGIT SIX                               */
    55,         /*  55     7  DIGIT SEVEN                             */
    56,         /*  56     8  DIGIT EIGHT                             */
    57,         /*  57     9  DIGIT NINE                              */
    58,         /*  58     :  COLON                                   */
    59,         /*  59     ;  SEMICOLON                               */
    60,         /*  60     <  LESS-THAN SIGN                          */
    61,         /*  61     =  EQUALS SIGN                             */
    62,         /*  62     >  GREATER-THAN SIGN                       */
    63,         /*  63     ?  QUESTION MARK                           */
    161,        /*  64     ¡  INVERTED EXCLAMATION MARK               */
    65,         /*  65     A  LATIN CAPITAL LETTER A                  */
    66,         /*  66     B  LATIN CAPITAL LETTER B                  */
    67,         /*  67     C  LATIN CAPITAL LETTER C                  */
    68,         /*  68     D  LATIN CAPITAL LETTER D                  */
    69,         /*  69     E  LATIN CAPITAL LETTER E                  */
    70,         /*  70     F  LATIN CAPITAL LETTER F                  */
    71,         /*  71     G  LATIN CAPITAL LETTER G                  */
    72,         /*  72     H  LATIN CAPITAL LETTER H                  */
    73,         /*  73     I  LATIN CAPITAL LETTER I                  */
    74,         /*  74     J  LATIN CAPITAL LETTER J                  */
    75,         /*  75     K  LATIN CAPITAL LETTER K                  */
    76,         /*  76     L  LATIN CAPITAL LETTER L                  */
    77,         /*  77     M  LATIN CAPITAL LETTER M                  */
    78,         /*  78     N  LATIN CAPITAL LETTER N                  */
    79,         /*  79     O  LATIN CAPITAL LETTER O                  */
    80,         /*  80     P  LATIN CAPITAL LETTER P                  */
    81,         /*  81     Q  LATIN CAPITAL LETTER Q                  */
    82,         /*  82     R  LATIN CAPITAL LETTER R                  */
    83,         /*  83     S  LATIN CAPITAL LETTER S                  */
    84,         /*  84     T  LATIN CAPITAL LETTER T                  */
    85,         /*  85     U  LATIN CAPITAL LETTER U                  */
    86,         /*  86     V  LATIN CAPITAL LETTER V                  */
    87,         /*  87     W  LATIN CAPITAL LETTER W                  */
    88,         /*  88     X  LATIN CAPITAL LETTER X                  */
    89,         /*  89     Y  LATIN CAPITAL LETTER Y                  */
    90,         /*  90     Z  LATIN CAPITAL LETTER Z                  */
    196,        /*  91     Ä  LATIN CAPITAL LETTER A WITH DIAERESIS   */
    214,        /*  92     Ö  LATIN CAPITAL LETTER O WITH DIAERESIS   */
    209,        /*  93     Ñ  LATIN CAPITAL LETTER N WITH TILDE       */
    220,        /*  94     Ü  LATIN CAPITAL LETTER U WITH DIAERESIS   */
    167,        /*  95     §  SECTION SIGN                            */
    191,        /*  96     ¿  INVERTED QUESTION MARK                  */
    97,         /*  97     a  LATIN SMALL LETTER A                    */
    98,         /*  98     b  LATIN SMALL LETTER B                    */
    99,         /*  99     c  LATIN SMALL LETTER C                    */
    100,        /*  100    d  LATIN SMALL LETTER D                    */
    101,        /*  101    e  LATIN SMALL LETTER E                    */
    102,        /*  102    f  LATIN SMALL LETTER F                    */
    103,        /*  103    g  LATIN SMALL LETTER G                    */
    104,        /*  104    h  LATIN SMALL LETTER H                    */
    105,        /*  105    i  LATIN SMALL LETTER I                    */
    106,        /*  106    j  LATIN SMALL LETTER J                    */
    107,        /*  107    k  LATIN SMALL LETTER K                    */
    108,        /*  108    l  LATIN SMALL LETTER L                    */
    109,        /*  109    m  LATIN SMALL LETTER M                    */
    110,        /*  110    n  LATIN SMALL LETTER N                    */
    111,        /*  111    o  LATIN SMALL LETTER O                    */
    112,        /*  112    p  LATIN SMALL LETTER P                    */
    113,        /*  113    q  LATIN SMALL LETTER Q                    */
    114,        /*  114    r  LATIN SMALL LETTER R                    */
    115,        /*  115    s  LATIN SMALL LETTER S                    */
    116,        /*  116    t  LATIN SMALL LETTER T                    */
    117,        /*  117    u  LATIN SMALL LETTER U                    */
    118,        /*  118    v  LATIN SMALL LETTER V                    */
    119,        /*  119    w  LATIN SMALL LETTER W                    */
    120,        /*  120    x  LATIN SMALL LETTER X                    */
    121,        /*  121    y  LATIN SMALL LETTER Y                    */
    122,        /*  122    z  LATIN SMALL LETTER Z                    */
    228,        /*  123    ä  LATIN SMALL LETTER A WITH DIAERESIS     */
    246,        /*  124    ö  LATIN SMALL LETTER O WITH DIAERESIS     */
    241,        /*  125    ñ  LATIN SMALL LETTER N WITH TILDE         */
    252,        /*  126    ü  LATIN SMALL LETTER U WITH DIAERESIS     */
    224         /*  127    à  LATIN SMALL LETTER A WITH GRAVE         */

/*  The double bytes below must be handled separately after the
    table lookup.

    12             27 10      FORM FEED
    94             27 20   ^  CIRCUMFLEX ACCENT
    123            27 40   {  LEFT CURLY BRACKET
    125            27 41   }  RIGHT CURLY BRACKET
    92             27 47   \  REVERSE SOLIDUS (BACKSLASH)
    91             27 60   [  LEFT SQUARE BRACKET
    126            27 61   ~  TILDE
    93             27 62   ]  RIGHT SQUARE BRACKET
    124            27 64   |  VERTICAL BAR                             */

};


static inline int readbyte(const char* buf, int pos)
{
  uint8_t* b = (uint8_t*) buf + pos*2;
  int v, i;

  v = 0;
  for (i = 0; i < 2; ++i)
  {
    int shift = i==0 ? 4 : 0;
    if (b[i] >= '0' && b[i] <= '9')
      v |= (b[i]-'0') << shift;
    else if (b[i] >= 'a' && b[i] <= 'f')
      v |= (b[i]-'a'+10) << shift;
    else if (b[i] >= 'A' && b[i] <= 'F')
      v |= (b[i]-'A'+10) << shift;
    else
    {
      // error in the hex string !
      printf("SMS PDU Parser, readbyte error in parsing: non hex digit found in the buffer\n");
    }
  }

  return v;
}

static inline void writebyte(int val, char* buf, int pos)
{
  uint8_t* b = (uint8_t*) buf + pos*2;
  int i;

  for (i = 0; i < 2; ++i)
  {
    int shift = i==0 ? 4 : 0;
    int v = (val >> shift) & 0xF;

    *b = ((v<=9)?'0':'A'-10) + v;

    b++;
  }
}

static inline void hextobin(const char* hex, int pos, int len, char* bin)
{
  int j;
  for (j = pos; j < len+pos; ++j)
    *bin++ = readbyte(hex, j);
}

/*static inline*/ void bintohex(const char* bin, int len, char* hex, int pos)
{
  int j;
  for (j = pos; j < len+pos; ++j)
    writebyte(*bin++, hex, j);
}


static inline unsigned int read7bits(const char* buffer, unsigned int pos)
{
  int a = readbyte(buffer, pos/8)>>(pos&7);
  int b = 0;
  if ((pos&7) > 1)
    b = readbyte(buffer, pos/8+1)<<(8-(pos&7));

  return (a|b) & 0x7F;
}

static inline void write7bits(char* buffer, unsigned int val, unsigned int pos)
{

  val &= 0x7F;

  if (!(pos&7))
    writebyte(val, buffer, pos/8);
  else if ((pos&7) == 1)
    writebyte(readbyte(buffer, pos/8) | (val<<1), buffer, pos/8);
  else
  {
    writebyte(readbyte(buffer, pos/8) | (val<<(pos&7)), buffer, pos/8);
    writebyte(val >> (8-(pos&7)), buffer, pos/8 +1);
  }
}

/**
 * Convert a 7bit string into an ascii string
 * a7bit is the hex buffer (hex digits are written in ASCII)
 * length is the number of 7bit chars in the a7bit buffer
 * return the size of the ASCII string. No ending null charater is added !
 * ascii pointer may be set to null when only the returned size is needed
 */
int convert_7bit_to_ascii(const char *a7bit, int pos, int length, char *ascii)
{
  int r;
  int w;

  w = 0;
  for (r = pos; r < length+pos; r++)
  {

    unsigned int byte = read7bits(a7bit, r*7);
    byte = lookup_ascii7to8[byte];

    if (byte != 27)
    {
      if (ascii)
        ascii[w] = byte;
      w++;
    }
    else
    {
      /* If we're escaped then the next byte have a special meaning. */
      r++;
      byte = lookup_ascii7to8[read7bits(a7bit, r*7)];
      if (ascii)
      {
        switch (byte)
        {
          case 10:
            ascii[w] = 12;
            break;
          case 20:
            ascii[w] = '^';
            break;
          case 40:
            ascii[w] = '{';
            break;
          case 41:
            ascii[w] = '}';
            break;
          case 47:
            ascii[w] = '\\';
            break;
          case 60:
            ascii[w] = '[';
            break;
          case 61:
            ascii[w] = '~';
            break;
          case 62:
            ascii[w] = ']';
            break;
          case 64:
            ascii[w] = '|';
            break;
          default:
            ascii[w] = NPC8;
            break;
        }
      }
      w++;
    }
  }

  return w;
}

/**
 * Convert an ASCII string into a 7bit string
 * length is the number of bytes in the ASCII buffer
 * return the size of the a7bit string (in 7bit chars!).
 * a7bit pointer may be set to null when only the returned size is needed
 */
int convert_ascii_to_7bit(const char *ascii, int pos, int length, char *a7bit)
{
  int r;
  int w;

  w = 0;

  for (r = pos; r < length+pos; ++r)
  {
    unsigned int byte = lookup_ascii8to7[(uint8_t)ascii[r]];
    if (byte < 128)
    {
      if (a7bit)
        write7bits(a7bit, byte, w*7);
      w++;
    }
    else
    {
      if (a7bit)
      {
        byte -= 128;
        write7bits(a7bit, 27, w*7);
        write7bits(a7bit, byte, w*7+7);
      }
      w += 2;
    }
  }

  return w; // the number of 7bit chars
}

// Some terminal do not include the SMSC information in the PDU format string. In that case, the following define must be removed
#define HAS_SMSC_INFORMATION

int decode_smspdu(const char* pdu, SMS** sms_)
{
  int pos = 0;

  // those values are needed for concatenated (=long) sms
  int concat_ref = 0;
  int concat_maxnb = 1;
  int concat_seqnb = 1;
  // Port values
  int portbits = 0;
  int srcport = 0;
  int dstport = 0;

#ifdef HAS_SMSC_INFORMATION
  int smsc_info_length = readbyte(pdu, pos++);
  if (smsc_info_length>0)
  {
    readbyte(pdu, pos++);
  }
  pos += smsc_info_length-1; // skip address
#endif

  int sms_deliver = readbyte(pdu, pos++);
  int sender_address_length = readbyte(pdu, pos++);
  int sender_toa = readbyte(pdu, pos++);
  const char* sender = pdu + 2*pos;

  pos += (sender_address_length+1) >> 1; // skip address

  /*int tp_pid =*/ readbyte(pdu, pos++);
  int tp_dcs = readbyte(pdu, pos++);
  const char* timestamp = pdu + 2*pos;
  pos += 7; // skip timestamp

  int tp_udl = readbyte(pdu, pos++);

  // check that we have a supported message type ( 7- or 8-bits characters)
  int encoding;
  if ((tp_dcs >> 6) == 0)
  {
    encoding = ((tp_dcs >> 2) & 3);
    if ( encoding != 1 && encoding != 0)
      return -2;
  }
  else if ((tp_dcs >> 4) == 0xF)
  {
    encoding = (tp_dcs >> 2) & 1;
  }
  else
    return -1;

  pdu += 2*pos; // reset the pdu pointer to the beginning of user data
  pos = 0;
  // Check if we have a user data header
  int message_length; // message length in character (7- or 8-bit depending on the encoding)
  int tp_udhi = sms_deliver & (1<<6);
  int tp_udhl = tp_udhi ? readbyte(pdu, pos++) : 0;
  if (tp_udhl)
  {
    int c = tp_udhl;
    while (c>0)
    {
      int id = readbyte(pdu, pos++);
      int l = readbyte(pdu, pos++);

      if (id == 0) // Check if we have a concatenated (=long) sms (8bits ref)
      {
        concat_ref = readbyte(pdu, pos);
        concat_maxnb = readbyte(pdu, pos+1);
        concat_seqnb = readbyte(pdu, pos+2);
      }

      else if (id == 8) // Check if we have a concatenated (=long) sms  (16bits ref)
      {
        concat_ref = (readbyte(pdu, pos)<<8) + readbyte(pdu, pos+1);
        concat_maxnb = readbyte(pdu, pos+2);
        concat_seqnb = readbyte(pdu, pos+3);
      }
      else if (id == 4) // The SMS has src/dst ports (8bits)
      {
        portbits = 8;
        dstport = readbyte(pdu, pos);
        srcport = readbyte(pdu, pos+1);
      }
      else if (id == 5) // The SMS has src/dst ports (16bits)
      {
        portbits = 16;
        dstport = (readbyte(pdu, pos)<<8) + readbyte(pdu, pos+1);
        srcport = (readbyte(pdu, pos+2)<<8) + readbyte(pdu, pos+3);
      }
      else
      {
        printf("Unsupported SMS Information-Element-Identifier: %d (ignored)\n", id);
      }

      c -= l+2;
      pos += l;
    }
    tp_udhl++; // add the length byte !
  }
  if (encoding == 1) // 8bits chars
    message_length = tp_udl-tp_udhl;
  else // encoding == 0 // 7bits chars
  {
    message_length = (tp_udl*7 - tp_udhl*8) /7;
    if (message_length <= 0)
      return -11;
    pos = (tp_udhl*8+6)/7; // translate the pos in 7bits char unit
  }


  // now, copy / convert the data into the SMS structure
  SMS* sms = malloc(sizeof(*sms) + message_length +1 + sender_address_length+2);
  if (!sms)
    return -10;
  sms->message = (char*) (sms+1);
  sms->address = sms->message + message_length+1;
  memcpy(sms->timestamp, timestamp, 14);

  if (encoding == 1) // 8bits chars
  {
    hextobin(pdu, pos, message_length, sms->message);
    sms->message_length = message_length;
  }
  else // encoding == 0 // 7bits chars
    sms->message_length = convert_7bit_to_ascii(pdu, pos, message_length, sms->message);

  sms->message[message_length] = 0; // add a null char, cannot do harm...

  char* s = sms->address;
  int typeofnumber = ((sender_toa>>4)&7);
  int i;
  if (typeofnumber == 5) // alphanumeric, 7-bit chars
  {
    i = convert_7bit_to_ascii(sender, 0, (sender_address_length*4)/7, s);
  }
  else
  {
    if (typeofnumber == 1) // international
    {
      s[0] = '+';
      s++;
    }
    for (i = 0; i < sender_address_length; i++)
    {
      s[i] = sender[(i&~1)+1-(i&1)];
    }
  }
  s[i] = 0;


  sms->concat_ref = concat_ref;
  sms->concat_maxnb = concat_maxnb;
  sms->concat_seqnb = concat_seqnb;

  sms->portbits = portbits;
  sms->dstport = dstport;
  sms->srcport = srcport;

  *sms_ = sms;

  return 0;
}

void free_sms(SMS* sms)
{
  free(sms);
}

/*
 * return <0 when an error happened, otherwise the number of SMS that are needed to send this message
 */
#define SMS_BUFFER_SIZE ((140+10+20+1)*2) // payload + header + phone number, *2 because it is written in hex semi-octet
#define min(a, b) ((a)<(b) ? (a) : (b))
int encode_smspdu(const char* message, int length, const char* address,  PDU** pdu_)
{
  int max_sms_length = 140;
  int nb_of_sms = 1;
  int n;
  int concat_ref_lb=0, concat_ref_hb=0;
  int smsc_size = 0;

  // Maximum size of a concatenated SMS
  if (length > 255*133)
    return -100;

  if (length > max_sms_length)
  {
    max_sms_length -= 7;
    nb_of_sms = (length+max_sms_length-1) / max_sms_length;
    concat_ref_lb = rand() & 0xFF;
    concat_ref_hb = rand() & 0xFF;
  }

  // First byte of the SMS-DELIVER SMS
  int tp_udhi = nb_of_sms>1;
  int sms_deliver_byte = (tp_udhi<<6) | 0x11;

  int address_len = strlen(address);
  if (address_len>20)
    return -1;
  int number_toa;

  char number[20];
  if (address[0] == '+') // international phone number
  {
    address_len--;
    memcpy(number, address+1, address_len);
    number_toa = 0x91;
  }
  else
  {
    memcpy(number, address, address_len);
    number_toa = 0x81;
  }

  if (address_len%2)
    number[address_len] = 'F';

  int tp_dcs = 4; // 8-bit chars only !


  // allocate the memory needed for all sms'
  char* buffer = malloc(nb_of_sms*(SMS_BUFFER_SIZE + sizeof(PDU))); // + the array of pointers
  if (!buffer)
    return -10;
  PDU* p = (PDU*)buffer;

  for (n=0; n<nb_of_sms; n++)
  {
    int pos = 0;
    p[n].buffer = buffer + (SMS_BUFFER_SIZE * n) + sizeof(PDU)*nb_of_sms ;
    char* pdu = p[n].buffer ;

#ifdef HAS_SMSC_INFORMATION
    writebyte(0, pdu, pos++); // use default SMSC information
    smsc_size = 1;
#endif //HAS_SMSC_INFORMATION

    // First octet of the SMS-DELIVER SMS
    writebyte(sms_deliver_byte, pdu, pos++);

    // Default TP-Message-Reference
    writebyte(0, pdu, pos++);

    // Address(phone number) length
    writebyte(address_len, pdu, pos++);

    // type-of-address
    writebyte(number_toa, pdu, pos++);

    // Phone number
    {
      int i;
      char* s = pdu+2*pos;
      for (i = 0; i < ((address_len+1)&~1); i++)
      {
        s[i] = number[(i&~1)+1-(i&1)];
      }
    }
    pos += (address_len+1)/2;

    //  TP-PID. Protocol identifier
    writebyte(0, pdu, pos++);

    // TP-DCS. Data coding scheme
    writebyte(tp_dcs, pdu, pos++);

    // TP-Validity-Period: set to 7 days
    writebyte(0xAD, pdu, pos++);

    // User data length
    int message_len = min(length, max_sms_length);
    writebyte(message_len+7*(nb_of_sms>1), pdu, pos++);

    // Write Concat SMS User Header
    if (nb_of_sms>1)
    {
      writebyte(6, pdu, pos++); // UDHL
      writebyte(8, pdu, pos++); // Information-Element-Identifier: Concat SMS
      writebyte(4, pdu, pos++); // Length of Information-Element
      writebyte(concat_ref_lb, pdu, pos++); // Concat ref HB
      writebyte(concat_ref_hb, pdu, pos++); // Concat ref LB
      writebyte(nb_of_sms, pdu, pos++); // Concat max nb
      writebyte(n+1, pdu, pos++); // Concat seq nb
    }

    // Write User data
    bintohex(message, message_len, pdu, pos);
    pos += message_len;
    message += message_len;
    length -= max_sms_length;

    pdu[pos*2] = 0;
    p[n].size = pos-smsc_size;
  }

  *pdu_ = p;
  return nb_of_sms;
}

void free_pdu(void* pdu)
{
  free(pdu);
}

#ifdef FOR_LUA
/*
 * lua interface:
 *  encodePdu(phoneNumber, message)
 *  returns a table t with one or more PDU data. (several PDU data are returned when the SMS need to be concatenated)
 *  t[k].size: size of the pdu that needs to be given to the at command
 *  t[k].buffer: ascii hex encoded pdu to give to the at command
 */
static int l_encodePdu(lua_State *L)
{
  const char* number;
  const char* message;
  size_t message_len;
  PDU* pdu;
  int nb, i;

  number = luaL_checkstring(L, 1);
  message = luaL_checklstring(L, 2, &message_len);

  nb = encode_smspdu(message, message_len, number, &pdu);
  if (nb < 0)
  {
    lua_pushnil(L);
    lua_pushfstring(L, "An error occurred when encoding the PDU: error=%d", nb);
    return 2;
  }

  lua_newtable(L);

  for (i = 0; i < nb; ++i)
  {
    lua_newtable(L);
    lua_pushinteger(L, pdu[i].size);
    lua_setfield(L, -2, "size");
    lua_pushstring(L, pdu[i].buffer);
    lua_setfield(L, -2, "buffer");

    lua_rawseti(L, -2, i+1);
  }

  free_pdu(pdu);

  return 1;
}

/*
 * lua interface:
 *  decodePdu(pdubuffer)
 *  returns a table with the following fields:
 *    address
 *    timestamp; // a binary element that conforms to the SMS PDU timestamp format
 *    message; // binary string of the message content
 *
 *  When the PDU is part of a concatenated SMS, some additional fields are present:
 *    concat.ref;
 *    concat.maxnb;
 *    concat.seqnb;
 */
static int l_decodePdu(lua_State *L)
{
  SMS* sms;
  const char* pdu;
  int status;

  pdu = luaL_checkstring(L, 1);

  status = decode_smspdu(pdu, &sms);
  if (status < 0)
  {
    lua_pushnil(L);
    lua_pushfstring(L, "An error occurred when decoding the PDU: error=%d", status);
    return 2;
  }

  lua_newtable(L);
  lua_pushstring(L, sms->address);
  lua_setfield(L, -2, "address");
  lua_pushlstring(L, sms->timestamp, 14);
  lua_setfield(L, -2, "timestamp");
  lua_pushlstring(L, sms->message, sms->message_length);
  lua_setfield(L, -2, "message");

  if (sms->concat_maxnb > 1)
  {
    lua_newtable(L);
    lua_pushinteger(L, sms->concat_ref);
    lua_setfield(L, -2, "ref");
    lua_pushinteger(L, sms->concat_maxnb);
    lua_setfield(L, -2, "maxnb");
    lua_pushinteger(L, sms->concat_seqnb);
    lua_setfield(L, -2, "seqnb");

    lua_setfield(L, -2, "concat");
  }

  if (sms->portbits)
  {
    lua_newtable(L);
    lua_pushinteger(L, sms->portbits);
    lua_setfield(L, -2, "bits");
    lua_pushinteger(L, sms->dstport);
    lua_setfield(L, -2, "dst");
    lua_pushinteger(L, sms->srcport);
    lua_setfield(L, -2, "src");

    lua_setfield(L, -2, "ports");
  }


  free_sms(sms);
  return 1;
}

static const luaL_reg R[] =
{
  { "encodePdu", l_encodePdu },
  { "decodePdu", l_decodePdu },
  { NULL, NULL}
};

SMS_API int luaopen_smspdu(lua_State *L)
{
  luaL_register(L, "smspdu", R);
  return 1;
}
#endif



