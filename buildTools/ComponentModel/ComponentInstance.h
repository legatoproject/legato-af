//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Component Instance class.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef COMPONENT_INSTANCE_H_INCLUDE_GUARD
#define COMPONENT_INSTANCE_H_INCLUDE_GUARD


namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Each object of this type represents a single runtime instance of a software component.
 *
 * @warning This is not to be confused with an @ref Component, which represents a static
 *          component (e.g., on-disk executable file vs. running process).
 *
 * @note    Today we only support one instance of a component per executable.
 **/
//--------------------------------------------------------------------------------------------------
class ComponentInstance
{
    private:
        std::string m_Name;                 ///< Name of the instance.
        Component& m_Component;             ///< The static component that this is an instance of.

    public:
        ComponentInstance(Component& component);
        ComponentInstance(const ComponentInstance& instance);
        ComponentInstance(ComponentInstance&& instance);
        virtual ~ComponentInstance();

    public:
        ComponentInstance& operator =(ComponentInstance&& instance);

    public:
        const std::string& Name() const;
        void Name(const std::string& name);

        Component& GetComponent() { return m_Component; }
        const Component& GetComponent() const { return m_Component; }
};


}


#endif  // COMPONENT_INSTANCE_H_INCLUDE_GUARD
