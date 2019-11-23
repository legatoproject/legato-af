/**
 * This module implements the atServer utilities.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "atServerUtil.h"

#define ATSERVERUTIL_CONNECT "\r\nCONNECT\r\n"
#define ATSERVERUTIL_NOCARRIER "NO CARRIER"

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
le_result_t atServerUtil_InstallCmdHandler
(
    atServerUtil_AtCmd_t* atCmdPtr  ///< [IN] AT command definition.
)
{
    if (NULL == atCmdPtr)
    {
        LE_ERROR("atCmdPtr is NULL!");
        return LE_FAULT;
    }
    if (NULL != atCmdPtr->cmdRef)
    {
        LE_ERROR("atCmdPtr->cmdRef already exists!");
        return LE_DUPLICATE;
    }
    if (NULL == atCmdPtr->handlerPtr)
    {
        LE_ERROR("atCmdPtr->handlerPtr is NULL!");
        return LE_FAULT;
    }
    atCmdPtr->cmdRef = le_atServer_Create(atCmdPtr->cmdPtr);
    if (NULL == atCmdPtr->cmdRef)
    {
        LE_ERROR("Cannot create a command: atCmdPtr->cmdRef is NULL!");
        return LE_FAULT;
    }
    le_atServer_AddCommandHandler(atCmdPtr->cmdRef, atCmdPtr->handlerPtr, atCmdPtr->contextPtr);
    return LE_OK;
}


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
le_result_t atServerUtil_GetDigitParameter
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

)
{
    int32_t digit = 0;
    char *endptr;

    if (LE_OK != le_atServer_GetParameter(commandRef, index, parameter, parameterNumElements))
    {
        LE_ERROR("failed to get param #%u", (unsigned int)index);
        goto err;
    }
    if(strlen(parameter) == 0)
    {
        goto noParam;
    }
    errno = 0;
    digit = strtol(parameter, &endptr, 10);
    if((*endptr != '\0') || (errno != 0))
    {
        LE_WARN("failed parameter \"%s\" conversion ",parameter);
        goto err;
    }
    *numericParameter = digit;

    return LE_OK;

noParam:
    return LE_NOT_FOUND;

err:
    return LE_FAULT;

}


//------------------------------------------------------------------------------
/**
 * This function converts ascii parameter of AT command to its long numeric value
 *
 * @return
 *      - LE_FAULT         Function failed to convert given parameter.
 *      - LE_NOT_FOUND     no parameter has been given for conversion.
 *      - LE_OK            Given parameter has been converted to its long format.
 */
//------------------------------------------------------------------------------
le_result_t atServerUtil_GetLongDigitParameter
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

)
{
    int64_t digit = 0;
    char *endptr;

    if (LE_OK != le_atServer_GetParameter(commandRef, index, parameter, parameterNumElements))
    {
        LE_ERROR("failed to get param #%u", (unsigned int)index);
        goto err;
    }
    if(strlen(parameter) == 0)
    {
        goto noParam;
    }
    errno = 0;
    digit = strtoll(parameter, &endptr, 10);
    if((*endptr != '\0') || (errno != 0))
    {
        LE_WARN("failed parameter \"%s\" conversion ",parameter);
        goto err;
    }
    *numericParameter = digit;

    return LE_OK;

noParam:
    return LE_NOT_FOUND;

err:
    return LE_FAULT;

}

//------------------------------------------------------------------------------
/**
 * This function gets the parameter string at a given index and
 * converts the Hexadecimal ascii parameter of AT command to its numeric value
 *
 * @return
 *      - LE_FAULT         Function failed.
 *      - LE_NOT_FOUND     no parameter.
 *      - LE_OK            Function succeeded.
 */
//------------------------------------------------------------------------------
le_result_t atServerUtil_GetHexDigitParameter
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    uint32_t index,
        ///< [IN] argument index

    char* parameterPtr,
        ///< [OUT] Pointer to parameter string value

    size_t parameterNumElements,
        ///< [IN]Buffer available in parameterPtr

    uint32_t *numericParameter
        ///< [OUT] decimal parameter value

)
{
    uint32_t digit = 0;
    char *endptr;

    if (NULL == parameterPtr)
    {
        LE_ERROR("NULL pointer passed for parameter string");
        goto err;
    }

    if (LE_OK != le_atServer_GetParameter(commandRef, index, parameterPtr, parameterNumElements))
    {
        LE_ERROR("failed to get param #%u", (unsigned int)index);
        goto err;
    }
    if(strlen(parameterPtr) == 0)
    {
        goto noParam;
    }
    errno = 0;
    digit = strtoul(parameterPtr, &endptr, 16);
    if((*endptr != '\0') || (errno != 0))
    {
        LE_WARN("failed parameter \"%s\" conversion ",parameterPtr);
        goto err;
    }
    *numericParameter = digit;

    return LE_OK;

noParam:
    return LE_NOT_FOUND;

err:
    return LE_FAULT;

}

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
le_result_t atServerUtil_GetStrParameter
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
)
{
    if (LE_OK != le_atServer_GetParameter(commandRef, index, parameter, parameterNumElements))
    {
        LE_ERROR("failed to get param #%u", (unsigned int)index);
        return LE_FAULT;
    }
    if(strlen(parameter) == 0)
    {
        return LE_NOT_FOUND;
    }
    errno = 0;
    if (LE_OK != le_utf8_Copy(strParameter, parameter, strParameterLen, NULL))
    {
        return LE_OVERFLOW;
    }
    return LE_OK;
}


//------------------------------------------------------------------------------
/**
 * Component initializer.
 *
 */
//------------------------------------------------------------------------------
COMPONENT_INIT
{
}
