/**
 * This module implements the atServer utilities.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#ifndef _ATSERVERUTIL_H__
#define _ATSERVERUTIL_H__

#include "legato.h"
#include "interfaces.h"

#define AT_SHORT_PARAM_RESP_MAX_BYTES 32
#define AT_LONG_PARAM_RESP_MAX_BYTES 96
#define AT_COMMAND_MAX_BYTES 32

typedef enum
{
    ATSERVERUTIL_OK,
    ATSERVERUTIL_NO_CARRIER,
    ATSERVERUTIL_ERROR
}
atServerUtil_FinalRsp_t;

//--------------------------------------------------------------------------------------------------
/**
 * atServerUtil_AtCmd_t definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char*                      cmdPtr;
    le_atServer_CmdRef_t             cmdRef;
    le_atServer_CommandHandlerFunc_t handlerPtr;
    void*                            contextPtr;
}
atServerUtil_AtCmd_t;

enum
{
    CME_ER_PHONE_FAILURE,                             /* CME ERROR: 0 */
    CME_ER_NO_PHONE_CONNECTION,                       /* CME ERROR: 1 */
    CME_ER_PHONE_ADAPTOR_RESERVED,                    /* CME ERROR: 2 */
    CME_ER_OPERATION_NOT_ALLOWED,                     /* CME ERROR: 3 */
    CME_ER_OPERATION_NOT_SUPPORTED,                   /* CME ERROR: 4 */
    CME_ER_PH_SIM_PIN_REQUIRED,                       /* CME ERROR: 5 */
    CME_ER_PH_FSIM_PIN_REQUIRED,                      /* CME ERROR: 6 */
    CME_ER_PH_FSIM_PUK_REQUIRED,                      /* CME ERROR: 7 */
    CME_ER_NO_SIM = 10,                               /* CME ERROR: 10 */
    CME_ER_PIN_REQUIRED,                              /* CME ERROR: 11 */
    CME_ER_PUK_REQUIRED,                              /* CME ERROR: 12 */
    CME_ER_SIM_FAILURE,                               /* CME ERROR: 13 */
    CME_ER_SIM_BUSY,                                  /* CME ERROR: 14 */
    CME_ER_SIM_WRONG,                                 /* CME ERROR: 15 */
    CME_ER_INCORRECT_PASSWORD,                        /* CME ERROR: 16 */
    CME_ER_SIM_PIN2_REQUIRED,                         /* CME ERROR: 17 */
    CME_ER_SIM_PUK2_REQUIRED,                         /* CME ERROR: 18 */
    CME_ER_MEMORY_FULL = 20,                          /* CME ERROR: 20 */
    CME_ER_INVALID_INDEX,                             /* CME ERROR: 21 */
    CME_ER_NOT_FOUND,                                 /* CME ERROR: 22 */
    CME_ER_MEMORY_FAILURE,                            /* CME ERROR: 23 */
    CME_ER_TEXT_STRING_TOO_LONG,                      /* CME ERROR: 24 */
    CME_ER_INVALID_CHAR_IN_TEXT,                      /* CME ERROR: 25 */
    CME_ER_DIAL_STRING_TOO_LONG,                      /* CME ERROR: 26 */
    CME_ER_INVALID_CHAR_IN_DIAL_STR,                  /* CME ERROR: 27 */
    CME_ER_NO_NETWORK_SERVICE = 30,                   /* CME ERROR: 30 */
    CME_ER_NETWORK_TIMEOUT,                           /* CME ERROR: 31 */
    CME_ER_NETWORK_NOT_ALLOWED,                       /* CME ERROR: 32 */
    CME_ER_NTWK_PERSO_PIN_REQUIRED=40,                /* CME ERROR: 40 */
    CME_ER_NTWK_PERSO_PUK_REQUIRED,                   /* CME ERROR: 41 */
    CME_ER_NTWK_SUBSET_PERSO_PIN_REQUIRED,            /* CME ERROR: 42 */
    CME_ER_NTWK_SUBSET_PERSO_PUK_REQUIRED,            /* CME ERROR: 43 */
    CME_ER_SVC_PROVIDER_PIN_REQUIRED,                 /* CME ERROR: 44 */
    CME_ER_SVC_PROVIDER_PUK_REQUIRED,                 /* CME ERROR: 45 */
    CME_ER_CORPORATE_PERSO_PIN_REQUIRED,              /* CME ERROR: 46 */
    CME_ER_CORPORATE_PERSO_PUK_REQUIRED,              /* CME ERROR: 47 */
    CME_ER_HIDEN_KEY_REQUIRED,                        /* CME ERROR: 48 */
    CME_ER_EAP_METHOD_NOT_SUPPORTED,                  /* CME ERROR: 49 */
    CME_ER_INCORRECT_PARAMETERS,                      /* CME ERROR: 50 */
    CME_ER_SYSTEM_FAILURE=60,                         /* CME ERROR: 60 */
    CME_ER_RESOURCE_LIMITATION = 99,                  /* CME ERROR: 99 */
    CME_ER_UNKNOWN,                                   /* CME ERROR: 100 */
    /* AT GPRS extension */
    CME_ER_ILLEGAL_MS=103,                            /* CME ERROR: 103 */
    CME_ER_ILLEGAL_ME=106,                            /* CME ERROR: 106 */
    CME_ER_GPRS_SERVICE_NOT_ALLOWED,                  /* CME ERROR: 107 */
    CME_ER_PLMN_NOT_ALLOWED = 111,                    /* CME ERROR: 111 */
    CME_ER_LOCATION_AREA_NOT_ALLOWED,                 /* CME ERROR: 112 */
    CME_ER_ROAMING_NOT_ALLOWED_IN_THIS_LOCATION_AREA, /* CME ERROR: 113 */
    CME_ER_SERVICE_OPTION_NOT_SUPPORTED = 132,        /* CME ERROR: 132 */
    CME_ER_REQUESTED_SERVICE_OPTION_NOT_SUBSCRIBED,   /* CME ERROR: 133 */
    CME_ER_SERVICE_OPTION_TEMPORARILY_OUT_OF_ORDER,   /* CME ERROR: 134 */
    CME_ER_UNSPECIFIED_GPRS_ERROR = 148,              /* CME ERROR: 148 */
    CME_ER_PDP_AUTHENTICATION_FAILURE,                /* CME ERROR: 149 */
    CME_ER_INVALID_MOBILE_CLASS,                      /* CME ERROR: 150 */

    /* AT AVMS error codes */
    CME_ER_AVMS_GENERAL_ERROR=650,                    /* CME ERROR: 650 */
    CME_ER_AVMS_COMMUNICATION_ERROR,                  /* CME ERROR: 651 */
    CME_ER_AVMS_SESSION_IN_PROGRESS,                  /* CME ERROR: 652 */
    CME_ER_AVMS_DEACTIVATED,                          /* CME ERROR: 654 */
    CME_ER_AVMS_PROHIBITED,                           /* CME ERROR: 655 */
    CME_ER_AVMS_TO_BE_PROVISIONED,                    /* CME ERROR: 656 */
    /* AT sim security error codes */
    CME_ER_SIM_SECURITY_UNSPECIFIED_ERROR = 800,      /* CME ERROR: 800 */

    /* AT protocol specific error codes */
    CME_ER_NO_SOCKET_AVAILABLE = 902,                 /* CME ERROR: 902 */
    CME_ER_MEMORY_PROBLEM,                            /* CME ERROR: 903 */
    CME_ER_DNS_ERROR,                                 /* CME ERROR: 904 */
    CME_ER_REMOTE_TCP_DISCONNECTION,                  /* CME ERROR: 905 */
    CME_ER_UDP_TCP_CONNECTION_ERROR,                  /* CME ERROR: 906 */
    CME_ER_GENERIC_ERROR,                             /* CME ERROR: 907 */
    CME_ER_FAILED_TO_ACCEPT_CLIENT_REQUEST,           /* CME ERROR: 908 */
    CME_ER_INCOHERENT_DATA,                           /* CME ERROR: 909 */
    CME_ER_BAD_SESSION_ID,                            /* CME ERROR: 910 */
    CME_ER_SESSION_ALREADY_RUNNIG,                    /* CME ERROR: 911 */
    CME_ER_ALL_SESSIONS_USED,                         /* CME ERROR: 912 */
    CME_ER_SOCKET_CONNECTION_TIMEOUT,                 /* CME ERROR: 913 */
    CME_ER_CONTROL_SOCKET_CONNECTION_TIMEOUT,         /* CME ERROR: 914 */
    CME_ER_PARAMETER_NOT_EXPECTED,                    /* CME ERROR: 915 */
    CME_ER_PARAMETER_INVALID_RANGE,                   /* CME ERROR: 916 */
    CME_ER_MISSING_PARAMETER,                         /* CME ERROR: 917 */
    CME_ER_FEATURE_NOT_SUPPORTED,                     /* CME ERROR: 918 */
    CME_ER_FEATURE_NOT_AVAILABLE,                     /* CME ERROR: 919 */
    CME_ER_PROTOCOL_NOT_SUPPORTED,                    /* CME ERROR: 920 */
    CME_ER_BEARER_CONNECTION_INVALID_STATE,           /* CME ERROR: 921 */
    CME_ER_SESSION_INVALID_STATE,                     /* CME ERROR: 922 */
    CME_ER_TERMINATE_PRT_DATA_MODE_INVALID_STATE,     /* CME ERROR: 923 */
    CME_ER_SESSION_BUSY,                              /* CME ERROR: 924 */
    CME_ER_HTTP_HEADER_NAME_ERROR,                    /* CME ERROR: 925 */
    CME_ER_HTTP_HEADER_VALUE_ERROR,                   /* CME ERROR: 926 */
    CME_ER_HTTP_HEADER_NAME_EMPTY,                    /* CME ERROR: 927 */
    CME_ER_HTTP_HEADER_VALUE_EMPTY,                   /* CME ERROR: 928 */
    CME_ER_INPUT_DATA_FORMAT_INVALID,                 /* CME ERROR: 929 */
    CME_ER_INPUT_DATA_CONTENT_INVALID,                /* CME ERROR: 930 */
    CME_ER_PARAMETER_LENGTH_INVALID,                  /* CME ERROR: 931 */
    CME_ER_PARAMETER_FORMAT_INVALID                   /* CME ERROR: 932 */
};

#define FIRST_IP_ERROR_CODE CME_ER_NO_SOCKET_AVAILABLE
#define LAST_IP_ERROR_CODE CME_ER_PARAMETER_FORMAT_INVALID
//------------------------------------------------------------------------------
/**
 * This function creates the command reference and install its handler
 *
 * @return
 *      - LE_FAULT         Function failed.
 *      - LE_DUPLICATE     The command reference already exists.
 *      - LE_OK            Function succeeded.
 */
//------------------------------------------------------------------------------
LE_SHARED le_result_t atServerUtil_InstallCmdHandler
(
    atServerUtil_AtCmd_t* atCmdPtr  ///< [IN] AT command definition.
);

//------------------------------------------------------------------------------
/**
 * This function converts ascii parameter of AT command in its numeric value
 *
 * @return
 *      - LE_FAULT         Function failed.
 *      - LE_NOT_FOUND     no parameter.
 *      - LE_OK            Function succeeded.
 */
//------------------------------------------------------------------------------
LE_SHARED le_result_t atServerUtil_GetDigitParameter
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    uint32_t index,
        ///< [IN] agument index

    char* parameter,
        ///< [OUT] parameter value

    size_t parameterNumElements,
        ///< [IN]

    uint32_t *numericParameter
        ///< [OUT] decimal parameter value

);

//------------------------------------------------------------------------------
/**
 * This function converts ascii parameter of AT command to its long numeric value
 *
 * @return
 *      - LE_FAULT         Function failed to convert given parameter.
 *      - LE_NOT_FOUND     no parameter has been given for conversion.
 *      - LE_OK            Given parameter has been converted to it's long format.
 */
//------------------------------------------------------------------------------
LE_SHARED le_result_t atServerUtil_GetLongDigitParameter
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    uint32_t index,
        ///< [IN] agument index

    char* parameter,
        ///< [OUT] parameter value

    size_t parameterNumElements,
        ///< [IN]

    uint64_t *numericParameter
        ///< [OUT] decimal parameter value

);

//------------------------------------------------------------------------------
/**
 * This function gets the parameter string at a given index and
 * converts ascii hex parameter of AT command to its numeric value
 *
 * @return
 *      - LE_FAULT         Function failed.
 *      - LE_NOT_FOUND     no parameter.
 *      - LE_OK            Function succeeded.
 */
//------------------------------------------------------------------------------
LE_SHARED le_result_t atServerUtil_GetHexDigitParameter
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    uint32_t index,
        ///< [IN] argument index

    char* parameterPtr,
        ///< [OUT] pointer to parameter string

    size_t parameterNumElements,
        ///< [IN]Buffer available in parameterPtr

    uint32_t *numericParameter
        ///< [OUT] decimal parameter value

);

//------------------------------------------------------------------------------
/**
 * This function get ascii parameter of AT command
 *
 * @return
 *      - LE_FAULT         Function failed.
 *      - LE_NOT_FOUND     no parameter.
 *      - LE_OK            Function succeeded.
 *      - LE_OVERFLOW      If parameter too long.
 */
//------------------------------------------------------------------------------
LE_SHARED le_result_t atServerUtil_GetStrParameter
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    uint32_t index,
        ///< [IN] agument index

    char* parameter,
        ///< [OUT] parameter value

    size_t parameterNumElements,
            ///< [IN]

    char *strParameter,
        ///< [OUT] decimal parameter value

    size_t strParameterLen
            ///< [IN]
);
#endif
