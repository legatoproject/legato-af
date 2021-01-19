//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef EDITACTION_H_INCLUDE_GUARD
#define EDITACTION_H_INCLUDE_GUARD

#include <iostream>
#include <string.h>
#include "mkedit.h"

class ArgHandler_t;

//--------------------------------------------------------------------------------------------------
/**
 * Abstract EditAction class for Do and Undo action of each edit action.
 */
//--------------------------------------------------------------------------------------------------
class EditAction_t
{
    protected:
        ArgHandler_t& handler;
    public:
        explicit EditAction_t(ArgHandler_t &arghandle) : handler(arghandle) {}
        virtual void DoAction() = 0;  ///< Pure virtual function
        virtual void UndoAction() {}

        virtual ~EditAction_t() {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Class to check if a given directory exists or not.
 */
//--------------------------------------------------------------------------------------------------
class CheckDirExistAction_t : public EditAction_t
{
    private:
        std::string dirPath;
        bool dirMustExist;
    public:
        CheckDirExistAction_t(ArgHandler_t& arghandle, std::string path, bool mustExist) :
            EditAction_t(arghandle), dirPath(path), dirMustExist(mustExist)
        {}

        void DoAction() override;
};


//--------------------------------------------------------------------------------------------------
/**
 * Class to check if a definition file exists or not.
 */
//--------------------------------------------------------------------------------------------------
class CheckDefFileExistAction_t : public EditAction_t
{
    private:
        std::string filePath;
        bool fileMustExist;
    public:
        CheckDefFileExistAction_t(ArgHandler_t& arghandle, std::string path, bool mustExist) :
            EditAction_t(arghandle), filePath (path), fileMustExist (mustExist)
        {}

        void DoAction() override;
};


//--------------------------------------------------------------------------------------------------
/**
 * Class to create a temporary working ADEF file and to update the required sections of the
 * temporary ADEF file. The temporary ADEF file is later replaced with original ADEF file using
 * RenameTempWorkToActiveFileAction_t.
 */
//--------------------------------------------------------------------------------------------------
class CreateUpdateTempAdefAction_t : public EditAction_t
{
    public:
        CreateUpdateTempAdefAction_t(ArgHandler_t& arghandle) : EditAction_t(arghandle) {}
        void DoAction() override;
        void UndoAction() override;
};


//-------------------------------------------------------------------------------------------------
/**
 * Class to create a temporary working CDEF file and to update the required sections of the
 * temporary CDEF file. The temporary CDEF file is later replaced with original CDEF file using
 * RenameTempWorkToActiveFileAction_t.
 */
//-------------------------------------------------------------------------------------------------
class CreateUpdateTempCdefAction_t : public EditAction_t
{
    private:
        std::string cdefWorkingFilePath;

    public:
        CreateUpdateTempCdefAction_t(ArgHandler_t& arghandle, std::string filePath) :
            EditAction_t(arghandle), cdefWorkingFilePath(filePath)
        {}
        void DoAction() override;
        void UndoAction() override;
};


//--------------------------------------------------------------------------------------------------
/**
 * Class to create a temporary working SDEF file and to update the required sections of the
 * temporary SDEF file. The temporary SDEF file is later replaced with original SDEF file using
 * RenameTempWorkToActiveFileAction_t.
 */
//--------------------------------------------------------------------------------------------------
class CreateUpdateTempSdefAction_t : public EditAction_t
{
    public:
        CreateUpdateTempSdefAction_t(ArgHandler_t& arghandle) : EditAction_t(arghandle) {}
        void DoAction() override;
        void UndoAction() override;
};


//--------------------------------------------------------------------------------------------------
/**
 * Class to generate template component directory with CDEF and source file.
 */
//--------------------------------------------------------------------------------------------------
class GenerateComponentTemplateAction_t : public EditAction_t
{
    public:
        GenerateComponentTemplateAction_t(ArgHandler_t& arghandle) : EditAction_t(arghandle) {}
        void DoAction() override;
        void UndoAction() override;
};


//--------------------------------------------------------------------------------------------------
/**
 * Class to generate template definition file (ADEF/MDEF/SDEF).
 */
//--------------------------------------------------------------------------------------------------
class GenerateDefTemplateAction_t : public EditAction_t
{
    private:
        bool isDirExist;
    public:
        GenerateDefTemplateAction_t(ArgHandler_t& arghandle) : EditAction_t(arghandle) {}
        void DoAction() override;
        void UndoAction() override;
};


//--------------------------------------------------------------------------------------------------
/**
 * Class to remove a directory.
 */
//--------------------------------------------------------------------------------------------------
class RemoveDirAction_t : public EditAction_t
{
    public:
        RemoveDirAction_t(ArgHandler_t& arghandle) : EditAction_t(arghandle) {}
        void DoAction() override;
        void UndoAction() override;
};


//--------------------------------------------------------------------------------------------------
/**
 * Class to remove a directory.
 */
//--------------------------------------------------------------------------------------------------
class RemoveFileAction_t : public EditAction_t
{
    public:
        RemoveFileAction_t(ArgHandler_t& arghandle) : EditAction_t(arghandle) {}
        void DoAction() override;
        void UndoAction() override;
};


//--------------------------------------------------------------------------------------------------
/**
 * Class to rename a file.
 */
//--------------------------------------------------------------------------------------------------
class RenameFileAction_t : public EditAction_t
{
    public:
        RenameFileAction_t(ArgHandler_t& arghandle) : EditAction_t(arghandle) {}
        void DoAction() override;
        void UndoAction() override;
};


//--------------------------------------------------------------------------------------------------
/**
 * Class to rename the temporary working definition file to original definition file.
 */
//--------------------------------------------------------------------------------------------------
class RenameTempWorkToActiveFileAction_t : public EditAction_t
{
    private:
        std::string activeDefFilePath;

    public:
        RenameTempWorkToActiveFileAction_t(ArgHandler_t& arghandle, std::string filePath) :
            EditAction_t(arghandle), activeDefFilePath(filePath)
        {}
        void DoAction() override;
};


#endif // EDITACTION_H_INCLUDE_GUARD
