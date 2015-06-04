//--------------------------------------------------------------------------------------------------
/**
 * @file mkCommon.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "mkCommon.h"
#include <string.h>
#include <unistd.h>

namespace cli
{


//--------------------------------------------------------------------------------------------------
/**
 * Run the Ninja build tool.  Executes the build.ninja script in the root of the working directory
 * tree, if it exists.
 *
 * If the build.ninja file exists, this function will never return.  If the build.ninja file doesn't
 * exist, then this function WILL return.
 */
//--------------------------------------------------------------------------------------------------
void RunNinja
(
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto ninjaFilePath = path::Combine(buildParams.workingDir, "build.ninja");

    if (file::FileExists(ninjaFilePath))
    {
        if (buildParams.beVerbose)
        {
            std::cout << "Executing ninja build system..." << std::endl;
            std::cout << "$ ninja -v -d explain -f " << ninjaFilePath << std::endl;

            (void)execlp("ninja",
                         "ninja",
                         "-v",
                         "-d", "explain",
                         "-f", ninjaFilePath.c_str(),
                         (char*)NULL);

            // REMINDER: If you change the list of arguments passed to ninja, don't forget to
            //           update the std::cout message above that to match your changes.
        }
        else
        {
            (void)execlp("ninja",
                         "ninja",
                         "-f", ninjaFilePath.c_str(),
                         (char*)NULL);
        }

        int errCode = errno;

        throw mk::Exception_t("Failed to execute ninja ("
                                   + std::string(strerror(errCode)) + ").");
    }
}


} // namespace cli
