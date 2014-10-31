//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Component Instance class.
 *
 * Copyright (C) 2013 Sierra Wireless Inc.  Use of this work is subject to license.
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

        Component& m_Component;             ///< The static component that this is an instance of.

        /// Map of client-side interface names interface objects.
        /// Copied here from the Component object when the Component Instance is created.
        ClientInterfaceMap m_RequiredApis;

        /// Map of server-side interface names to interface objects.
        /// Copied here from the Component object when the Component Instance is created.
        ServerInterfaceMap m_ProvidedApis;

        /// Set of pointers to sub-component instances that this component instance depends on.
        std::set<ComponentInstance*> m_SubInstances;

        /// Pointer to the Executable that this component instance is a part of.
        Executable* m_ExePtr;

    public:

        ComponentInstance(Component& component, Executable* exePtr);
        ComponentInstance(const ComponentInstance& instance);
        ComponentInstance(ComponentInstance&& instance);
        virtual ~ComponentInstance();

    public:

        ComponentInstance& operator =(ComponentInstance&& instance);

    public:

        std::string Name() const { return m_Component.Name(); /* Only singletons allowed for now */}
        std::string AppUniqueName() const;

        Component& GetComponent() { return m_Component; }
        const Component& GetComponent() const { return m_Component; }

        ClientInterfaceMap& RequiredApis(void) { return m_RequiredApis; }
        const ClientInterfaceMap& RequiredApis(void) const { return m_RequiredApis; }
        ServerInterfaceMap& ProvidedApis(void) { return m_ProvidedApis; }
        const ServerInterfaceMap& ProvidedApis(void) const { return m_ProvidedApis; }

        Interface& FindInterface(const std::string& name);
        ClientInterface& FindClientInterface(const std::string& name);
        ServerInterface& FindServerInterface(const std::string& name);

        const std::set<ComponentInstance*>& SubInstances() const { return m_SubInstances; }
        std::set<ComponentInstance*>& SubInstances() { return m_SubInstances; }

        void SetExe(Executable* exePtr);
        Executable* Exe() { return m_ExePtr; }
};


}


#endif  // COMPONENT_INSTANCE_H_INCLUDE_GUARD
