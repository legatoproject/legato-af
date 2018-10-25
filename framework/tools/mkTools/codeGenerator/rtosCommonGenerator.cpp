/**
 * @file rtosCommonGenerator.cpp
 *
 * Common functions used across RTOS code generation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "mkTools.h"

namespace code
{

//--------------------------------------------------------------------------------------------------
/**
 * Convert service-directory interface name to symbol for use in local bindings.
 */
//--------------------------------------------------------------------------------------------------
std::string ConvertInterfaceNameToSymbol
(
    const std::string &interfaceName
)
{
    std::string serviceName = interfaceName;
    std::replace(serviceName.begin(), serviceName.end(),
                 '.', '_');
    return serviceName;
}

}
