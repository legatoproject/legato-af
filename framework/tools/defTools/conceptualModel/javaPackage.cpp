//--------------------------------------------------------------------------------------------------
/**
 * @file javaPackage.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"
#include <algorithm>


namespace model
{




//--------------------------------------------------------------------------------------------------
/**
 * Initialize a new java package.  This constructor will search the filesystem for all of the Java
 * source files that make up the package and will generate an appropriate source and class file
 * list.
 */
//--------------------------------------------------------------------------------------------------
JavaPackage_t::JavaPackage_t
(
    const std::string& name,
    const std::string& basePath
)
//--------------------------------------------------------------------------------------------------
:   packageName(name)
//--------------------------------------------------------------------------------------------------
{
    packagePath = packageName;
    std::replace(packagePath.begin(), packagePath.end(), '.', '/');
    packagePath = path::Combine("src", packagePath);

    for (auto& file : file::ListFiles(path::Combine(basePath, packagePath)))
    {
        if (path::HasSuffix(file, ".java"))
        {
            auto newPath = path::Combine(packagePath, file);

            sourceFiles.push_back(newPath);
            classFiles.push_back(path::RemoveSuffix(newPath, ".java") + ".class");
        }
    }
}



}
