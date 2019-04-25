//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Process class, which is used to hold the details of a single process
 * defined in a "run:" subsection of a "processes:" section in a .adef file.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef PROCESS_H_INCLUDE_GUARD
#define PROCESS_H_INCLUDE_GUARD


struct Process_t
{
    Process_t(const parseTree::RunProcess_t* p): parseTreePtr(p) {}

    std::string exePath;
    std::list<std::string> commandLineArgs;

    void SetName(const std::string& name);
    const std::string& GetName() const { return name; }

    const parseTree::RunProcess_t* parseTreePtr;

private:

    std::string name;
};


#endif // PROCESS_H_INCLUDE_GUARD
