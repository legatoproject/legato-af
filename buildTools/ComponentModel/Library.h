//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Library class.
 *
 * Copyright (C) 2014 Sierra Wireless Inc.  Use of this work is subject to license.
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
        Library() : m_IsUpToDate(false) {}
        Library(const Library& original);
        Library(Library&& original);

    private:
        std::string m_ShortName;        ///< Name of the library (minus the "lib" and ".so")
        std::string m_BuildOutputDir;   ///< File system path to the directory on the build host.
        bool m_IsUpToDate;              ///< true if we think the library doesn't need to be built.

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

        std::string BuildOutputPath() const { return m_BuildOutputDir + "/lib"
                                                     + m_ShortName + ".so";
                                            }

        void MarkUpToDate() { m_IsUpToDate = true; }
        void MarkOutOfDate() { m_IsUpToDate = false; }
        bool IsUpToDate() const { return m_IsUpToDate; }
        bool IsOutOfDate() const { return !m_IsUpToDate; }
};


}


#endif  // LIBRARY_H_INCLUDE_GUARD
