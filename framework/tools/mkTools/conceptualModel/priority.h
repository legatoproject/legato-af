//--------------------------------------------------------------------------------------------------
/**
 * Class that holds a thread priority.
 *
 * Copyright (C) Sierra Wireless, Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef PRIORITY_H_INCLUDE_GUARD
#define PRIORITY_H_INCLUDE_GUARD


class Priority_t : public Limit_t
{
    public:

        virtual ~Priority_t() {}

    protected:

        std::string value;    ///< The value, as a string.
        int numericalValue;   ///< Numerical representation of the value (for internal use only).

    public:

        Priority_t& operator =(const std::string& value);

        const std::string& Get() const;

        bool operator >(const Priority_t& other);

        bool IsRealTime() const;
};


#endif // PRIORITY_H_INCLUDE_GUARD
