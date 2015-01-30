//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Library class.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LIBRARY_H_INCLUDE_GUARD
#define LIBRARY_H_INCLUDE_GUARD


namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Each object of this type represents a single shared library.
 **/
//--------------------------------------------------------------------------------------------------
class Library
{
    public:
        Library() : m_IsUpToDate(false), m_IsStatic(false), m_Exists(false) {}
        Library(const Library& original);
        Library(Library&& original);

    private:
        std::string m_ShortName;        ///< Name of the library (minus the "lib" and ".so")
        std::string m_BuildOutputDir;   ///< File system path to the directory on the build host.
        std::string m_BuildOutputPath;  ///< File system path of the library file
                                        ///  If set, overrides m_ShortName and m_BuildOutputDir.

        bool m_IsUpToDate;              ///< true if we think the library doesn't need to be built.
        bool m_IsStatic;                ///< true if static (.a) library.  Falso if shared (.so).
        bool m_Exists;                  ///< true if the library file actually exists.

    public:
        Library& operator =(const Library& library);
        Library& operator =(Library&& library);

    public:
        void ShortName(const std::string& name) { m_ShortName = name; }
        void ShortName(std::string&& name) { m_ShortName = std::move(name); }
        const std::string& ShortName() const { return m_ShortName; }

        void BuildOutputDir(const std::string& path) { m_BuildOutputDir = path; }
        void BuildOutputDir(std::string&& path) { m_BuildOutputDir = std::move(path); }
        const std::string& BuildOutputDir() const { return m_BuildOutputDir; }

        std::string BuildOutputPath() const;
        void BuildOutputPath(const std::string& path) { m_BuildOutputPath = path; }

        void MarkUpToDate() { m_IsUpToDate = true; }
        void MarkOutOfDate() { m_IsUpToDate = false; }
        bool IsUpToDate() const { return m_IsUpToDate; }
        bool IsOutOfDate() const { return !m_IsUpToDate; }

        void IsStatic(bool isStatic) { m_IsStatic = isStatic; }
        bool IsStatic() const { return m_IsStatic; }

        void MarkExisting() { m_Exists = true; }
        bool Exists() const { return m_Exists; }
};


}


#endif  // LIBRARY_H_INCLUDE_GUARD
