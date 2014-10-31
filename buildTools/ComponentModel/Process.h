//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Process class, which is used to hold the details of a single process
 * defined in a "run:" subsection of a "processes:" section in a .adef file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013-2014.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef PROCESS_H_INCLUDE_GUARD
#define PROCESS_H_INCLUDE_GUARD

namespace legato
{


class Process
{
    public:
        Process();
        Process(const Process&) = delete;
        Process(Process&&);
        virtual ~Process() {}

    public:
        Process& operator =(const Process&) = delete;
        Process& operator =(const Process&&) = delete;

    private:
        std::string m_Name;
        std::string m_ExePath;
        Executable* m_ExePtr;   ///< Ptr to Executable object, or NULL if exe not built by mk tools.
        std::list<std::string> m_CommandLineArgs;

    public:
        void Name(const std::string& name);
        void Name(std::string&& name);
        const std::string& Name() const { return m_Name; }

        void ExePath(const std::string& path) { m_ExePath = path; }
        void ExePath(std::string&& path) { m_ExePath = std::move(path); }
        const std::string& ExePath() const { return m_ExePath; }

        void ExePtr(Executable* exePtr) { m_ExePtr = exePtr; }
        const Executable* ExePtr() const { return m_ExePtr; }
        Executable* ExePtr() { return m_ExePtr; }

        void AddCommandLineArg(const std::string& arg) { m_CommandLineArgs.push_back(arg); }
        void AddCommandLineArg(std::string&& arg) { m_CommandLineArgs.push_back(arg); }
        const std::list<std::string>& CommandLineArgs() const { return m_CommandLineArgs; }
};


}

#endif // PROCESS_H_INCLUDE_GUARD
