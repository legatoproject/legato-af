//--------------------------------------------------------------------------------------------------
/**
 * Object that knows how to build Components.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef COMPONENT_BUILDER_H_INCLUDE_GUARD
#define COMPONENT_BUILDER_H_INCLUDE_GUARD


class ComponentBuilder_t
{
    private:
        const legato::BuildParams_t&    m_Params;

    public:
        ComponentBuilder_t(const legato::BuildParams_t& params) : m_Params(params) {}

    public:
        void GenerateInterfacesHeader(legato::Component& component);
        void BuildInterfaces(legato::Component& component);
        void BuildComponentLib(legato::Component& component);
        void Build(legato::Component& component);
};


#endif // COMPONENT_BUILDER_H_INCLUDE_GUARD
