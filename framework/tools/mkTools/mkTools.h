//--------------------------------------------------------------------------------------------------
/**
 * @file mkTools.h
 *
 * Header that includes all the other common header files used by multiple parts of the mk tools.
 *
 * This is done partly as a convenience for the programmer (so they don't have to include as many
 * files) and partly as a performance improvement measure (because this file can be pre-compiled).
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_H_INCLUDE_GUARD


#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_set>
#include <unordered_map>
#include <vector>

// Allow xgettext to pull all error messages.  Also prepares for localization
#define LE_I18N(x) (x)

#include "exception.h"
#include "buildParams.h"
#include "envVars.h"
#include "path.h"
#include "file.h"
#include "format.h"
#include "md5.h"
#include "parseTree/parseTree.h"
#include "parser/parser.h"
#include "conceptualModel/conceptualModel.h"
#include "modeller/modeller.h"
#include "buildScriptGenerator/buildScriptGenerator.h"
#include "codeGenerator/codeGenerator.h"
#include "configGenerator/configGenerator.h"
#include "adefGenerator/exportedAdefGenerator.h"
#include "generators.h"
#include "targetInfo/linux.h"

#endif  // LEGATO_MKTOOLS_H_INCLUDE_GUARD
