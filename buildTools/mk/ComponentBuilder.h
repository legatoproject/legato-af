//--------------------------------------------------------------------------------------------------
/**
 * Object that knows how to build and bundle Components.
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
        void Build(legato::Component& component, const std::string& objOutputDir);
        void Bundle(legato::Component& component);

    private:
        void GenerateInterfaceHeaders(legato::Component& component, const std::string& outputDir);
        void GenerateInterfacesHeader(legato::Component& component, const std::string& outputDir);
        void BuildObjectFiles(legato::Component& component, const std::string& outputDir);
        void CompileCSourceFile(legato::Component& component, const std::string& file, const std::string& outputDir);
        void CompileCxxSourceFile(legato::Component& component, const std::string& file, const std::string& outputDir);
        void LinkLib(legato::Component& component);
        void LinkSharedLib(legato::Component& component);
        void LinkStaticLib(legato::Component& component);
};


#endif // COMPONENT_BUILDER_H_INCLUDE_GUARD
