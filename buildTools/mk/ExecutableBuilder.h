//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Executable Builder class, which knows how to build Executable objects.
 *
 * Copyright (C) 201 Sierra Wireless Inc., all rights reserved.
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
        void GenerateMain(legato::Executable& executable);
        void Build(const legato::Executable& executable);
};


#endif // EXECUTABLE_BUILDER_H_INCLUDE_GUARD
