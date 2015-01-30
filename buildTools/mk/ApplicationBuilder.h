//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Application Builder class, which knows how to build Application objects.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef APPLICATION_BUILDER_H_INCLUDE_GUARD
#define APPLICATION_BUILDER_H_INCLUDE_GUARD


class ApplicationBuilder_t
{
    private:
        const legato::BuildParams_t&    m_Params;

    public:
        ApplicationBuilder_t(const legato::BuildParams_t& params) : m_Params(params) {}

    public:
        void Build(legato::App& app, std::string outputDirPath);
};


#endif // APPLICATION_BUILDER_H_INCLUDE_GUARD
