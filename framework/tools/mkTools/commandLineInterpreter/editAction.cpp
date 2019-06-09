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
 * Do action to check if a directory exists.
 **/
//--------------------------------------------------------------------------------------------------
void CheckDirExistAction_t::DoAction()
{
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
    updateDefs::UpdateDefinitionFile(handler.absAdefFilePath, handler.tempWorkDefFilePath,
                                     handler.linePositionToWrite);
}


//--------------------------------------------------------------------------------------------------
/**
 * Undo action to delete the previously created temporary working ADEF file if it exists.
 **/
//--------------------------------------------------------------------------------------------------
void CreateUpdateTempAdefAction_t::UndoAction()
{
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
    updateDefs::UpdateDefinitionFile(handler.absSdefFilePath, handler.tempWorkDefFilePath,
                                     handler.linePositionToWrite);
}


//--------------------------------------------------------------------------------------------------
/**
  * Undo action to delete the previously created temporary working SDEF file if it exists.
 **/
//--------------------------------------------------------------------------------------------------
void CreateUpdateTempSdefAction_t::UndoAction()
{
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
            defs::GenerateApplicationTemplate(handler);
            break;

        case ArgHandler_t::EditItemType_t::MODULE:
            defs::GenerateModuleTemplate(handler);
            break;

        case ArgHandler_t::EditItemType_t::SYSTEM:
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

    // Delete the template file created in case of error exceptions to keep user's workspace clean.
    file::DeleteFile(fileName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Do action to remove the component directory.
 **/
//--------------------------------------------------------------------------------------------------
void RemoveDirAction_t::DoAction()
{
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
    file::RemoveFile(fileName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Undo action of removing a defintion file which is to throw an error exception.
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

    file::RenameFile(fileName, oldFileName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Do action to rename the temporary working definition file to the active definition file.
 **/
//--------------------------------------------------------------------------------------------------
void RenameTempWorkToActiveFileAction_t::DoAction()
{
    file::RenameFile(handler.tempWorkDefFilePath, activeDefFilePath);
}


}
