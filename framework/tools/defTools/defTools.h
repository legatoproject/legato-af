//--------------------------------------------------------------------------------------------------
/**
 * @file defTools.h
 *
 * Overall header for libdefTools.so.  This library provides functions for parsing and generating
 * a model of Legato def files.
 *
 * This library is used as the front-end for mkTools, but can also be used by other tools that need
 * to read .def files.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_H_INCLUDE_GUARD

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

#include "buildParams.h"
#include "envVars.h"
#include "exception.h"
#include "format.h"
#include "path.h"
#include "file.h"
#include "md5.h"
#include "parseTree/parseTree.h"
#include "parser/parser.h"
#include "conceptualModel/conceptualModel.h"
#include "modeller/modeller.h"

#endif  // LEGATO_DEFTOOLS_H_INCLUDE_GUARD
