//--------------------------------------------------------------------------------------------------
/**
 * Class that holds a thread priority.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef PRIORITY_H_INCLUDE_GUARD
#define PRIORITY_H_INCLUDE_GUARD


class Priority_t : public Limit_t
{
    public:

        virtual ~Priority_t() {}

        //------------------------------------------------------------------------------------------
        /**
         * Enumerations of selected priority levels.
         *
         * Real-time priorities are numbered from 1 to 32.
         */
        //------------------------------------------------------------------------------------------
        typedef enum
        {
            IDLE = -3,
            LOW = -2,
            MEDIUM = -1,
            HIGH = 0,
        }
        PriorityLevel_t;

    protected:

        std::string value;    ///< The value, as a string.
        int numericalValue;   ///< Numerical representation of the value.

    public:

        Priority_t& operator =(const std::string& value);

        const std::string& Get() const;
        int GetNumericalValue() const;

        bool operator >(const Priority_t& other);

        bool IsRealTime() const;
};


#endif // PRIORITY_H_INCLUDE_GUARD
