//--------------------------------------------------------------------------------------------------
/**
 * Class that holds a thread priority.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef PRIORITY_H_INCLUDE_GUARD
#define PRIORITY_H_INCLUDE_GUARD

namespace legato
{


class Priority_t : public Limit_t
{
    public:

        Priority_t() {}
        Priority_t(const Priority_t& original);
        virtual ~Priority_t() {}

    protected:

        std::string m_Value;    ///< The value, as a string.
        int m_NumericalValue;   ///< Numerical representation of the value (for internal use only).

    public:

        Priority_t& operator =(const Priority_t& original);
        Priority_t& operator =(Priority_t&& original);

        void operator =(const std::string& value);
        void operator =(std::string&& value);

        const std::string& Get() const;

        bool operator >(const Priority_t& other);

        bool IsRealTime() const;
};


}   // namespace legato

#endif // PRIORITY_H_INCLUDE_GUARD
