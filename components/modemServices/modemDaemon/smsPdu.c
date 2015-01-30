/** @file smsPdu.c
 *
 * Source code of functions to interact with SMS PDU data.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include <time.h>
#include "legato.h"
#include "smsPdu.h"
#include "cdmaPdu.h"

//--------------------------------------------------------------------------------------------------
/**
 * Trace reference used for controlling tracing in this module.
 */
//--------------------------------------------------------------------------------------------------
static le_log_TraceRef_t TraceRef;

/// Macro used to generate trace output in this module.
/// Takes the same parameters as LE_DEBUG() et. al.
#define TRACE(...) LE_TRACE(TraceRef, ##__VA_ARGS__)

/// Macro used to query current trace state in this module
#define IS_TRACE_ENABLED LE_IS_TRACE_ENABLED(TraceRef)


/* Define Non-Printable Characters as a question mark */
#define NPC7    63
#define NPC8    '?'

#ifndef min
# define min(a, b) ((a)<(b) ? (a) : (b))
#endif

/* C.S0005-D v2.0 Table 2.7.1.3.2.4-4. Representation of DTMF Digits */
static const char *DtmfChars = "D1234567890*#ABC";

/**
 * Some terminal do not include the SMSC information in the PDU format string.
 * In that case, the following define must be removed
 */
#define HAS_SMSC_INFORMATION

/****************************************************************************
 * This lookup table converts from ISO-8859-1 8-bit ASCII to the
 * 7 bit "default alphabet" as defined in ETSI GSM 03.38
 *
 * ISO-characters that don't have any corresponding character in the
 * 7-bit alphabet is replaced with the NPC7-character.  If there's
 * a close match between the ISO-char and a 7-bit character (for example
 * the letter i with a circumflex and the plain i-character) a substitution
 * is done.
 *
 * There are some character (for example the square brace "]") that must
 * be converted into a 2 byte 7-bit sequence.  These characters are
 * marked in the table by having 128 added to its value.
 ****************************************************************************/
const uint8_t Ascii8to7[] = {
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
    32,         /*   160      non-breaking space                      */
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
 *  This lookup table converts from the 7 bit "default alphabet" as
 *   defined in ETSI GSM 03.38 to a standard ISO-8859-1 8-bit ASCII.
 *
 *   Some characters in the 7-bit alphabet does not exist in the ISO
 *   character set, they are replaced by the NPC8-character.
 *
 *   If the character is decimal 27 (ESC) the following character have
 *   a special meaning and must be handled separately.
 ****************************************************************************/
const uint8_t Ascii7to8[] = {
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
     *   table lookup.
     *
     *   12             27 10      FORM FEED
     *   94             27 20   ^  CIRCUMFLEX ACCENT
     *   123            27 40   {  LEFT CURLY BRACKET
     *   125            27 41   }  RIGHT CURLY BRACKET
     *   92             27 47   \  REVERSE SOLIDUS (BACKSLASH)
     *   91             27 60   [  LEFT SQUARE BRACKET
     *   126            27 61   ~  TILDE
     *   93             27 62   ]  RIGHT SQUARE BRACKET
     *   124            27 64   |  VERTICAL BAR                             */

};

//--------------------------------------------------------------------------------------------------
/**
 * Dump the PDU
 */
//--------------------------------------------------------------------------------------------------
static void DumpPdu
(
    const char      *label,     ///< [IN] label
    const uint8_t   *buffer,    ///< [IN] buffer
    size_t           bufferSize ///< [IN] buffer size
)
{
    if ( IS_TRACE_ENABLED )
    {
        uint32_t index = 0, i = 0;
        char output[65] = {0};

        LE_DEBUG("%s:",label);
        for (i=0;i<bufferSize;i++)
        {
            index += sprintf(&output[index],"%02X",buffer[i]);
            if ( !((i+1)%32) )
            {
                LE_DEBUG("%s",output);
                index = 0;
            }
        }
        LE_DEBUG("%s",output);
    }
}


static inline unsigned int Read7Bits
(
    const uint8_t* bufferPtr,
    uint32_t       pos
)
{
    int a = bufferPtr[pos/8] >> (pos&7);
    int b = 0;
    if ((pos&7) > 1) {
        b = bufferPtr[(pos/8)+1] << (8-(pos&7));
    }

    return (a|b) & 0x7F;
}

static inline void Write7Bits
(
    uint8_t* bufferPtr,
    uint8_t  val,
    uint32_t pos
)
{
    val &= 0x7F;
    uint8_t idx = pos/8;


    if (!(pos&7)) {
        bufferPtr[idx] = val;
    }
    else if ((pos&7) == 1) {
        bufferPtr[idx] = bufferPtr[idx] | (val<<1);
    }
    else {
        bufferPtr[idx] = bufferPtr[idx] | (val<<(pos&7));
        bufferPtr[idx+1] = (val>>(8-(pos&7)));
    }
}

static inline unsigned int ReadCdma7Bits
(
    const uint8_t* bufferPtr,
    uint32_t       pos
)
{
    uint8_t idx = pos/8;

    return (((bufferPtr[idx]<<(pos&7))&0xFF)|(bufferPtr[idx+1]>>(8-(pos&7))))>>1;
}

static inline void WriteCdma7Bits
(
    uint8_t* bufferPtr,
    uint8_t  val,
    uint32_t pos
)
{
    val &= 0x7F;
    uint8_t idx = pos/8;

    if (!(pos&7)) {
        bufferPtr[idx] = val << 1;
    }
    else if ((pos&7) == 1) {
        bufferPtr[idx] = bufferPtr[idx] | val;
    }
    else {
        bufferPtr[idx] = bufferPtr[idx] | (val>>((pos&7)-1));
        bufferPtr[idx+1] = ((val<<(8-((pos&7)-1)))&0xff);
    }
}

/**
 * Convert an ascii array into a 7bits array
 * length is the number of bytes in the ascii buffer
 *
 * @return the size of the a7bit string (in 7bit chars!), or LE_OVERFLOW if a7bitPtr is too small.
 */
static int32_t Convert8BitsTo7Bits
(
    const uint8_t *a8bitPtr,    ///< [IN] 8bits array to convert
    int            pos,         ///< [IN] starting position of conversion
    int            length,      ///< [IN] size of 8bits byte conversion
    uint8_t       *a7bitPtr,    ///< [OUT] 7bits array result
    size_t         a7bitSize,   ///< [IN] 7bits array size
    uint8_t       *a7bitsNumber ///< [OUT] number of char in &7bitsPtr
)
{
    int read;
    int write = 0;
    int size = 0;

    for (read = pos; read < length+pos; ++read)
    {
        uint8_t byte = Ascii8to7[a8bitPtr[read]];

        /* Escape */
        if (byte >= 128)
        {
            if (size>a7bitSize)
            {
                return LE_OVERFLOW;
            }

            Write7Bits(a7bitPtr, 0x1B, write*7);
            write++;
            byte -= 128;
        }

        if (size>a7bitSize)
        {
            return LE_OVERFLOW;
        }

        Write7Bits(a7bitPtr, byte, write*7);
        write++;

        /* Number of 8 bit chars */
        size = (write * 7);
        size = (size % 8 != 0) ? (size/8 + 1) : (size/8);
    }

    if (size>a7bitSize)
    {
        return LE_OVERFLOW;
    }

    /* Number of written chars */
    *a7bitsNumber = write;

    return size;
}

/**
 * Convert a 7bit array into a ascii array
 * length is the number of 7bit char in the a7bit buffer
 *
 * @return the size of the ascii array, of LE_OVERFLOW if a8bitPtr is too small.
 */
static int32_t Convert7BitsTo8Bits
(
    const uint8_t *a7bitPtr,     ///< [IN] 7bits array to convert
    int            pos,          ///< [IN] startin position of conversion
    int            length,       ///< [IN] size of 7bits byte convertion
    uint8_t       *a8bitPtr,     ///< [OUT] 8bits array restul
    size_t         a8bitSize     ///< [IN] 8bits array size.
)
{
    int r;
    int w;

    w = 0;
    for (r = pos; r < length+pos; r++)
    {
        uint8_t byte = Read7Bits(a7bitPtr, r*7);
        byte = Ascii7to8[byte];

        if (byte != 27)
        {
            if (w < a8bitSize)
            {
                a8bitPtr[w] = byte;
                w++;
            }
            else
            {
                return LE_OVERFLOW;
            }
        }
        else
        {
            /* If we're escaped then the next byte have a special meaning. */
            r++;

            byte = Read7Bits(a7bitPtr, r*7);
            if (w < a8bitSize)
            {
                switch (byte)
                {
                    case 10:
                        a8bitPtr[w] = 12;
                        break;
                    case 20:
                        a8bitPtr[w] = '^';
                        break;
                    case 40:
                        a8bitPtr[w] = '{';
                        break;
                    case 41:
                        a8bitPtr[w] = '}';
                        break;
                    case 47:
                        a8bitPtr[w] = '\\';
                        break;
                    case 60:
                        a8bitPtr[w] = '[';
                        break;
                    case 61:
                        a8bitPtr[w] = '~';
                        break;
                    case 62:
                        a8bitPtr[w] = ']';
                        break;
                    case 64:
                        a8bitPtr[w] = '|';
                        break;
                    default:
                        a8bitPtr[w] = NPC8;
                        break;
                }
                w++;
            }
            else
            {
                return LE_OVERFLOW;
            }
        }
    }

    return w;
}

static inline uint8_t ReadByte
(
    const uint8_t* bufPtr,
    uint8_t        pos
)
{
    return bufPtr[pos];
}

static inline void WriteByte
(
    uint8_t* bufPtr,
    int      pos,
    uint8_t  val
)
{
    bufPtr[pos]=val;
}

static uint32_t ConvertBinaryIntoPhoneNumber
(
    const uint8_t* binPtr,
    uint32_t       binSize,
    char*          phonePtr,
    uint32_t       phoneSize
)
{
    uint32_t idx;

    if (phoneSize < 2*binSize+1)
    {
        return -1;
    }

    uint8_t pos=0;
    uint8_t phoneLength = binPtr[0];
    uint8_t toa = binPtr[1];

    if (toa == 0x91) // international phone number
    {
        phonePtr[pos++] = '+';
    }

    le_hex_BinaryToString((const uint8_t*)&binPtr[2],(phoneLength+1)>>1,&phonePtr[pos],phoneSize);

    for(idx=0; idx<binSize;idx++,pos+=2)
    {
        char tmp = phonePtr[pos+1];
        phonePtr[pos+1] = phonePtr[pos];
        phonePtr[pos] = tmp;
    }

    if (phoneLength%2) {
        phonePtr[phoneLength+1] = '\0';
    }
    phonePtr[phoneSize] = '\0';

    return idx+2;
}

static size_t ConvertPhoneNumberIntoBinary
(
    const char* phonePtr,
    uint8_t*    tabPtr,
    size_t      size
)
{
    int i = 0;
    char number[20] = {0};
    size_t phoneLength = strlen(phonePtr);

    if (phonePtr[0] == '+') // International phone number
    {
        phoneLength--;
        phonePtr++;
    }

    for(i = 0; i < phoneLength; i=i+2)
    {
        number[i] = ( (i+1) < phoneLength ) ? phonePtr[i+1] : 'F';
        number[i+1] = phonePtr[i];
    }

    le_hex_StringToBinary(number, strlen(number), tabPtr, size);

    return phoneLength;
}

static uint32_t ConvertBinaryIntoTimestamp
(
    const uint8_t* binPtr,
    uint32_t       binSize,
    char*          timestampPtr,
    uint32_t       timestampSize
)
{
    if (binSize != 7) {
        snprintf(timestampPtr, timestampSize, "xx/xx/xx,xx:xx:xxxxx");
        return 7;
    }

    snprintf(timestampPtr,timestampSize,
            "%u%u/%u%u/%u%u,%u%u:%u%u:%u%u%c%u%u",
            binPtr[0]&0x0F,
            (binPtr[0]>>4)&0x0F,
            binPtr[1]&0x0F,
            (binPtr[1]>>4)&0x0F,
            binPtr[2]&0x0F,
            (binPtr[2]>>4)&0x0F,
            binPtr[3]&0x0F,
            (binPtr[3]>>4)&0x0F,
            binPtr[4]&0x0F,
            (binPtr[4]>>4)&0x0F,
            binPtr[5]&0x0F,
            (binPtr[5]>>4)&0x0F,
            binPtr[6]&0x80?'-':'+',
            binPtr[6]&0x0F,
            (binPtr[6]&0x70)>>4
    );

    return binSize;
}

static smsPdu_Encoding_t DetermineEncoding
(
    uint8_t tp_dcs
)
{
    smsPdu_Encoding_t encoding = SMSPDU_ENCODING_UNKNOWN;

    /* Coding Group Bits 7..4 */
    if ((tp_dcs >> 6) == 0) // 00xx
    {
        encoding = ((tp_dcs >> 2) & 0x3);
    }
    else if ((tp_dcs >> 4) == 0xF) // 1111
    {
        encoding = (tp_dcs >> 2) & 1;
    }
    else
    {
        LE_DEBUG("this encoding is not supported (tp_dcs %u)", tp_dcs);
        return LE_FAULT;
    }

    return encoding;
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode the content of dataPtr.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DecodeGsmPdu
(
    const uint8_t    *dataPtr,  ///< [IN] PDU data to decode
    pa_sms_Message_t *smsPtr    ///< [OUT] Buffer to store decoded data
)
{
    int pos = 0;
    uint8_t firstByte;
    char address[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};
    char timestamp[LE_SMS_TIMESTAMP_MAX_BYTES] = {0};
    uint8_t addressLen;
    uint8_t tp_dcs;
    uint8_t tp_udl;
    smsPdu_Encoding_t encoding;

    memset(smsPtr, 0, sizeof(pa_sms_Message_t));

#ifdef HAS_SMSC_INFORMATION
    uint8_t smscInfoLen = ReadByte(dataPtr,pos++);
    pos += smscInfoLen ; // skip SCA address and type of address
#endif

    firstByte = ReadByte(dataPtr,pos++);

    /* TP-MTI */
    switch(firstByte & 0x03)
    {
        case 0x00:
            smsPtr->type = PA_SMS_SMS_DELIVER;
            smsPtr->smsDeliver.option = PA_SMS_OPTIONMASK_NO_OPTION;
            break;
        case 0x01:
            smsPtr->type = PA_SMS_SMS_SUBMIT;
            smsPtr->smsSubmit.option = PA_SMS_OPTIONMASK_NO_OPTION;
            break;
        default:
            LE_ERROR("Decoding this message is not supported.");
            return LE_UNSUPPORTED;
    }

    if(smsPtr->type == PA_SMS_SMS_SUBMIT)
    {
        /* TP-MR: Message Reference */
        pos++; // skip TP-MR
    }

    /* TP-DA: Destination Address for SMS-SUBMIT */
    /* TP-OA: Originating Address for SMS-DELIVER */
    {
        addressLen = ReadByte(dataPtr,pos);

        pos += ConvertBinaryIntoPhoneNumber(
                    &dataPtr[pos],
                    (addressLen+1)>>1,
                    address,
                    LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    }

    /* TP-PID: Protocol identifier (1 byte) */
    pos++;

    /* TP-DCS: Data Coding Scheme (1 byte) */
    tp_dcs = ReadByte(dataPtr,pos++);

    if(smsPtr->type == PA_SMS_SMS_DELIVER)
    {
        /* TP-SCTS: Service Center Time Stamp (7 bytes) */
        pos += ConvertBinaryIntoTimestamp(
            &dataPtr[pos],
            7,
            timestamp,
            LE_SMS_TIMESTAMP_MAX_BYTES
        );
    }

    // check that we have a supported message type ( 7- or 8-bits characters)
    encoding = DetermineEncoding(tp_dcs);

    switch(encoding)
    {
        case SMSPDU_7_BITS:
        case SMSPDU_8_BITS:
            break;
        default:
            LE_ERROR("Message format not supported.");
            return LE_UNSUPPORTED;
    }

    if(smsPtr->type == PA_SMS_SMS_SUBMIT)
    {
        /* TP-VP: Validity Period (0, 1 or 7 bytes) */
        pos++;
    }

    /* TP-UDL: User Data Length (1 byte) */
    tp_udl = ReadByte(dataPtr,pos++);

    // Check if we have a user data header
    uint8_t messageLen = 0; // message length in character (7- or 8-bit depending on the encoding)
    uint8_t tp_udhi = firstByte & (1<<6);
    uint8_t tp_udhl = tp_udhi ? ReadByte(dataPtr,pos++) : 0;
    if (tp_udhl)
    {
        LE_DEBUG("Multi part SMS are not available yet");
        return LE_UNSUPPORTED;
    }

    uint8_t * destDataPtr;
    size_t    destDataSize;
    uint32_t * destDataLenPtr;
    le_sms_Format_t * formatPtr;

    switch(smsPtr->type)
    {
        case PA_SMS_SMS_DELIVER:
            le_utf8_Copy(smsPtr->smsDeliver.oa, address, sizeof(smsPtr->smsDeliver.oa), NULL);
            smsPtr->smsDeliver.option |= PA_SMS_OPTIONMASK_OA;
            le_utf8_Copy(smsPtr->smsDeliver.scts, timestamp, sizeof(smsPtr->smsDeliver.scts), NULL);
            smsPtr->smsDeliver.option |= PA_SMS_OPTIONMASK_SCTS;

            destDataPtr = smsPtr->smsDeliver.data;
            destDataSize = sizeof(smsPtr->smsDeliver.data);
            destDataLenPtr = &(smsPtr->smsDeliver.dataLen);
            formatPtr = &(smsPtr->smsDeliver.format);
            break;

        case PA_SMS_SMS_SUBMIT:
            le_utf8_Copy(smsPtr->smsSubmit.da, address, sizeof(smsPtr->smsSubmit.da), NULL);
            smsPtr->smsDeliver.option |= PA_SMS_OPTIONMASK_DA;

            destDataPtr = smsPtr->smsSubmit.data;
            destDataSize = sizeof(smsPtr->smsSubmit.data);
            destDataLenPtr = &(smsPtr->smsSubmit.dataLen);
            formatPtr = &(smsPtr->smsSubmit.format);
            break;

        default:
            return LE_FAULT;
    }

    switch(encoding)
    {
        case SMSPDU_8_BITS:
            messageLen = tp_udl-tp_udhl;

            *formatPtr = LE_SMS_FORMAT_BINARY;
            if (messageLen<destDataSize)
            {
                memcpy(destDataPtr,&dataPtr[pos],messageLen);
                *destDataLenPtr = messageLen;
            }
            else
            {
                LE_ERROR("Overflow occurs when converting 8bits to 8bits %d>%zd",
                         messageLen,destDataSize);
                return LE_OVERFLOW;
            }
            break;

        case SMSPDU_7_BITS:
            messageLen = (tp_udl*7 - tp_udhl*8) /7;
            if (messageLen <= 0) {
                LE_ERROR("the message length %d is <0 ",messageLen);
                return LE_FAULT;
            }
            pos -= (tp_udhl*8+6)/7; // translate the pos in 7bits char unit

            *formatPtr = LE_SMS_FORMAT_TEXT;
            int size = Convert7BitsTo8Bits(&dataPtr[pos],
                                            0,messageLen,
                                            destDataPtr,destDataSize);
            if (size == LE_OVERFLOW)
            {
                LE_ERROR("Overflow occurs when converting 7bits to 8bits ");
                return LE_OVERFLOW;
            }
            *destDataLenPtr = size;
            break;

        default:
            LE_ERROR("Decoding error");
            return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Encode the content of messagePtr in PDU format.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EncodeGsmPdu
(
    const uint8_t*      messagePtr,   ///< [IN] Data to encode
    size_t              length,       ///< [IN] Length of data
    const char*         addressPtr,   ///< [IN] Phone Number
    smsPdu_Encoding_t   encoding,     ///< [IN] Type of encoding to be used
    pa_sms_MsgType_t    messageType,  ///< [IN] Message Type
    pa_sms_Pdu_t*       pduPtr        ///< [OUT] Buffer for the encoded PDU
)
{
    const int maxSmsLength = 160;
    uint8_t tp_udhi = 0;
    uint8_t tp_dcs = 0x00;
    uint8_t firstByte = 0x00;
    uint8_t addressToa;

    if (length > maxSmsLength)
    {
        LE_DEBUG("Message cannot be encoded, message with length > %d are not supported yet",
                 maxSmsLength);
        return LE_FAULT;
    }

    /* First Byte:
     * 1-0 TP-Message-Type-Indicator (TP-MTI)
     * 2   TP-More-Messages-to-Send (TP-MMS) in SMS-DELIVER (0 = more messages)
     * 2   TP-Reject-Duplicates (TP-RD) in SMS-SUBMIT
     * 3   TP-Loop-Prevention (TP-LP) in SMS-DELIVER and SMS-STATUS-REPORT
     * 4-3 TP-Validity-Period-Format (TP-VPF) in SMS-SUBMIT (00 = not present)
     * 5   TP-Status-Report-Indication (TP-SRI) in SMS-DELIVER
     * 5   TP-Status-Report-Request (TP-SRR) in SMS-SUBMIT and SMS-COMMAND
     * 5   TP-Status-Report-Qualifier (TP-SRQ) in SMS-STATUS-REPORT
     * 6   TP-User-Data-Header-Indicator (TP-UDHI)
     * 7   TP-Reply-Path (TP-RP) in SMS-DELIVER and SMS-SUBMIT
     */
    switch(messageType)
    {
        case PA_SMS_SMS_DELIVER:
            firstByte = 0x00; // MTI (00)
            firstByte |= tp_udhi << 6;
            break;

        case PA_SMS_SMS_SUBMIT:
            firstByte = 0x11; // MTI (01) | VPF (10)
            firstByte |= tp_udhi << 6;
            break;

        default:
            LE_WARN("Message Type not supported");
            return LE_UNSUPPORTED;
    }

    /* Prepare address */
    int addressLen = strlen(addressPtr);
    if (addressLen > LE_MDMDEFS_PHONE_NUM_MAX_BYTES) {
        LE_DEBUG("Address is too long %d. should be at max %d",
                 addressLen, LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1);
        return LE_FAULT;
    }

    /* Prepare type of address: EXT, TON (Type of number), NPI (Numbering plan identification) */
    if (addressPtr[0] == '+')
    {
        /* TON International phone number: EXT=0b1 TON=0b001 NPI=0b0001 */
        addressLen--;
        addressToa = 0x91;
    }
    else
    {
        /* TON Unknown: EXT=0b1 TON=0b000 NPI=0b0001 */
        addressToa = 0x81;
    }

    /* Prepare DCS */
    switch(encoding)
    {
        case SMSPDU_7_BITS: // GSM 7 bits encoding (GSM 03.38)
            tp_dcs = 0x00;
            break;
        case SMSPDU_8_BITS:
            tp_dcs = 0x04;
            break;
        case SMSPDU_UCS_2:
            LE_ERROR("UCS-2 encoding is not supported.");
            return LE_UNSUPPORTED;
        default:
            LE_ERROR("Invalid encoding %d.", encoding);
            return LE_FAULT;
    }

    /* Prepare PDU data */
    {
        int pos = 0;

        /* Init dest array */
        memset(pduPtr->data, 0x00, sizeof(pduPtr->data));

#ifdef HAS_SMSC_INFORMATION
        WriteByte(pduPtr->data, pos++, 0x00); // Use default SMSC information
#endif //HAS_SMSC_INFORMATION

        /* First Byte */
        WriteByte(pduPtr->data, pos++, firstByte);

        if(messageType == PA_SMS_SMS_SUBMIT)
        {
            /* TP-MR: Message Reference */
            /* Default value */
            WriteByte(pduPtr->data, pos++, 0x00);
        }

        /* TP-DA: Destination Address for SMS-SUBMIT */
        /* TP-OA: Originating Address for SMS-DELIVER */
        /* (aka phone number, 2-12 bytes) */
        {
            WriteByte(pduPtr->data, pos++, addressLen);
            /* Type of address */
            WriteByte(pduPtr->data, pos++, addressToa);
            /* Number encoded */
            pos += (ConvertPhoneNumberIntoBinary(addressPtr,
                                                 &pduPtr->data[pos],
                                                 LE_SMS_PDU_MAX_BYTES-pos)
                     +1) / 2;
        }

        if(messageType == PA_SMS_SMS_DELIVER)
        {
            int idx;

            /* TP-SCTS: Service Center Time Stamp (7 bytes) */
            for(idx = 0; idx < 7; idx++)
            {
                WriteByte(pduPtr->data, pos++, 0x00);
            }
        }

        /* TP-PID: Protocol identifier (1 byte) */
        WriteByte(pduPtr->data, pos++, 0x00);

        /* TP-DCS: Data Coding Scheme (1 byte) */
        WriteByte(pduPtr->data, pos++, tp_dcs);

        if(messageType == PA_SMS_SMS_SUBMIT)
        {
            /* TP-VP: Validity Period (0, 1 or 7 bytes) */
            /* Set to 7 days */
            /* @TODO: Allow this value to be changed */
            WriteByte(pduPtr->data, pos++, 0xAD);
        }

        /* TP-UDL: User Data Length (1 byte) */
        int messageLen = min(length, maxSmsLength);
        WriteByte(pduPtr->data, pos++, messageLen);

        if(tp_udhi)
        {
            LE_ERROR("Udhi not supported");
        }

        /* TP-UD: User Data */
        switch(encoding)
        {
            case SMSPDU_7_BITS:
            {
                uint8_t newMessageLen;
                int size = Convert8BitsTo7Bits((const uint8_t*)messagePtr,
                                                0, messageLen,
                                                &pduPtr->data[pos],LE_SMS_PDU_MAX_PAYLOAD,
                                                &newMessageLen);
                if (size==LE_OVERFLOW)
                {
                    LE_ERROR("Overflow occurs when converting 8bits to 7bits");
                    return LE_OVERFLOW;
                }

                // Update message length size for special char.
                /* TP-UDL: User Data Length (1 byte) */
                WriteByte(pduPtr->data, pos-1 , newMessageLen);

                pos+=size;
                break;
            }
            case SMSPDU_8_BITS:
            {
                if (messageLen <= LE_SMS_PDU_MAX_PAYLOAD)
                {
                    memcpy(&pduPtr->data[pos], messagePtr, messageLen);

                    pos += messageLen;
                }
                else
                {
                    LE_ERROR("Overflow occurs when converting 8bits to 8bits");
                    return LE_OVERFLOW;
                }
                break;
            }
            case SMSPDU_UCS_2:
                LE_ERROR("UCS-2 encoding is not supported.");
                /* no break */
            default:
                return LE_UNSUPPORTED;
        }

        pduPtr->dataLen = pos;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode the content of dataPtr.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DecodeMessageGsm
(
    const uint8_t*    dataPtr,  ///< [IN] PDU data to decode
    size_t            dataSize, ///< [IN] PDU data size
    pa_sms_Message_t* smsPtr    ///< [OUT] Buffer to store decoded data
)
{
    le_result_t result;

    result = DecodeGsmPdu(dataPtr,smsPtr);

    if (result != LE_OK)
    {
        LE_ERROR("Could not decode GSM PDU message");
        return result;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Encode the content of dataPtr.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EncodeMessageGsm
(
    const uint8_t*      messagePtr,   ///< [IN] Data to encode
    size_t              length,       ///< [IN] Length of data
    const char*         addressPtr,   ///< [IN] Phone Number
    smsPdu_Encoding_t   encoding,     ///< [IN] Type of encoding to be used
    pa_sms_MsgType_t    messageType,  ///< [IN] Message Type
    pa_sms_Pdu_t*       pduPtr        ///< [OUT] Buffer for the encoded PDU
)
{
    le_result_t result;

    result = EncodeGsmPdu(messagePtr,length,addressPtr,encoding,messageType,pduPtr);

    if (result != LE_OK)
    {
        LE_ERROR("Could not encode GSM PDU message");
        return result;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Encode CDMA data in 7bits mode.
 *
 * @return LE_OK       Function success
 * @return LE_OVERFLOW a7bitPtr is too small
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EncodeCdma7BitsData
(
    const uint8_t *a8bitPtr,    ///< [IN] 8bits array to convert
    int            a8bitPtrSize,///< [IN] size of 8bits byte conversion
    uint8_t       *a7bitPtr,    ///< [OUT] 7bits array result
    size_t         a7bitSize,   ///< [IN] 7bits array size
    uint8_t       *a7bitsNumber ///< [OUT] number of char in 7bitsPtr
)
{
    int read;
    int write = 0;
    int size = 0;

    memset(a7bitPtr,0,a7bitSize);

    for (read = 0; read < a8bitPtrSize; ++read)
    {
        if (size>a7bitSize)
        {
            return LE_OVERFLOW;
        }

        WriteCdma7Bits(a7bitPtr, a8bitPtr[read], write*7);
        write++;

        /* Number of 8 bit chars */
        size = (write * 7);
        size = (size % 8 != 0) ? (size/8 + 1) : (size/8);
    }

    /* Number of written chars */
    *a7bitsNumber = write;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode CDMA data in 7bits mode.
 *
 * @return LE_OK       Function success
 * @return LE_OVERFLOW a8bitSize is too small
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DecodeCdma7bitsData
(
    const uint8_t *a7bitPtr,     ///< [IN] 7bits array to convert
    uint32_t       a7bitPtrSize, ///< [IN] number of 7bits byte convertion
    uint8_t       *a8bitPtr,     ///< [OUT] 8bits array restul
    size_t         a8bitSize,    ///< [IN] 8bits array size.
    uint32_t      *a8bitNumber   ///< [OUT] number of char written
)
{
    int read;
    int write;

    memset(a8bitPtr,0,a8bitSize);

    for (read = 0, write = 0; read < a7bitPtrSize; read++, write++)
    {
        if (write < a8bitSize)
        {
            a8bitPtr[write] = ReadCdma7Bits(a7bitPtr, read*7);
        }
        else
        {
            return LE_OVERFLOW;
        }
    }

    *a8bitNumber = write;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieved the Message type
 *
 * @return LE_OK        Function success
 * @return LE_NOT_FOUND Message identifier is not present in CDMA message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetCdmaMessageType
(
    const cdmaPdu_t         *cdmaMessage,   ///< [IN] cdma message
    cdmaPdu_MessageType_t   *messageType    ///< [OUT] message type
)
{
    if ( !(cdmaMessage->message.parameterMask & CDMAPDU_PARAMETERMASK_BEARER_DATA))
    {
        LE_INFO("No Bearer data in the message");
        return LE_NOT_FOUND;
    }

    if (!(  cdmaMessage->message.bearerData.subParameterMask
          & CDMAPDU_SUBPARAMETERMASK_MESSAGE_IDENTIFIER))
    {
        LE_INFO("No message identifier in the message");
        return LE_NOT_FOUND;
    }

    *messageType = cdmaMessage->message.bearerData.messageIdentifier.messageType;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode cdmaPdu_AddressParameter_t.
 *
 *
 * @return LE_OK        function succeed
 * @return LE_OVERFLOW  address is too small
 * @return LE_FAULT     function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DecodeAddressParameter
(
    const cdmaPdu_AddressParameter_t *addressParameter,   ///< [IN] the address parameter
    char                             *address,            ///< [OUT] address converted
    size_t                            addressSize         ///< [IN] size of address
)
{
    uint32_t i;
    uint32_t numFields = addressParameter->fieldsNumber;
    bool digitMode = addressParameter->digitMode;
    bool numberMode = addressParameter->numberMode;

    if (numFields>addressSize)
    {
        LE_WARN("Buffer overflow will occur (%d>%zd)",numFields,addressSize);
        return LE_OVERFLOW;
    }

    for(i = 0; i < numFields; i++)
    {
        if (digitMode == 0)
        {
            // DTMF encoding
            uint8_t addrDigit;
            if (i%2)
            {
                addrDigit = (addressParameter->chari[i/2]&0x0F);
            }
            else
            {
                addrDigit = (addressParameter->chari[i/2]&0xF0) >> 4;
            }
            if (addrDigit==0)
            {
                LE_WARN("%d digit code is not possible", addrDigit);
                return LE_FAULT;
            }
            address[i] = DtmfChars[addrDigit];
        }
        else
        {
            uint8_t addrDigit = addressParameter->chari[i];
            if (numberMode == 0)
            {
                // ASCII representation with with the most significant bit set set to 0
                address[i] = addrDigit;
            }
            else
            {
                if (addressParameter->numberType == CDMAPDU_NUMBERINGTYPE_INTERNET_EMAIL_ADDRESS)
                {
                    // 8 bit ASCII
                    address[i] = addrDigit;
                }
                else if (addressParameter->numberType == CDMAPDU_NUMBERINGTYPE_INTERNET_PROTOCOL)
                {
                    // Binary value of an octet of the address
                    address[i] = addrDigit;
                }
                else
                {
                    LE_WARN("Do not support this number type %d", addressParameter->numberType);
                    return LE_FAULT;
                }
            }
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Encode cdmaPdu_AddressParameter_t.
 *
 * @return LE_OK        function succeed
 * @return LE_OVERFLOW  address is too long
 * @return LE_FAULT     function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EncodeAddressParameter
(
    cdmaPdu_AddressParameter_t *addressParameter,   ///< [OUT] the address parameter
    const char                 *address,            ///< [IN] address converted
    size_t                      addressSize         ///< [IN] size of address
)
{
    uint32_t addressIndex = 0, i;

    // Hard coded
    addressParameter->digitMode = false;
    addressParameter->numberMode = false;

    size_t phoneLength = addressSize;

    if (address[0] == '+') // International phone number
    {
        phoneLength--;
        addressIndex++;
    }

    if (addressParameter->digitMode)
    {
        if (phoneLength>sizeof(addressParameter->chari))
        {
            LE_DEBUG("Buffer overflow");
            return LE_OVERFLOW;
        }
    }
    else
    {
        if (phoneLength>sizeof(addressParameter->chari)/2)
        {
            LE_DEBUG("Buffer overflow");
            return LE_OVERFLOW;
        }
    }

    addressParameter->fieldsNumber = phoneLength;

    for (i = 0; i < addressParameter->fieldsNumber; i++)
    {
        // Fill address
        if (addressParameter->digitMode)
        {
            addressParameter->chari[i] = address[i+addressIndex];
        }
        else
        {
            int pos = strchr(DtmfChars,address[i+addressIndex]) - DtmfChars;
            if (i%2)
            {
                addressParameter->chari[i/2] |= pos;
            }
            else
            {
                addressParameter->chari[i/2] |= pos << 4;
            }
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the Originating address
 *
 * @return LE_OK        Function success
 * @return LE_OVERFLOW  address is too small
 * @return LE_NOT_FOUND Originating address not present in CDMA message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetCdmaMessageOA
(
    const cdmaPdu_t *cdmaMessage,   ///< [IN] cdma message
    char            *address,       ///< [OUT] address converted
    size_t           addressSize    ///< [IN] size of address
)
{
    if ( !(cdmaMessage->message.parameterMask & CDMAPDU_PARAMETERMASK_ORIGINATING_ADDR))
    {
        LE_INFO("No origination address in the message");
        return LE_NOT_FOUND;
    }

    return DecodeAddressParameter(&cdmaMessage->message.originatingAddr, address, addressSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the Destination address
 *
 * @return LE_OK        Function success
 * @return LE_OVERFLOW  address is too small
 * @return LE_NOT_FOUND Destination address not present in CDMA message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetCdmaMessageDA
(
    const cdmaPdu_t *cdmaMessage,   ///< [IN] cdma message
    char            *address,       ///< [OUT] address converted
    size_t           addressSize    ///< [IN] size of address
)
{
    if ( !(cdmaMessage->message.parameterMask & CDMAPDU_PARAMETERMASK_DESTINATION_ADDR))
    {
        LE_INFO("No destination address in the message");
        return LE_NOT_FOUND;
    }

    return DecodeAddressParameter(&cdmaMessage->message.destinationAddr, address, addressSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the Destination address
 *
 * @return LE_OK        Function success
 * @return LE_FAULT     function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetCdmaMessageDA
(
    const char  *address,       ///< [IN] address message
    cdmaPdu_t   *cdmaMessage   ///< [OUT] message
)
{
    le_result_t result = EncodeAddressParameter(&cdmaMessage->message.destinationAddr,
                                                address,
                                                strlen(address));
    if (result != LE_OK)
    {
        LE_DEBUG("No destination address set in the message");
        return LE_FAULT;
    }

    cdmaMessage->message.parameterMask |= CDMAPDU_PARAMETERMASK_DESTINATION_ADDR;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert bytes into decimal.
 *
 * 0x41 hexa -> 41 decimal
 *
 * @return the decimal value
 */
//--------------------------------------------------------------------------------------------------
static inline uint32_t ConvertFromByte
(
    const uint8_t byte   ///< [IN] the date parameter
)
{
    return ( ( ((byte&0xF0)>>4) * 10) + (byte&0x0F) );
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert decimal into hexa.
 *
 * 41 decimal -> 0x41 hexa
 *
 * @return the hexa value
 */
//--------------------------------------------------------------------------------------------------
static inline uint8_t ConvertToByte
(
    const uint32_t value   ///< [IN] the date parameter
)
{
    return ( ( (value/10)<<4) | (value%10) );
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode cdmaPdu_AddressParameter_t.
 *
 * @return LE_OK        function succeed
 * @return LE_OVERFLOW  address is too small
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DecodeDate
(
    const cdmaPdu_Date_t *dateParameter,   ///< [IN] the date parameter
    char                 *date,            ///< [OUT] date converted
    size_t                dateSize         ///< [IN] size of date
)
{
    uint32_t timestampSize;

    timestampSize = 18;
    if (timestampSize>dateSize)
    {
        LE_WARN("Buffer overflow will occur (%d>%zd)",timestampSize,dateSize);
        return LE_OVERFLOW;
    }

    snprintf(date, timestampSize-1, "%d/%d/%d,%d:%d:%d",
             ConvertFromByte(dateParameter->year),
             ConvertFromByte(dateParameter->month),
             ConvertFromByte(dateParameter->day),
             ConvertFromByte(dateParameter->hours),
             ConvertFromByte(dateParameter->minutes),
             ConvertFromByte(dateParameter->seconds));
    date[timestampSize] = '\0';

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the Service center time stamp date
 *
 * @return LE_OK        Function success
 * @return LE_OVERFLOW  address is too small
 * @return LE_NOT_FOUND Message Service Time stamp not present in CDMA message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetCdmaMessageServiceCenterTimeStamp
(
    const cdmaPdu_t *cdmaMessage,   ///< [IN] cdma message
    char            *address,       ///< [OUT] address converted
    size_t           addressSize    ///< [IN] size of address
)
{
    if ( !(cdmaMessage->message.parameterMask & CDMAPDU_PARAMETERMASK_BEARER_DATA))
    {
        LE_INFO("No Bearer data in the message");
        return LE_NOT_FOUND;
    }

    if (!(  cdmaMessage->message.bearerData.subParameterMask
          & CDMAPDU_SUBPARAMETERMASK_MESSAGE_CENTER_TIME_STAMP))
    {
        LE_INFO("No service center time stamp in the message");
        return LE_NOT_FOUND;
    }

    return DecodeDate(&cdmaMessage->message.bearerData.messageCenterTimeStamp,
                      address, addressSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the message data
 *
 * @return LE_OK        Function success
 * @return LE_OVERFLOW  data is too small
 * @return LE_FAULT     function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetCdmaMessageData
(
    const cdmaPdu_t *cdmaMessage,   ///< [IN] cdma message
    uint8_t         *data,          ///< [OUT] data converted
    size_t           dataSize,      ///< [IN] size of data
    uint32_t        *dataLen,       ///< [OUT] data length
    le_sms_Format_t *format         ///< [OUT] format
)
{
    cdmaPdu_Encoding_t encoding;

    if ( !(cdmaMessage->message.parameterMask & CDMAPDU_PARAMETERMASK_BEARER_DATA))
    {
        LE_INFO("No Bearer data in the message");
        return LE_FAULT;
    }

    if ( !(cdmaMessage->message.bearerData.subParameterMask & CDMAPDU_SUBPARAMETERMASK_USER_DATA))
    {
        LE_INFO("No data in the message");
        return LE_FAULT;
    }

    encoding = cdmaMessage->message.bearerData.userData.messageEncoding;
    switch (encoding)
    {
        case CDMAPDU_ENCODING_7BIT_ASCII:
        {
            le_result_t result = DecodeCdma7bitsData(cdmaMessage->message.bearerData.userData.chari,
                                              cdmaMessage->message.bearerData.userData.fieldsNumber,
                                              data,
                                              dataSize,
                                              dataLen);
            if (result == LE_OVERFLOW)
            {
                LE_WARN("Overflow occur when decoding user data");
                return LE_OVERFLOW;
            }
            *format = LE_SMS_FORMAT_TEXT;
            break;
        }
        case CDMAPDU_ENCODING_OCTET:
        {
            if (cdmaMessage->message.bearerData.userData.fieldsNumber>dataSize-1)
            {
                LE_WARN("Overflow occur when decoding user data");
                return LE_OVERFLOW;
            }
            memcpy(data,
                   cdmaMessage->message.bearerData.userData.chari,
                   cdmaMessage->message.bearerData.userData.fieldsNumber);
            *dataLen = cdmaMessage->message.bearerData.userData.fieldsNumber;
            *format = LE_SMS_FORMAT_BINARY;
            break;
        }
        default:
        {
            LE_WARN("Do not support %d encoding",encoding);
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Teleservice Id
 *
 * @return LE_OK        Function success
 * @return LE_FAULT     function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetCdmaMessageTeleserviceId
(
    uint16_t     teleserviceId, ///< [IN] address message
    cdmaPdu_t   *cdmaMessage    ///< [OUT] message
)
{
    cdmaMessage->message.teleServiceId = teleserviceId;
    cdmaMessage->message.parameterMask |= CDMAPDU_PARAMETERMASK_TELESERVICE_ID;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the message id
 *
 * @return LE_OK        Function success
 * @return LE_FAULT     function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetCdmaMessageId
(
    cdmaPdu_MessageType_t type,         ///< [IN] type
    uint16_t              id,           ///< [IN] id
    cdmaPdu_t            *cdmaMessage   ///< [OUT] message
)
{
    cdmaMessage->message.bearerData.messageIdentifier.messageType = type;
    cdmaMessage->message.bearerData.messageIdentifier.messageIdentifier = id;
    cdmaMessage->message.bearerData.messageIdentifier.headerIndication = false;

    cdmaMessage->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_MESSAGE_IDENTIFIER;
    cdmaMessage->message.parameterMask |= CDMAPDU_PARAMETERMASK_BEARER_DATA;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the time stamp
 *
 * @return LE_OK        Function success
 * @return LE_FAULT     function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetCdmaMessageTimeStamp
(
    cdmaPdu_t            *cdmaMessage   ///< [OUT] message
)
{
    time_t currentTime;
    struct tm ctime;

    time(&currentTime);
    localtime_r(&currentTime,&ctime);

    if (ctime.tm_year>100) /* is > 2000 */
    {
        cdmaMessage->message.bearerData.messageCenterTimeStamp.year =
                ConvertToByte(ctime.tm_year-100);
    }
    else
    {
        cdmaMessage->message.bearerData.messageCenterTimeStamp.year = ConvertToByte(ctime.tm_year);
    }
    cdmaMessage->message.bearerData.messageCenterTimeStamp.month = ConvertToByte(ctime.tm_mon);
    cdmaMessage->message.bearerData.messageCenterTimeStamp.day = ConvertToByte(ctime.tm_mday);
    cdmaMessage->message.bearerData.messageCenterTimeStamp.hours = ConvertToByte(ctime.tm_hour);
    cdmaMessage->message.bearerData.messageCenterTimeStamp.minutes = ConvertToByte(ctime.tm_min);
    cdmaMessage->message.bearerData.messageCenterTimeStamp.seconds = ConvertToByte(ctime.tm_sec);

    cdmaMessage->message.bearerData.subParameterMask
                    |= CDMAPDU_SUBPARAMETERMASK_MESSAGE_CENTER_TIME_STAMP;
    cdmaMessage->message.parameterMask |= CDMAPDU_PARAMETERMASK_BEARER_DATA;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the message pririty
 *
 * @return LE_OK        Function success
 * @return LE_FAULT     function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetCdmaMessagePriority
(
    cdmaPdu_Priority_t  priority,     ///< [IN] priority
    cdmaPdu_t          *cdmaMessage   ///< [OUT] message
)
{
    cdmaMessage->message.bearerData.priority = priority;

    cdmaMessage->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_PRIORITY;
    cdmaMessage->message.parameterMask |= CDMAPDU_PARAMETERMASK_BEARER_DATA;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the message data
 *
 * @return LE_OK        Function success
 * @return LE_OVERFLOW  address is too small
 * @return LE_FAULT     function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetCdmaMessageData
(
    const uint8_t       *dataPtr,   ///< [IN] text message
    size_t               dataSize,  ///< [IN] size of data
    smsPdu_Encoding_t    encoding,  ///< [IN] Type of encoding to be used
    cdmaPdu_t           *messagePtr ///< [OUT] cdma message
)
{
    switch (encoding)
    {
        case SMSPDU_7_BITS: // 7 bits ASCII encoding
        {
            le_result_t result = EncodeCdma7BitsData(dataPtr,
                                             dataSize,
                                             messagePtr->message.bearerData.userData.chari,
                                             sizeof(messagePtr->message.bearerData.userData.chari),
                                             &messagePtr->message.bearerData.userData.fieldsNumber);
            if (result == LE_OVERFLOW)
            {
                LE_WARN("Overflow occur when encoding user data");
                return LE_OVERFLOW;
            }
            messagePtr->message.bearerData.userData.messageEncoding = CDMAPDU_ENCODING_7BIT_ASCII;
            break;
        }
        case SMSPDU_8_BITS:
        {
            if (dataSize>sizeof(messagePtr->message.bearerData.userData.chari))
            {
                LE_WARN("Overflow occur when encoding user data %zd>%zd",
                         dataSize,sizeof(messagePtr->message.bearerData.userData.chari));
                return LE_OVERFLOW;
            }
            memcpy(messagePtr->message.bearerData.userData.chari,dataPtr,dataSize);
            messagePtr->message.bearerData.userData.fieldsNumber = dataSize;
            messagePtr->message.bearerData.userData.messageEncoding = CDMAPDU_ENCODING_OCTET;
            break;
        }
        default:
        {
            LE_WARN("Do not support %d encoding",encoding);
            return LE_FAULT;
        }
    }

    messagePtr->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_USER_DATA;
    messagePtr->message.parameterMask |= CDMAPDU_PARAMETERMASK_BEARER_DATA;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode the content of dataPtr.
 *
 * @return LE_OK        Function succeed
 * @return LE_FAULT     Function Failed
 * @return LE_OVERFLOW  An overflow occur
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DecodeMessageCdma
(
    const uint8_t*    dataPtr,  ///< [IN] PDU data to decode
    size_t            dataSize, ///< [IN] PDU data size
    pa_sms_Message_t* smsPtr    ///< [OUT] Buffer to store decoded data
)
{
    le_result_t result = LE_OK;
    cdmaPdu_t message;
    cdmaPdu_MessageType_t messageType;

    result = cdmaPdu_Decode(dataPtr,dataSize,&message);

    if (result != LE_OK)
    {
        LE_ERROR("Could not decode CDMA PDU message");
        return LE_FAULT;
    }

    result = GetCdmaMessageType(&message,&messageType);

    if (result != LE_OK)
    {
        return LE_FAULT;
    }

    // Initialize output parameter
    memset(smsPtr, 0, sizeof(*smsPtr));

    switch (messageType)
    {
        case CDMAPDU_MESSAGETYPE_DELIVER:
        {
            smsPtr->smsDeliver.option = PA_SMS_OPTIONMASK_NO_OPTION;

            result = GetCdmaMessageOA(&message,smsPtr->smsDeliver.oa,sizeof(smsPtr->smsDeliver.oa));
            if (result==LE_OK)
            {
                smsPtr->smsDeliver.option |= PA_SMS_OPTIONMASK_OA;
            }

            result = GetCdmaMessageServiceCenterTimeStamp(&message,
                                                           smsPtr->smsDeliver.scts,
                                                           sizeof(smsPtr->smsDeliver.scts));
            if (result==LE_OK)
            {
                smsPtr->smsDeliver.option |= PA_SMS_OPTIONMASK_SCTS;
            }

            result = GetCdmaMessageData(&message,
                                        smsPtr->smsDeliver.data,
                                        sizeof(smsPtr->smsDeliver.data),
                                        &smsPtr->smsDeliver.dataLen,
                                        &smsPtr->smsDeliver.format);
            if (result!=LE_OK)
            {
                LE_ERROR("Could not retrieve data");
                return result;
            }

            smsPtr->type = PA_SMS_SMS_DELIVER;
            break;
        }
        case CDMAPDU_MESSAGETYPE_SUBMIT:
        {
            result = GetCdmaMessageDA(&message,smsPtr->smsSubmit.da,sizeof(smsPtr->smsSubmit.da));
            if (result==LE_OK)
            {
                smsPtr->smsDeliver.option |= PA_SMS_OPTIONMASK_DA;
            }

            result = GetCdmaMessageData(&message,
                                        smsPtr->smsSubmit.data,
                                        sizeof(smsPtr->smsSubmit.data),
                                        &smsPtr->smsSubmit.dataLen,
                                        &smsPtr->smsSubmit.format);
            if (result!=LE_OK)
            {
                LE_ERROR("Could not retrieve data");
                return result;
            }

            smsPtr->type = PA_SMS_SMS_SUBMIT;
            break;
        }
        default:
        {
            LE_WARN("Do not support this message type %d", messageType);
            result = LE_UNSUPPORTED;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Encode the content of dataPtr.
 *
 * @return LE_OK    Function succeed
 * @return LE_FAULT Function Failed
 * @return LE_OVERFLOW  An overflow occur
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EncodeMessageCdma
(
    const uint8_t*      messagePtr,   ///< [IN] Data to encode
    size_t              length,       ///< [IN] Length of data
    const char*         addressPtr,   ///< [IN] Phone Number
    smsPdu_Encoding_t   encoding,     ///< [IN] Type of encoding to be used
    pa_sms_MsgType_t    messageType,  ///< [IN] Message Type
    pa_sms_Pdu_t*       pduPtr        ///< [OUT] Buffer for the encoded PDU
)
{
    le_result_t result = LE_OK;
    cdmaPdu_t message;

    memset(&message,0,sizeof(message));

    // Hard-coded
    message.messageFormat = CDMAPDU_MESSAGEFORMAT_POINTTOPOINT;

    // Hard-coded
    result = SetCdmaMessageTeleserviceId(0x1002,&message);
    if (result!=LE_OK)
    {
        LE_ERROR("Could not set Teleservice Id");
        return result;
    }

    result = SetCdmaMessageDA(addressPtr,&message);
    if (result!=LE_OK)
    {
        LE_ERROR("Could not set Destination Adress");
        return result;
    }

    // Hard-coded
    result = SetCdmaMessageId(CDMAPDU_MESSAGETYPE_SUBMIT,1,&message);
    if (result!=LE_OK)
    {
        LE_ERROR("Could not set data");
        return result;
    }

    result = SetCdmaMessageData(messagePtr,length,encoding,&message);
    if (result!=LE_OK)
    {
        LE_ERROR("Could not set data");
        return result;
    }

    // Hard-coded
    result = SetCdmaMessageTimeStamp(&message);
    if (result!=LE_OK)
    {
        LE_ERROR("Could not set data");
        return result;
    }

    // Hard-coded
    result = SetCdmaMessagePriority(CDMAPDU_PRIORITY_NORMAL,&message);
    if (result!=LE_OK)
    {
        LE_ERROR("Could not set data");
        return result;
    }

    result = cdmaPdu_Encode(&message,
                            pduPtr->data,
                            sizeof(pduPtr->data),
                            &pduPtr->dataLen);
    if (result != LE_OK)
    {
        LE_ERROR("Could not Encode CDMA PDU message");
        return result;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the module.
 *
 * @return LE_OK
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsPdu_Initialize
(
    void
)
{
    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("smsPdu");

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode the content of dataPtr.
 *
 * @return LE_OK            Function succeed
 * @return LE_UNSUPPORTED   Protocol is not supported
 * @return LE_FAULT         Function failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsPdu_Decode
(
    pa_sms_Protocol_t protocol, ///< [IN] decoding protocol
    const uint8_t*    dataPtr,  ///< [IN] PDU data to decode
    size_t            dataSize, ///< [IN] PDU data size
    pa_sms_Message_t* smsPtr    ///< [OUT] Buffer to store decoded data
)
{
    le_result_t result = LE_OK;

    if (IS_TRACE_ENABLED)
    {
        DumpPdu("PDU to decode",dataPtr,dataSize);
    }

    if (protocol == PA_SMS_PROTOCOL_GSM)
    {
        result = DecodeMessageGsm(dataPtr,dataSize,smsPtr);
    }
    else if ( protocol == PA_SMS_PROTOCOL_CDMA)
    {
        result = DecodeMessageCdma(dataPtr,dataSize,smsPtr);
    }
    else
    {
        LE_WARN("Protocol %d not supported", protocol);
        result = LE_UNSUPPORTED;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Encode the content of messagePtr in PDU format.
 *
 * @return LE_OK            Function succeed
 * @return LE_UNSUPPORTED   Protocol is not supported
 * @return LE_FAULT         Function failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsPdu_Encode
(
    pa_sms_Protocol_t   protocol,     ///< [IN] Encoding protocol
    const uint8_t*      messagePtr,   ///< [IN] Data to encode
    size_t              length,       ///< [IN] Length of data
    const char*         addressPtr,   ///< [IN] Phone Number
    smsPdu_Encoding_t   encoding,     ///< [IN] Type of encoding to be used
    pa_sms_MsgType_t    messageType,  ///< [IN] Message Type
    pa_sms_Pdu_t*       pduPtr        ///< [OUT] Buffer for the encoded PDU
)
{
    le_result_t result = LE_OK;

    if (protocol == PA_SMS_PROTOCOL_GSM)
    {
        result = EncodeMessageGsm(messagePtr,length,addressPtr,encoding,messageType,pduPtr);
    }
    else if ( protocol == PA_SMS_PROTOCOL_CDMA)
    {
        result = EncodeMessageCdma(messagePtr,length,addressPtr,encoding,messageType,pduPtr);
    }
    else
    {
        LE_WARN("Protocol %d not supported", protocol);
        result = LE_UNSUPPORTED;
    }

    if (IS_TRACE_ENABLED)
    {
        DumpPdu("PDU Encoded",pduPtr->data,pduPtr->dataLen);
    }

    return result;
}
