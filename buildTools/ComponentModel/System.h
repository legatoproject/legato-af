//--------------------------------------------------------------------------------------------------
/**
 *  Definition of the System class.  This class holds all the information specific to a
 *  system of interacting applications.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef SYSTEM_H_INCLUDE_GUARD
#define SYSTEM_H_INCLUDE_GUARD


namespace legato
{


//----------------------------------------------------------------------------------------------
/**
 * Represents a System.
 */
//----------------------------------------------------------------------------------------------
class System
{
    private:
        std::string m_Name;             ///< Name of the system.

        std::string m_Version;          ///< Version of the system.

        std::string m_DefFilePath;      ///< Path to the .sdef file.

        /// List of applications that exist in the system, keyed by application name.
        std::map<std::string, App> m_Apps;

        /// Map of client-side interface names (user.interface) to user-to-user IPC API binds.
        std::map<std::string, UserToUserApiBind> m_ApiBinds;

    public:
        System() {};
        System(System&& system);
        System(const System& system) = delete;
        virtual ~System() {};

    public:
        System& operator =(System&& system);
        System& operator =(const System& system) = delete;

    public:
        void Name(const std::string& name) { m_Name = name; }
        void Name(std::string&& name) { m_Name = std::move(name); }
        const std::string& Name() const { return m_Name; }

        void Version(const std::string& version) { m_Version = version; }
        void Version(std::string&& version) { m_Version = std::move(version); }
        const std::string& Version() const { return m_Version; }
        std::string& Version() { return m_Version; }

        void DefFilePath(const std::string& path);
        void DefFilePath(std::string&& path);
        const std::string& DefFilePath() const { return m_DefFilePath; }

        const std::string Path() const { return std::move(GetContainingDir(m_DefFilePath)); }

        App& CreateApp(const std::string& adefPath);
        const std::map<std::string, App>& Apps() const { return m_Apps; }
        std::map<std::string, App>& Apps() { return m_Apps; }

        void AddApiBind(UserToUserApiBind&& bind);
        const std::map<std::string, UserToUserApiBind>& ApiBinds() const { return m_ApiBinds; }
};



}

#endif  // SYSTEM_H_INCLUDE_GUARD
