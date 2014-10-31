//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Executable Builder class, which knows how to build Executable objects.
 *
 * Copyright (C) 2013-2014, Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef EXECUTABLE_BUILDER_H_INCLUDE_GUARD
#define EXECUTABLE_BUILDER_H_INCLUDE_GUARD


class ExecutableBuilder_t
{
    private:
        const legato::BuildParams_t&    m_Params;

    public:
        ExecutableBuilder_t(const legato::BuildParams_t& params) : m_Params(params) {}

    public:
        void GenerateMain(legato::Executable& executable, const std::string& objOutputDir);
        void Build(legato::Executable& executable, const std::string& objOutputDir);

    private:
        void Link(legato::Executable& executable);
};


#endif // EXECUTABLE_BUILDER_H_INCLUDE_GUARD
