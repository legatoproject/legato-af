//--------------------------------------------------------------------------------------------------
/**
 *  Implement do and undo action functionality for all supported edit actions.
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------
#include "mkTools.h"
#include "commandLineInterpreter.h"

namespace cli
{

#define TEMP_EXT ".temp"

//--------------------------------------------------------------------------------------------------
/**
 * Function to check if a file's containing directory exists or not.
 **/
//--------------------------------------------------------------------------------------------------
bool ContainingDirectoryExists
(
    std::string filePath
)
{
    if (file::DirectoryExists(path::GetContainingDir(filePath)))
    {
       return true;
    }
    else
    {
        return false;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Do action to check if a directory exists.
 **/
//--------------------------------------------------------------------------------------------------
void CheckDirExistAction_t::DoAction()
{
    if (handler.isPrintLogging())
    {
        if (dirMustExist)
        {
            std::cout << mk::format(LE_I18N("\nChecking if directory '%s' exists"), dirPath);
        }
        else
        {
            std::cout << mk::format(LE_I18N("\nChecking if directory '%s' does not exist"), dirPath);
        }
    }

    if (dirMustExist)
    {
       dirPath = path::MakeCanonical(dirPath);
        if (!file::DirectoryExists(dirPath))
        {
             throw mk::Exception_t(
                   mk::format(LE_I18N("Component directory '%s' does not exist."), dirPath)
            );
        }
    }
    else
    {
        if (file::DirectoryExists(dirPath))
        {
             throw mk::Exception_t(
                   mk::format(LE_I18N("Component directory '%s' already exist."), dirPath)
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Do action to check if a definition file exists.
 **/
//--------------------------------------------------------------------------------------------------
void CheckDefFileExistAction_t::DoAction()
{
    if (handler.isPrintLogging())
    {
        if (fileMustExist)
        {
            std::cout << mk::format(LE_I18N("\nChecking if definition file '%s' exists."),
                                    filePath
                         );
        }
        else
        {
            std::cout << mk::format(LE_I18N("\nChecking if definition file '%s' does not exist."),
                                    filePath
                         );

        }
    }

    if (fileMustExist)
    {
        if (!file::FileExists(filePath))
        {
             throw mk::Exception_t(
                   mk::format(LE_I18N("Definition file '%s' does not exist."), filePath)
            );
        }
    }
    else
    {
        if (file::FileExists(filePath))
        {
             throw mk::Exception_t(
                   mk::format(LE_I18N("Definition file '%s' already exist."), filePath)
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Do action to create a temporary working ADEF file and update that file's required section
 * with line to write.
 **/
//--------------------------------------------------------------------------------------------------
void CreateUpdateTempAdefAction_t::DoAction()
{
    // List of do actions to update the temporary working application definition file.
    handler.tempWorkDefFilePath = handler.absAdefFilePath + TEMP_EXT;
    updateDefs::EvaluateAdefGetEditLinePosition(handler, handler.systemPtr);
    updateDefs::UpdateDefinitionFile(handler, handler.absAdefFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Undo action to delete the previously created temporary working ADEF file if it exists.
 **/
//--------------------------------------------------------------------------------------------------
void CreateUpdateTempAdefAction_t::UndoAction()
{
    if (handler.buildParams.beVerbose)
    {
        std::cout << mk::format(
                        LE_I18N("\nDeleting temporary definition file '%s'."),
                        handler.tempWorkDefFilePath
                     );
    }

    file::DeleteFile(handler.tempWorkDefFilePath);
}


//-------------------------------------------------------------------------------------------------
/**
 * Do action to create a temporary working CDEF file and update that file's required section
 * with line to write.
 **/
//-------------------------------------------------------------------------------------------------
void CreateUpdateTempCdefAction_t::DoAction()
{
    // List of do actions to update the temporary working component definition file.
    handler.tempWorkDefFilePath = cdefWorkingFilePath + TEMP_EXT;
    updateDefs::EvaluateCdefGetEditLinePosition(handler, cdefWorkingFilePath);
    updateDefs::UpdateDefinitionFile(handler, cdefWorkingFilePath);
}


//-------------------------------------------------------------------------------------------------
/**
 * Undo action to delete the previously created temporary working CDEF file if it exists.
 **/
//-------------------------------------------------------------------------------------------------
void CreateUpdateTempCdefAction_t::UndoAction()
{
    if (handler.buildParams.beVerbose)
    {
        std::cout << mk::format(
                        LE_I18N("\nDeleting temporary definition file '%s'."),
                        handler.tempWorkDefFilePath
                     );
    }

    file::DeleteFile(handler.tempWorkDefFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Do action to create a temporary working SDEF file and update that file's required section
 * with line to write.
 **/
//--------------------------------------------------------------------------------------------------
void CreateUpdateTempSdefAction_t::DoAction()
{
    // List of do actions to update the temporary working system definition file.
    handler.tempWorkDefFilePath = handler.absSdefFilePath + TEMP_EXT;
    updateDefs::ParseSdefUpdateItem(handler);
    updateDefs::UpdateDefinitionFile(handler, handler.absSdefFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
  * Undo action to delete the previously created temporary working SDEF file if it exists.
 **/
//--------------------------------------------------------------------------------------------------
void CreateUpdateTempSdefAction_t::UndoAction()
{
    if (handler.buildParams.beVerbose)
    {
        std::cout << mk::format(
                        LE_I18N("\nDeleting temporary SDEF file '%s'."),
                        handler.tempWorkDefFilePath
                     );
    }

    file::DeleteFile(handler.tempWorkDefFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Do action to generate component template directory with CDEF and source file.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateComponentTemplateAction_t::DoAction()
{
    defs::GenerateComponentTemplate(handler);
}


//--------------------------------------------------------------------------------------------------
/**
 * Undo action to delete the component directory and containing files.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateComponentTemplateAction_t::UndoAction()
{
    if (handler.buildParams.beVerbose)
    {
        std::cout << mk::format(
                        LE_I18N("\nDeleting component directory '%s'."), handler.absCdefFilePath);
    }

    file::DeleteDir(handler.absCdefFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Do action to generate the template definition file based on edit item type.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateDefTemplateAction_t::DoAction()
{
    switch (handler.editItemType)
    {
        case ArgHandler_t::EditItemType_t::APP:
            isDirExist = ContainingDirectoryExists(handler.absAdefFilePath);
            defs::GenerateApplicationTemplate(handler);
            break;

        case ArgHandler_t::EditItemType_t::MODULE:
            isDirExist = ContainingDirectoryExists(handler.absMdefFilePath);
            defs::GenerateModuleTemplate(handler);
            break;

        case ArgHandler_t::EditItemType_t::SYSTEM:
            isDirExist = ContainingDirectoryExists(handler.absSdefFilePath);
            defs::GenerateSystemTemplate(handler);
            break;

        default:
            throw mk::Exception_t(LE_I18N("Internal error: Invalid edit item type."));
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Undo action to delete the previously generated definition template file.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateDefTemplateAction_t::UndoAction()
{
    std::string fileName = handler.GetFileForEditItemType();

    // In case of error exceptions user's workspace must be kept clean.
    // If the file's containing directory did not initially exist, delete the directory. Otherwise
    // delete only the file.
    if (!isDirExist)
    {
        if (handler.buildParams.beVerbose)
        {
            std::cout << mk::format(LE_I18N("\nDeleting directory '%s'."), fileName);
        }

        file::DeleteDir(path::GetContainingDir(fileName));
    }
    else
    {
        if (handler.buildParams.beVerbose)
        {
            std::cout << mk::format(LE_I18N("\nDeleting definition file '%s'."), fileName);
        }

        file::DeleteFile(fileName);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Do action to remove the component directory.
 **/
//--------------------------------------------------------------------------------------------------
void RemoveDirAction_t::DoAction()
{
    if (handler.isPrintLogging())
    {
        std::cout << mk::format(LE_I18N("\nDeleting component directory '%s'."),
                                handler.absCdefFilePath
                     );
    }

    if (handler.buildParams.isDryRun)
    {
        return;
    }

    file::DeleteDir(handler.absCdefFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Undo action of removing directory which is to throw an error exception.
 **/
//--------------------------------------------------------------------------------------------------
void RemoveDirAction_t::UndoAction()
{
    throw mk::Exception_t(
           mk::format(
               LE_I18N("Internal error: Attempt to undo non-reversible action of removed "
                       "directory '%s'."),
                       handler.absCdefFilePath)
    );
}


//--------------------------------------------------------------------------------------------------
/**
 * Do action to remove a definition file.
 **/
//--------------------------------------------------------------------------------------------------
void RemoveFileAction_t::DoAction()
{
    std::string fileName = handler.GetFileForEditItemType();

    if (handler.isPrintLogging())
    {
        std::cout << mk::format(LE_I18N("\nDeleting file '%s'."), fileName);
    }

    if (handler.buildParams.isDryRun)
    {
        return;
    }

    file::RemoveFile(fileName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Undo action of removing a definition file which is to throw an error exception.
 **/
//--------------------------------------------------------------------------------------------------
void RemoveFileAction_t::UndoAction()
{
    std::string fileName = handler.GetFileForEditItemType();

    throw mk::Exception_t(
           mk::format(
             LE_I18N("Internal error: Attempt to undo non-reversible action of removed file '%s'."),
                        fileName)
    );
}


//--------------------------------------------------------------------------------------------------
/**
 * Do action to rename an old defintion file to new file name.
 **/
//--------------------------------------------------------------------------------------------------
void RenameFileAction_t::DoAction()
{
    std::string oldFileName = handler.GetOldFileForEditItemType();
    std::string newFileName = handler.GetFileForEditItemType();

    if (handler.isPrintLogging())
    {
        std::cout << mk::format(LE_I18N("\nRenaming definition file '%s' to '%s'."),
                                oldFileName, newFileName
                     );
    }

    if (handler.buildParams.isDryRun)
    {
        return;
    }

    file::RenameFile(oldFileName, newFileName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Undo action to revert the definition file name to it's old original file name.
 **/
//--------------------------------------------------------------------------------------------------
void RenameFileAction_t::UndoAction()
{
    std::string fileName = handler.GetFileForEditItemType();
    std::string oldFileName = handler.GetOldFileForEditItemType();

    if (handler.buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("\nRenaming file '%s' to '%s'."), fileName, oldFileName);
    }

    file::RenameFile(fileName, oldFileName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Do action to rename the temporary working definition file to the active definition file.
 **/
//--------------------------------------------------------------------------------------------------
void RenameTempWorkToActiveFileAction_t::DoAction()
{
    if (handler.isPrintLogging())
    {
        std::cout << mk::format(LE_I18N("\nRenaming file '%s' to '%s'."),
                                handler.tempWorkDefFilePath, activeDefFilePath
                     );
    }

    if (handler.buildParams.isDryRun)
    {
        return;
    }

    file::RenameFile(handler.tempWorkDefFilePath, activeDefFilePath);
}


}
