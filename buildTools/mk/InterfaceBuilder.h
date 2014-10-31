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
        void GenerateApiHeaders(const legato::ClientInterface& interface,
                                const std::string& outputDir) const;
        void GenerateApiHeaders(const legato::ServerInterface& interface,
                                const std::string& outputDir) const;
        std::string GenerateApiCode(legato::ClientInterface& interface,
                                    const std::string& outputDir) const;
        std::string GenerateApiCode(legato::ServerInterface& interface,
                                    const std::string& outputDir) const;
        void BuildInterfaceLibrary(legato::Interface& interface,
                                   const std::string& sourceFilePath);

    public:
        void Build(legato::ClientInterface& interface, const std::string& objOutputDir);
        void Build(legato::ServerInterface& interface, const std::string& objOutputDir);

    private:
        std::string GenerateImportDirArgs(const legato::Api_t& api) const;
        void GenerateApiHeaders(const legato::Api_t& api, const std::string& outputDir) const;
};


#endif // INTERFACE_BUILDER_H_INCLUDE_GUARD
