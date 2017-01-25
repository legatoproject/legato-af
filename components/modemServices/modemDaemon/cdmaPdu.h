/** @file cdmaPdu.h
 *
 * This file contains the declaration of the CDMA PDU encoding/decoding.
 *
 * Reference used:
 *  - C.S0015-B_v2.0  "Short Message Service (SMS) for Wideband Spread Spectrum Systems"
 *  - C.R1001-D_v1.0  "Administration of Parameter Value Assignments for cdma2000 Spread
 *  Spectrum Standards"
 *  - C.S0005-D_v2.0  "Upper Layer (Layer 3) Signaling Standard for cdma2000 Spread Spectrum
 *  Systems"
 *  - N.S0005-0_v1.0  "Cellular Radiotelecommunications Intersystem Operations"
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef CDMAPDU_INCLUDE_GUARD
#define CDMAPDU_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Maximum address bytes size
 */
//--------------------------------------------------------------------------------------------------
#define CDMAPDU_ADDRESS_MAX_BYTES   50

//--------------------------------------------------------------------------------------------------
/**
 * Maximum Data bytes size
 */
//--------------------------------------------------------------------------------------------------
#define CDMAPDU_DATA_MAX_BYTES  140

//--------------------------------------------------------------------------------------------------
/**
 * CDMA message format.
 *
 * C.S0015-B V2.0 Table 3.4-1
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_MESSAGEFORMAT_POINTTOPOINT = 0,    ///< SMS Point-to-Point '00000000'
    CDMAPDU_MESSAGEFORMAT_BROADCAST    = 1,    ///< SMS Broadcast      '00000001'
    CDMAPDU_MESSAGEFORMAT_ACKNOWLEDGE  = 2     ///< SMS Acknowledge    '00000010'
}
cdmaPdu_MessageFormat_t;

//--------------------------------------------------------------------------------------------------
/**
 * Parameter mask that define which parameter (in cdmaPdu_Message_t) is filled with value.
 *
 * C.S0015-B V2.0 Table 3.4.3-1
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_PARAMETERMASK_NO_PARAMS             = 0x0000,   ///< No param
    CDMAPDU_PARAMETERMASK_TELESERVICE_ID        = 0x0001,   ///< teleServiceId mask
    CDMAPDU_PARAMETERMASK_ORIGINATING_ADDR      = 0x0002,   ///< originatingAddr mask
    CDMAPDU_PARAMETERMASK_DESTINATION_ADDR      = 0x0004,   ///< destinationAddr mask
    CDMAPDU_PARAMETERMASK_SERVICE_CATEGORY      = 0x0008,   ///< serviceCategory mask
    CDMAPDU_PARAMETERMASK_ORIGINATING_SUBADDR   = 0x0010,   ///< originatingSubaddress mask
    CDMAPDU_PARAMETERMASK_DESTINATION_SUBADDR   = 0x0020,   ///< destinationSubaddress mask
    CDMAPDU_PARAMETERMASK_BEARER_REPLY_OPTION   = 0x0040,   ///< bearerReplyOption mask
    CDMAPDU_PARAMETERMASK_BEARER_DATA           = 0x0080,   ///< bearerData mask
    CDMAPDU_PARAMETERMASK_CAUSE_CODES           = 0x0100,   ///< causeCodes mask
}cdmaPdu_ParameterMask_t;

//--------------------------------------------------------------------------------------------------
/**
 * SubParameter mask that define which parameter (in cdmaPdu_BearerData_t) is filled with value.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_SUBPARAMETERMASK_NO_PARAMS                       = 0x00000000, ///< No param
    CDMAPDU_SUBPARAMETERMASK_MESSAGE_IDENTIFIER              = 0x00000001, ///< messageIdentifier mask
    CDMAPDU_SUBPARAMETERMASK_USER_DATA                       = 0x00000002, ///< userData mask
    CDMAPDU_SUBPARAMETERMASK_USER_RESPONSE_CODE              = 0x00000004, ///< userResponseCode mask
    CDMAPDU_SUBPARAMETERMASK_MESSAGE_CENTER_TIME_STAMP       = 0x00000008, ///< messageCenterTimeStamp mask
    CDMAPDU_SUBPARAMETERMASK_VALIDITY_PERIOD_ABSOLUTE        = 0x00000010, ///< validityPeriodAbsolute mask
    CDMAPDU_SUBPARAMETERMASK_VALIDITY_PERIOD_RELATIVE        = 0x00000020, ///< validityPeriodRelative mask
    CDMAPDU_SUBPARAMETERMASK_DEFERRED_DELIVERY_TIME_ABSOLUTE = 0x00000040, ///< deferredDeliveryTimeAbsolute mask
    CDMAPDU_SUBPARAMETERMASK_DEFERRED_DELIVERY_TIME_RELATIVE = 0x00000080, ///< deferredDeliveryTimeRelative mask
    CDMAPDU_SUBPARAMETERMASK_PRIORITY                        = 0x00000100, ///< priority mask
    CDMAPDU_SUBPARAMETERMASK_PRIVACY                         = 0x00000200, ///< privacy mask
    CDMAPDU_SUBPARAMETERMASK_REPLY_OPTION                    = 0x00000400, ///< replyOption mask
    CDMAPDU_SUBPARAMETERMASK_MESSAGE_COUNT                   = 0x00000800, ///< messageCount mask
    CDMAPDU_SUBPARAMETERMASK_ALERT_ON_MESSAGE_DELIVERY       = 0x00001000, ///< alertOnMessageDelivery mask
    CDMAPDU_SUBPARAMETERMASK_LANGUAGE                        = 0x00002000, ///< language mask
    CDMAPDU_SUBPARAMETERMASK_CALL_BACK_NUMBER                = 0x00004000, ///< callBackNumber mask
    CDMAPDU_SUBPARAMETERMASK_MESSAGE_DISPLAY_MODE            = 0x00008000, ///< messageDisplayMode mask
    CDMAPDU_SUBPARAMETERMASK_MULTIPLE_ENCODING_USER_DATA     = 0x00010000, ///< multipleEncodingUserData mask
    CDMAPDU_SUBPARAMETERMASK_MESSAGE_DEPOSIT_INDEX           = 0x00020000, ///< messageDepositIndex mask
    CDMAPDU_SUBPARAMETERMASK_SERVICE_CATEGORY_PROGRAM_DATA   = 0x00040000, ///< serviceCategoryProgramData mask
    CDMAPDU_SUBPARAMETERMASK_SERVICE_CATEGORY_PROGRAM_RESULTS= 0x00080000, ///< serviceCategoryProgramResults mask
    CDMAPDU_SUBPARAMETERMASK_MESSAGE_STATUS                  = 0x00040000, ///< messageStatus mask
    CDMAPDU_SUBPARAMETERMASK_TP_FAILURE_CAUSE                = 0x00080000, ///< tpFailureCause mask
    CDMAPDU_SUBPARAMETERMASK_ENHANCED_VMN                    = 0x00100000, ///< enhancedVmn mask
    CDMAPDU_SUBPARAMETERMASK_ENHANCED_VMN_ACK                = 0x00200000, ///> enhancedVmnAck mask
}cdmaPdu_SubParameterMask_t;

//--------------------------------------------------------------------------------------------------
/**
 * Numbering type enumeration.
 *
 * C.S0005-D v2.0 Table 2.7.1.3.2.4-2. Number Types
 * C.S0015-B v2.0 Table 3.4.3.3-1. Data Network Address Number Types
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    // used when digitMode is true and numberMode is false
    CDMAPDU_NUMBERINGTYPE_UNKNOWN = 0,                 ///< Unknown '000'
    CDMAPDU_NUMBERINGTYPE_INTERNATIONAL = 1,           ///< International number '001'
    CDMAPDU_NUMBERINGTYPE_NATIONAL = 2,                ///< National number '010'
    CDMAPDU_NUMBERINGTYPE_NETWORK_SPECIFIC = 3,        ///< Network-specific number '011'
    CDMAPDU_NUMBERINGTYPE_SUBSCRIBER = 4,              ///< Subscriber number '100'
    CDMAPDU_NUMBERINGTYPE_RESERVED = 5,                ///< Reserved '101'
    CDMAPDU_NUMBERINGTYPE_ABBREVIATED = 6,             ///< Abbreviated number '110'
    CDMAPDU_NUMBERINGTYPE_RESERVED_FOR_EXTENSION =7,   ///< Reserved for extension '111'
    // Used when digitMode and numberMode are true
    CDMAPDU_NUMBERINGTYPE_INTERNET_PROTOCOL = 1,       ///< Internet Protocol '001'
    CDMAPDU_NUMBERINGTYPE_INTERNET_EMAIL_ADDRESS = 2,  ///< Internet Email Address '010'
}
cdmaPdu_NumberingType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Numbering plan enumeration.
 *
 * C.S0005-D v2.0 Table 2.7.1.3.2.4-3. Numbering Plan Identification
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    // used when digitMode is true and numberMode is false
    CDMAPDU_NUMBERINGPLAN_UNKNOWN  = 0,    ///< Unknown '0000'
    CDMAPDU_NUMBERINGPLAN_ISDN     = 1,    ///< ISDN/Telephony numbering plan '0001'
    CDMAPDU_NUMBERINGPLAN_DATA     = 3,    ///< Data numbering plan '0011'
    CDMAPDU_NUMBERINGPLAN_TELEX    = 4,    ///< Telex numbering plan '0100'
    CDMAPDU_NUMBERINGPLAN_PRIVATE  = 9,    ///< Private numbering plan '1001'
    CDMAPDU_NUMBERINGPLAN_RESERVED = 15,   ///< Reserved for extension
}
cdmaPdu_NumberingPlan_t;

//--------------------------------------------------------------------------------------------------
/**
 * Address Parameter structure.
 *
 * C.S0015-B v2.0 section 3.4.3.3 or 3.4.3.4
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool digitMode;     ///< Digit mode indicator
    bool numberMode;    ///< Number mode indicator.
    cdmaPdu_NumberingType_t numberType; ///< Type of number.
    cdmaPdu_NumberingPlan_t numberPlan; ///< Numbering plan.
    uint8_t fieldsNumber;                       ///< Number of digits in the chari
    uint8_t chari[CDMAPDU_ADDRESS_MAX_BYTES];   ///< An address digit or character.
}
cdmaPdu_AddressParameter_t;

//--------------------------------------------------------------------------------------------------
/**
 * Service Category
 *
 * see documentation C.R1001.D_v1.0_110403 section 9.3 (Service Category Assignments)
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_SERVICECATEGORY_UNKNOWN = 0x0000,    ///< Unknown or unspecified
    CDMAPDU_SERVICECATEGORY_EMERGENCY_BROADCAST = 0x0001, ///<  Emergency Broadcasts
    CDMAPDU_SERVICECATEGORY_ADMINISTRATIVE = 0x0002, ///< Administrative
    CDMAPDU_SERVICECATEGORY_MAINTENANCE = 0x0003, ///< Maintenance
    CDMAPDU_SERVICECATEGORY_GENERAL_NEWS_LOCAL = 0x0004, ///< General News – Local
    CDMAPDU_SERVICECATEGORY_GENERAL_NEWS_REGIONAL = 0x0005, ///< General News – Regional
    CDMAPDU_SERVICECATEGORY_GENERAL_NEWS_NATIONAL = 0x0006, ///< General News – National
    CDMAPDU_SERVICECATEGORY_GENERAL_NEWS_INTERNATIONAL = 0x0007, ///< General News – International
    CDMAPDU_SERVICECATEGORY_BUSINESS_FINANCIAL_NEWS_LOCAL = 0x0008, ///< Business/Financial News – Local
    CDMAPDU_SERVICECATEGORY_BUSINESS_FINANCIAL_NEWS_REGIONAL = 0x0009, ///< Business/Financial News – Regional
    CDMAPDU_SERVICECATEGORY_BUSINESS_FINANCIAL_NEWS_NATIONAL = 0x000A, ///< Business/Financial News – National
    CDMAPDU_SERVICECATEGORY_BUSINESS_FINANCIAL_NEWS_INTERNATIONAL = 0x000B, ///< Business/Financial News – International
    CDMAPDU_SERVICECATEGORY_SPORT_NEWS_LOCAL = 0x000C, ///< Sports News – Local
    CDMAPDU_SERVICECATEGORY_SPORT_NEWS_REGIONAL = 0x000D, ///< Sports News – Regional
    CDMAPDU_SERVICECATEGORY_SPORT_NEWS_NATIONAL = 0x000E, ///< Sports News – National
    CDMAPDU_SERVICECATEGORY_SPORT_NEWS_INTERNATIONAL = 0x000F, ///< Sports News – International
    CDMAPDU_SERVICECATEGORY_ENTERTAINMENT_NEWS_LOCAL = 0x0010, ///< Entertainment News – Local
    CDMAPDU_SERVICECATEGORY_ENTERTAINMENT_NEWS_REGIONAL = 0x0011, ///< Entertainment News – Regional
    CDMAPDU_SERVICECATEGORY_ENTERTAINMENT_NEWS_NATIONAL = 0x0012, ///< Entertainment News – National
    CDMAPDU_SERVICECATEGORY_ENTERTAINMENT_NEWS_INTERNATIONAL = 0x0013, ///< Entertainment News – International
    CDMAPDU_SERVICECATEGORY_LOCAL_WEATHER = 0x0014, ///< Local Weather
    CDMAPDU_SERVICECATEGORY_AREA_TRAFFIC_REPORTS = 0x0015, ///< Area Traffic Reports
    CDMAPDU_SERVICECATEGORY_LOCAL_AIRPORT_FLIGHT_SCHEDULES = 0x0016, ///< Local Airport Flight Schedules
    CDMAPDU_SERVICECATEGORY_RESTAURANTS = 0x0017, ///< Restaurants
    CDMAPDU_SERVICECATEGORY_LODGINGS = 0x0018, ///< Lodgings
    CDMAPDU_SERVICECATEGORY_RETAIL_DIRECTORY = 0x0019, ///< Retail Directory
    CDMAPDU_SERVICECATEGORY_ADVERTISEMENTS = 0x001A, ///< Advertisements
    CDMAPDU_SERVICECATEGORY_STOCK_QUOTES = 0x001B, ///< Stock Quotes
    CDMAPDU_SERVICECATEGORY_EMPLOYMENT_OPPORTUNITIES = 0x001C, ///< Employment Opportunities
    CDMAPDU_SERVICECATEGORY_MEDICAL_HEALTH_HOSPITALS = 0x001D, ///< Medical/Health/Hospitals
    CDMAPDU_SERVICECATEGORY_TECHNOLOGY_NEWS = 0x001E, ///< Technology News
    CDMAPDU_SERVICECATEGORY_MULTICATEGORY = 0x001F, ///< Multi-category
}
cdmaPdu_ServiceCategory_t;

//--------------------------------------------------------------------------------------------------
/**
 * Subaddress type
 *
 * C.S0015-B_v2.0_051006 Table 3.4.3.4-1. Subaddress Type Values
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_SUBADDRESS_TYPE_NSAP = 0,           ///< NSAP '000'
    CDMAPDU_SUBADDRESS_TYPE_USER_SPECIFIED = 1, ///< User-specified '001'
}
cdmaPdu_SubAddress_type_t;

//--------------------------------------------------------------------------------------------------
/**
 * Address Parameter structure.
 *
 * C.S0015-B v2.0 section 3.4.3.4
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    cdmaPdu_SubAddress_type_t type; ///< Subaddress type.
    bool    odd;                    ///< Odd/even indicator.
    uint8_t fieldsNumber;                      ///< Number of digits in the address
    uint8_t chari[CDMAPDU_ADDRESS_MAX_BYTES];  ///< An address digit or character.
}
cdmaPdu_SubAddress_t;

//--------------------------------------------------------------------------------------------------
/**
 * Bearer Reply Option structure.
 *
 * C.S0015-B v2.0 section 3.4.3.5
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t replySeq;   ///< Reply sequence number.
}
cdmaPdu_BearerReplyOption_t;

//--------------------------------------------------------------------------------------------------
/**
 * Error report class.
 *
 * C.S0015-B v2.0 section 3.4.3.6 or 4.5.21
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_ERRORCLASS_NO_ERROR  = 0,   ///< no error '00'
    CDMAPDU_ERRORCLASS_RESERVED  = 1,   ///< Reserved '01'
    CDMAPDU_ERRORCLASS_TEMPORARY = 2,   ///< temporary condition '10'
    CDMAPDU_ERRORCLASS_PERMANENT = 3,   ///< permanent condition '11'
}
cdmaPdu_ErrorClass_t;

//--------------------------------------------------------------------------------------------------
/**
 * Cause Codes structure.
 *
 * C.S0015-B v2.0 section 3.4.3.6
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t replySeq;                   ///< Reply sequence number.
    cdmaPdu_ErrorClass_t errorClass;    ///< Error report class.
    uint8_t errorCause;                 ///< Error cause identifier. see SMS_CauseCode table in N.S0005
}
cdmaPdu_CauseCodes_t;

//--------------------------------------------------------------------------------------------------
/**
 * Message type enumeration.
 *
 * C.S0015-B v2.0 table 4.5.1-1
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_MESSAGETYPE_RESERVED       = 0,    ///< Reserved '0000'
    CDMAPDU_MESSAGETYPE_DELIVER        = 1,    ///< Deliver (mobile-terminated only) '0001'
    CDMAPDU_MESSAGETYPE_SUBMIT         = 2,    ///< Submit (mobile-originated only)
    CDMAPDU_MESSAGETYPE_CANCELLATION   = 3,    ///< Cancellation (mobile-originated only) '0011'
    CDMAPDU_MESSAGETYPE_DELIVERY_ACK   = 4,    ///< Delivery Acknowledgment (mobile-terminated only) '0100'
    CDMAPDU_MESSAGETYPE_USER_ACK       = 5,    ///< User Acknowledgment (either direction) '0101'
    CDMAPDU_MESSAGETYPE_READ_ACK       = 6,    ///< Read Acknowledgment (either direction) '0110'
    CDMAPDU_MESSAGETYPE_DELIVER_REPORT = 7,    ///< Deliver Report (mobile-originated only) '0111'
    CDMAPDU_MESSAGETYPE_SUBMIT_REPORT  = 8,    ///< Submit Report (mobile-terminated only) '1000'
}
cdmaPdu_MessageType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Message Identifier structure.
 *
 * C.S0015-B v2.0 section 4.5.1
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    cdmaPdu_MessageType_t   messageType;        ///< Message type
    uint16_t                messageIdentifier;  ///< Message identifier.
    bool                    headerIndication;   ///< Header Indicator.
}
cdmaPdu_MessageIdentifier_t;

//--------------------------------------------------------------------------------------------------
/**
 * Encoding enumeration.
 *
 * C.R1001-D v1.0 Table 9.2-1. Language Indicator Value Assignments
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_ENCODING_OCTET                      = 0,    ///< Octet, unspecified ‘00000’
    CDMAPDU_ENCODING_EXTENDED_PROTOCOL_MESSAGE  = 1,    ///< Extended Protocol Message ‘00001’
    CDMAPDU_ENCODING_7BIT_ASCII                 = 2,    ///< 7-bit ASCII '00010'
    CDMAPDU_ENCODING_IA5                        = 3,    ///< IA5 '00011'
    CDMAPDU_ENCODING_UNICODE                    = 4,    ///< UNICODE '00100' 16-bit encoding
    CDMAPDU_ENCODING_SHIFT_JIS                  = 5,    ///< Shift-JIS '00101'
    CDMAPDU_ENCODING_KOREAN                     = 6,    ///< Korean '00110'
    CDMAPDU_ENCODING_LATIN_HEBREW               = 7,    ///< Latin/Hebrew '00111'
    CDMAPDU_ENCODING_LATIN                      = 8,    ///< Latin '01000'
    CDMAPDU_ENCODING_GSM_7BIT_DEFAULT_ALPHABET  = 9,    ///< GSM 7-bit default alphabet '01001'
}
cdmaPdu_Encoding_t;

//--------------------------------------------------------------------------------------------------
/**
 * User Data structure.
 *
 * C.S0015-B v2.0 section 4.5.2
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    cdmaPdu_Encoding_t  messageEncoding;                ///< Message encoding.
    uint8_t             messageType;                    ///< Message type.
    uint8_t             fieldsNumber;                   ///< Number of digits in the chari
    uint8_t             chari[CDMAPDU_DATA_MAX_BYTES];  ///< Character
}
cdmaPdu_UserData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Date structure.
 *
 * C.S0015-B v2.0 section 4.5.4 or 4.5.5 or 4.5.7
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t year;       ///< Year
    uint8_t month;      ///< Month
    uint8_t day;        ///< day
    uint8_t hours;      ///< hours
    uint8_t minutes;    ///< minutes
    uint8_t seconds;    ///< seconds
}
cdmaPdu_Date_t;

//--------------------------------------------------------------------------------------------------
/**
 * Priority enumeration.
 *
 * C.S0015-B v2.0 table 4.5.9-1 Priority Indicator Values
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_PRIORITY_NORMAL         = 0,    ///< Normal '00'
    CDMAPDU_PRIORITY_INTERACTIVE    = 1,    ///< Interactive '01'
    CDMAPDU_PRIORITY_URGENT         = 2,    ///< Urgent '10'
    CDMAPDU_PRIORITY_EMERGENCY      = 3,    ///< Emergency '11'
}
cdmaPdu_Priority_t;

//--------------------------------------------------------------------------------------------------
/**
 * Privacy enumeration.
 *
 * C.S0015-B v2.0 table 4.5.10-1. Privacy Indicator Values
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_PRIVACY_NOT_RESTRICTED = 0,    ///< Not restricted (privacy level 0) '00'
    CDMAPDU_PRIVACY_RESTRICTED     = 1,    ///< Restricted (privacy level 1) '01'
    CDMAPDU_PRIVACY_CONFIDENTIAL   = 2,    ///< Confidential (privacy level 2) '10'
    CDMAPDU_PRIVACY_SECRET         = 3,    ///< Secret (privacy level 3) '11'
}
cdmaPdu_Privacy_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reply Option structure.
 *
 * C.S0015-B v2.0 section 4.5.11
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool userAck;           ///< Positive user (manual) acknowledgment requested.
    bool deliveryAck;       ///< Delivery acknowledgment request.
    bool readAck;           ///< Read acknowledgment request.
    bool deliveryReport;    ///< Delivery report request.
}
cdmaPdu_ReplyOption_t;

//--------------------------------------------------------------------------------------------------
/**
 * Alert Priority enumeration.
 *
 * C.S0015-B v2.0 table 4.5.13-1. ALERT_PRIORITY Values
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_ALERTPRIORITY_DEFAULT   = 0,    ///< Use Mobile default alert '00'
    CDMAPDU_ALERTPRIORITY_LOW       = 1,    ///< Use Low-priority alert '01'
    CDMAPDU_ALERTPRIORITY_MEDIUM    = 2,    ///< Use Medium-priority alert '10'
    CDMAPDU_ALERTPRIORITY_HIGH      = 3,    ///< Use High-priority alert '11'
}
cdmaPdu_AlertPriority_t;

//--------------------------------------------------------------------------------------------------
/**
 * Language enumeration.
 *
 * see table 9.2 (Language Indicator Value Assignments) in C.R1001-D
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_LANGUAGE_UNKNOWN    = 0,    ///< Unknown or unspecified '00000000'
    CDMAPDU_LANGUAGE_ENGLISH    = 1,    ///< English '00000001'
    CDMAPDU_LANGUAGE_FRENCH     = 2,    ///< French ‘00000010’
    CDMAPDU_LANGUAGE_SPANISH    = 3,    ///< Spanish ‘00000011’
    CDMAPDU_LANGUAGE_JAPANESE   = 4,    ///< Japanese '00000100'
    CDMAPDU_LANGUAGE_KOREAN     = 5,    ///< Korean '00000101'
    CDMAPDU_LANGUAGE_CHINESE    = 6,    ///< Chinese '00000110'
    CDMAPDU_LANGUAGE_HEBREW     = 7,    ///< Hebrew '00000111'
}
cdmaPdu_Language_t;

//--------------------------------------------------------------------------------------------------
/**
 * Message display mode enumeration.
 *
 * C.S0015-B v2.0 table 4.5.16-1. Message Display Mode Indicator Values
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_MESSAGEDISPLAY_IMMEDIATE    = 0,    ///< Immediate Display '00'
    CDMAPDU_MESSAGEDISPLAY_DEFAULT      = 1,    ///< Mobile default setting '01'
    CDMAPDU_MESSAGEDISPLAY_USER_INVOKE  = 2,    ///< User Invoke '10'
    CDMAPDU_MESSAGEDISPLAY_RESERVED     = 3,    ///< Reserved '11'
}
cdmaPdu_MessageDisplay_t;

//--------------------------------------------------------------------------------------------------
/**
 * Message display mode enumeration.
 *
 * C.S0015-B V2.0  Table 4.5.21-1. SMS Message Status Codes
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CDMAPDU_MESSAGESTATUSCODE_MESSAGE_ACCEPTED      = 0,    ///< Message accepted '000000'
    CDMAPDU_MESSAGESTATUSCODE_MESSAGE_DEPOSITED     = 1,    ///< Message deposited to Internet '000001'
    CDMAPDU_MESSAGESTATUSCODE_MESSAGE_DELIVERED     = 2,    ///< Message delivered '000010'
    CDMAPDU_MESSAGESTATUSCODE_MESSAGE_CANCELLED     = 3,    ///< Message cancelled '000011'
    CDMAPDU_MESSAGESTATUSCODE_NETWORK_CONGESTION    = 4,    ///< Network congestion '000100'
    CDMAPDU_MESSAGESTATUSCODE_NETWORK_ERROR         = 5,    ///< Network error '000101'
    CDMAPDU_MESSAGESTATUSCODE_CANCEL_FAILED         = 6,    ///< Cancel failed '000110'
    CDMAPDU_MESSAGESTATUSCODE_BLOCKED_DESTINATION   = 7,    ///< Blocked destination '000111'
    CDMAPDU_MESSAGESTATUSCODE_TEXT_TOO_LONG         = 8,    ///< Text too long '001000'
    CDMAPDU_MESSAGESTATUSCODE_DUPLICATE_MESSAGE     = 9,    ///< Duplicate message '001001'
    CDMAPDU_MESSAGESTATUSCODE_INVALID_DESTINATION   = 10,   ///< Invalid destination '001010'
    CDMAPDU_MESSAGESTATUSCODE_MESSAGE_EXPIRED       = 13,   ///< Message expired '001101'
    CDMAPDU_MESSAGESTATUSCODE_UNKNOWN               = 31,   ///< Unknown error '011111'
}
cdmaPdu_MessageStatusCode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Message Status structure.
 *
 * C.S0015-B V2.0 Section 4.5.21
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    cdmaPdu_ErrorClass_t        errorClass;         ///< Error report class.
    cdmaPdu_MessageStatusCode_t messageStatusCode;  ///< Message status code.
}
cdmaPdu_MessageStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Bearer Data structure.
 *
 * Based on documentation C.S0015-B V2.0
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    cdmaPdu_SubParameterMask_t  subParameterMask;       ///< Mask of subparameters available
    cdmaPdu_MessageIdentifier_t messageIdentifier;      ///< Message Identifier (4.5.1)
    cdmaPdu_UserData_t          userData;               ///< User Data  (4.5.2)
    uint8_t                     userResponseCode;       ///< User Response Code (4.5.3)
    cdmaPdu_Date_t              messageCenterTimeStamp; ///< Message Center Time Stamp (4.5.4)
    cdmaPdu_Date_t              validityPeriodAbsolute; ///< Validity Period - Absolute (4.5.5)
    uint8_t                     validityPeriodRelative; ///< Validity Period - Relative (4.5.6)
    cdmaPdu_Date_t              deferredDeliveryTimeAbsolute; ///< Deferred Delivery Time - Absolute (4.5.7)
    uint8_t                     deferredDeliveryTimeRelative; ///< Deferred Delivery Time - Relative (4.5.8)
    cdmaPdu_Priority_t          priority;               ///< Priority Indicator (4.5.9)
    cdmaPdu_Privacy_t           privacy;                ///< Privacy Indicator (4.5.10)
    cdmaPdu_ReplyOption_t       replyOption;            ///< Reply Option (4.5.11)
    uint8_t                     messageCount;           ///< Number of Messages (two 4-bit BCD numbers) (4.5.12)
    cdmaPdu_AlertPriority_t     alertOnMessageDelivery; ///< Alert on Message Delivery (4.5.13)
    cdmaPdu_Language_t          language;               ///< Language Indicator (4.5.14)
    cdmaPdu_AddressParameter_t  callBackNumber;         ///< Call-Back Number (4.5.15)
    cdmaPdu_MessageDisplay_t    messageDisplayMode;     ///< Message Display Mode (4.5.16)
    // not implemented: Multiple Encoding User Data (4.5.17)
    uint16_t                    messageDepositIndex;    ///< Message Deposit Index (4.5.18)
    // not implemented: Service Category Program Data (4.5.19)
    // not implemented: Service Category Program Results (4.5.20)
    cdmaPdu_MessageStatus_t     messageStatus;          ///< Message Status (4.5.21)
    uint8_t                     tpFailureCause;         ///< TP-Failure Cause (4.5.22)
    // not implemented: Enhanced VMN (4.5.23)
    // not implemented: Enhanced VMN Ack (4.5.24)
}
cdmaPdu_BearerData_t;

//--------------------------------------------------------------------------------------------------
/**
 * CDMA Point-To-Point message structure.
 *
 * Based on documentation C.S0015-B V2.0
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    cdmaPdu_ParameterMask_t     parameterMask;          ///< Mask of parameters available
    uint16_t                    teleServiceId;          ///< Teleservice Identifier (3.4.3.1) ( N.S0005-0 section 6.5.2.137)
    cdmaPdu_AddressParameter_t  originatingAddr;        ///< Originating Address (3.4.3.3)
    cdmaPdu_AddressParameter_t  destinationAddr;        ///< Destination Address (3.4.3.3)
    cdmaPdu_ServiceCategory_t   serviceCategory;        ///< Service Category (3.4.3.2)
    cdmaPdu_SubAddress_t        originatingSubaddress;  ///< Originating Subaddress (3.4.3.4)
    cdmaPdu_SubAddress_t        destinationSubaddress;  ///< Destination Subaddress (3.4.3.4)
    cdmaPdu_BearerReplyOption_t bearerReplyOption;      ///< Bearer Reply Option (3.4.3.5)
    cdmaPdu_BearerData_t        bearerData;             ///< Bearer Data (3.4.3.7)
    cdmaPdu_CauseCodes_t        causeCodes;             ///< Cause Codes (3.4.3.6)
}
cdmaPdu_Message_t;

//--------------------------------------------------------------------------------------------------
/**
 * CDMA message type structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    cdmaPdu_MessageFormat_t messageFormat;
    cdmaPdu_Message_t       message;
}
cdmaPdu_t;

//--------------------------------------------------------------------------------------------------
/**
 * Decode the content of dataPtr.
 *
 * return LE_OK function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t cdmaPdu_Decode
(
    const uint8_t   *dataPtr,       ///< [IN] PDU data to decode
    size_t           dataPtrSize,   ///< [IN] Size of the dataPtr
    cdmaPdu_t       *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
);

//--------------------------------------------------------------------------------------------------
/**
 * Encode the content of dataPtr.
 */
//--------------------------------------------------------------------------------------------------
le_result_t cdmaPdu_Encode
(
    const cdmaPdu_t *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    uint8_t         *dataPtr,       ///< [OUT] PDU data to decode
    size_t           dataPtrSize,   ///< [IN] Size of the dataPtr
    uint32_t        *pduByteSize    ///< [OUT] size of the encoded pdu in bytes
);

//--------------------------------------------------------------------------------------------------
/**
 * Print the content of the structure cdmaPdu_t
 */
//--------------------------------------------------------------------------------------------------
void cdmaPdu_Dump
(
    const cdmaPdu_t *cdmaSmsPtr     ///< [IN] Buffer to store decoded data
);

#endif /* CDMAPDU_INCLUDE_GUARD */
