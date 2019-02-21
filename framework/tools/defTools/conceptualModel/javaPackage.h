//--------------------------------------------------------------------------------------------------
/**
 * @file javaPackage.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_JAVA_PACKAGE_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_JAVA_PACKAGE_H_INCLUDE_GUARD


struct JavaPackage_t
{
    std::string packageName;  ///< Name of the Java package.
    std::string packagePath;

    std::list<std::string> sourceFiles;
    std::list<std::string> classFiles;

    JavaPackage_t(const std::string& name, const std::string& basePath);
};


#endif // LEGATO_DEFTOOLS_MODEL_JAVA_PACKAGE_H_INCLUDE_GUARD
