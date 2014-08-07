//--------------------------------------------------------------------------------------------------
/**
 * Object that stores build parameters gathered from the command line.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef BUILD_PARAMS_H_INCLUDE_GUARD
#define BUILD_PARAMS_H_INCLUDE_GUARD

namespace legato
{


class BuildParams_t
{
    private:
        bool                    m_IsVerbose;
        std::string             m_Target;           ///< (e.g., "localhost" or "ar7")
        std::list<std::string>  m_InterfaceDirs;    ///< Interface search directory paths.
        std::list<std::string>  m_ComponentDirs;    ///< Component search directory paths.
        std::string             m_ExeOutputDir;     ///< Path to directory for build executables.
        std::string             m_LibOutputDir;     ///< Path to directory for built libraries.
        std::string             m_ObjOutputDir;     ///< Dir path for intermediate build products.
        std::string             m_StagingDir;       ///< Dir path for build staging.
        std::string             m_CCompilerFlags;   ///< Flags to be passed to the C compiler.
        std::string             m_LinkerFlags;      ///< Flags to be passed to the linker.

    public:
        BuildParams_t();
        BuildParams_t(const BuildParams_t& original);
        BuildParams_t(BuildParams_t&& original);
        virtual ~BuildParams_t() {};

    public:
        BuildParams_t& operator = (const BuildParams_t& original);
        BuildParams_t& operator = (BuildParams_t&& original);

    public: // Functions for setting up the parameters.
        void SetVerbose() { m_IsVerbose = true; }

        void SetTarget(const std::string& target) { m_Target = target; }
        void SetTarget(std::string&& target) { m_Target = target; }

        void AddInterfaceDir(const std::string& path) { m_InterfaceDirs.push_back(path); }
        void AddInterfaceDir(std::string&& path) { m_InterfaceDirs.push_back(path); }

        void AddComponentDir(const std::string& path) { m_ComponentDirs.push_back(path); }
        void AddComponentDir(std::string&& path) { m_ComponentDirs.push_back(path); }

        void ExeOutputDir(const std::string& path) { m_ExeOutputDir = path; }
        void ExeOutputDir(std::string&& path) { m_ExeOutputDir = std::move(path); }

        void LibOutputDir(const std::string& path) { m_LibOutputDir = path; }
        void LibOutputDir(std::string&& path) { m_LibOutputDir = std::move(path); }

        void ObjOutputDir(const std::string& path) { m_ObjOutputDir = path; }
        void ObjOutputDir(std::string&& path) { m_ObjOutputDir = std::move(path); }

        void StagingDir(const std::string& path) { m_StagingDir = path; }
        void StagingDir(std::string&& path) { m_StagingDir = std::move(path); }

        void CCompilerFlags(const std::string& path) { m_CCompilerFlags = path; }
        void CCompilerFlags(std::string&& path) { m_CCompilerFlags = std::move(path); }

        void LinkerFlags(const std::string& path) { m_LinkerFlags = path; }
        void LinkerFlags(std::string&& path) { m_LinkerFlags = std::move(path); }

    public: // Functions for retrieving the parameters.
        bool IsVerbose() const { return m_IsVerbose; }
        const std::string& Target() const { return m_Target; }
        const std::list<std::string>& InterfaceDirs() const { return m_InterfaceDirs; }
        const std::list<std::string>& ComponentDirs() const { return m_ComponentDirs; }
        const std::string & ExeOutputDir() const { return m_ExeOutputDir; }
        const std::string & LibOutputDir() const { return m_LibOutputDir; }
        const std::string& ObjOutputDir() const { return m_ObjOutputDir; }
        const std::string& StagingDir() const { return m_StagingDir; }
        const std::string& CCompilerFlags() const { return m_CCompilerFlags; }
        const std::string& LinkerFlags() const { return m_LinkerFlags; }
};



}

#endif // BUILD_PARAMS_H_INCLUDE_GUARD
