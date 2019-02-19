/**
 * @file targetInfo.h
 *
 * Target specific information for principal model nodes (systems, apps, components, etc.)
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_DEFTOOLS_TARGET_INFO_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_TARGET_INFO_H_INCLUDE_GUARD

/**
 * Base class for target-specific info.
 *
 * Base class is empty, but we need a common base class so we can safely downcast to the real type.
 */
class TargetInfo_t
{
    protected:
        // Do not allow base class to be instantiated directly.
        TargetInfo_t() {}

    public:
        virtual ~TargetInfo_t() { }
};

/**
 * Base class for any class which can have some target-specific info.
 */
struct HasTargetInfo_t
{
    /// Set of target-specific information generated during the various build steps.
    std::unordered_map<std::type_index, std::shared_ptr<TargetInfo_t> > targetInfo;

    // Type-safe getter for some target information
    template<class T>
    const T* GetTargetInfo(void) const
    {
        try
        {
            return dynamic_cast<const T*>(targetInfo.at(typeid(T)).get());
        }
        catch (std::out_of_range& e)
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("INTERNAL ERROR: Trying to get target info '%s', which is"
                                   " not set for the current target."), typeid(T).name())
            );
        }
    }

    template<class T>
    T* GetTargetInfo(void)
    {
        try
        {
            return dynamic_cast<T*>(targetInfo.at(typeid(T)).get());
        }
        catch (std::out_of_range& e)
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("INTERNAL ERROR: Trying to get target info '%s', which is"
                                   " not set for the current target."), typeid(T).name())
            );
        }
    }

    /// Give this app some target info
    template<class T>
    void SetTargetInfo(T* targetInfoPtr)
    {
        targetInfo.insert(std::make_pair(std::type_index(typeid(T)),
                                         std::shared_ptr<TargetInfo_t>(targetInfoPtr)));
    }
};

#endif /* LEGATO_DEFTOOLS_TARGET_INFO_H_INCLUDE_GUARD */
