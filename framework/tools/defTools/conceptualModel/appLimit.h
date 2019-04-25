//--------------------------------------------------------------------------------------------------
/**
 * Base class for various configurable limits, such as memory limits and CPU limits, that
 * can be placed on processes, applications, etc.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef APP_LIMIT_H_INCLUDE_GUARD
#define APP_LIMIT_H_INCLUDE_GUARD


class Limit_t
{
    public:

        Limit_t() : isSet(false) {}

    protected:

        bool isSet;

    public:

        bool IsSet() const { return isSet; }
};



#include "boolLimit.h"
#include "nonNegativeIntLimit.h"
#include "positiveIntLimit.h"
#include "watchdogTimeout.h"
#include "watchdogAction.h"
#include "faultAction.h"
#include "priority.h"


#endif // APP_LIMIT_H_INCLUDE_GUARD
