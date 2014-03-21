//--------------------------------------------------------------------------------------------------
/**
 * Object that knows how to build Components.
 *
 * Copyright (C) 201 Sierra Wireless Inc., all rights reserved.
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
        void GenerateInterfaceCode(legato::Component& component);
        void Build(const legato::Component& component);
};


#endif // COMPONENT_BUILDER_H_INCLUDE_GUARD
