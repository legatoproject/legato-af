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

        /// Map of client-side interface names to api files.  Needs its own copy of the Component's
        /// list because the names could be changed by settings in the .adef file.
        ImportedInterfaceMap m_RequiredApis;

        /// Map of server-side interface names to api files.  Needs its own copy of the Component's
        /// list because the names could be changed by settings in the .adef file.
        ExportedInterfaceMap m_ProvidedApis;

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

        ImportedInterfaceMap& RequiredApis(void) { return m_RequiredApis; }
        const ImportedInterfaceMap& RequiredApis(void) const { return m_RequiredApis; }
        ExportedInterfaceMap& ProvidedApis(void) { return m_ProvidedApis; }
        const ExportedInterfaceMap& ProvidedApis(void) const { return m_ProvidedApis; }

        Interface& FindInterface(const std::string& name);
        ImportedInterface& FindClientInterface(const std::string& name);
        ExportedInterface& FindServerInterface(const std::string& name);
};


}


#endif  // COMPONENT_INSTANCE_H_INCLUDE_GUARD
