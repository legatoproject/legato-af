//--------------------------------------------------------------------------------------------------
/**
 * Object that knows how to build Component Instances.
 *
 * When a Component Instance is built, the component library, all of the interface libraries, and
 * all sub-component instances will be built before the component instance library itself is built.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef COMPONENT_INSTANCE_BUILDER_H_INCLUDE_GUARD
#define COMPONENT_INSTANCE_BUILDER_H_INCLUDE_GUARD


class ComponentInstanceBuilder_t
{
    private:
        const legato::BuildParams_t&    m_Params;

    public:
        ComponentInstanceBuilder_t(const legato::BuildParams_t& params) : m_Params(params) {}

    public:
        void Build(legato::ComponentInstance& instance);

    private:
        void BuildInterfaces(legato::ComponentInstance& instance);
};


#endif // COMPONENT_INSTANCE_BUILDER_H_INCLUDE_GUARD
