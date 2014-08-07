//--------------------------------------------------------------------------------------------------
/**
 * Object that knows how to build Interfaces.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef INTERFACE_BUILDER_H_INCLUDE_GUARD
#define INTERFACE_BUILDER_H_INCLUDE_GUARD


class InterfaceBuilder_t
{
    private:
        const legato::BuildParams_t&    m_Params;

    public:
        InterfaceBuilder_t(const legato::BuildParams_t& params) : m_Params(params) {}

    public:
        void GenerateApiClientHeaders(const legato::ImportedInterface& interface);
        void GenerateApiServerHeaders(const legato::ExportedInterface& interface);
        std::string GenerateApiClientCode(legato::ImportedInterface& interface);
        std::string GenerateApiServerCode(legato::ExportedInterface& interface);
        void BuildInterfaceLibrary(legato::Interface& interface,
                                   const std::string& sourceFilePath);

    public:
        void Build(legato::ExportedInterface& interface);
        void Build(legato::ImportedInterface& interface);
};


#endif // INTERFACE_BUILDER_H_INCLUDE_GUARD
