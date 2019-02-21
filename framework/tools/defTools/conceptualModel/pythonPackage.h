//--------------------------------------------------------------------------------------------------
/**
 * @file pythonPackage.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_PYTHON_PACKAGE_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_PYTHON_PACKAGE_H_INCLUDE_GUARD


struct PythonPackage_t
{
    std::string packageName;  ///< Name of the Python package.
    std::string packagePath;

    std::list<std::string> sourceFiles;

    PythonPackage_t(const std::string& name, const std::string& basePath)
        : packageName(name), packagePath(path::Combine(basePath,name)) {}
};


#endif // LEGATO_DEFTOOLS_MODEL_PYTHON_PACKAGE_H_INCLUDE_GUARD
