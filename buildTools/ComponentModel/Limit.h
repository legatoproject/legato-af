//--------------------------------------------------------------------------------------------------
/**
 * Base class for various configurable limits, such as memory limits and CPU limits, that
 * can be placed on processes, applications, etc.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LIMIT_H_INCLUDE_GUARD
#define LIMIT_H_INCLUDE_GUARD

namespace legato
{


class Limit_t
{
    public:

        Limit_t() : m_IsSet(false) {}
        virtual ~Limit_t() {}

    protected:

        bool m_IsSet;

    public:

        bool IsSet() const { return m_IsSet; }
};


}


#include "BoolLimit.h"
#include "NonNegativeIntLimit.h"
#include "PositiveIntLimit.h"
#include "WatchdogTimeout.h"
#include "WatchdogAction.h"
#include "FaultAction.h"
#include "Priority.h"


#endif // LIMIT_H_INCLUDE_GUARD
