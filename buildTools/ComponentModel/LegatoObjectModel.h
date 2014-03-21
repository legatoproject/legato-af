//--------------------------------------------------------------------------------------------------
/**
 *  @file LegatoObjectModel.h
 *
 * Core header of the Component Model.
 *
 * Client code need only include this file to get all object model definitions.
 *
 * The object model looks like this: (note: '*' = "multiple")
 *
 * App --+--> Component List --*--> Component --+--> Sources List
 *       |                               ^      |
 *       |                               |      +--> Library List
 *       |                               |      |
 *       |                               |      +--> Imported Interfaces Map --*--+
 *       |                               |      |                                 +--> Interface
 *       |                               |      +--> Exported Interfaces Map --*--+
 *       |                               |      |
 *       |                               |      +--> Included Files List --*--+
 *       |                               |      |                             +--> File Mapping <--+
 *       |                               |      +--> File Imports List --*----+                    |
 *       |                               |      |                                                  |
 *       |                               |      +--> Pools Map (name, pool size)                   |
 *       |                               |      |                                                  |
 *       |                               |      +--> Config Item Map (path, value)                 |
 *       |                               |                                                         |
 *       |                               +------------------------------------------+              |
 *       |                                                                          |              |
 *       +--> Executables List --*--> Executable                                    |              |
 *       |                                |                                         |              |
 *       |                                +--> Component Instance List --*--> Component Instance   |
 *       |                                |                                                        |
 *       |                                +--> Bind List --*--> Bind                               |
 *       |                                |                                                        |
 *       |                                +--> Imported Interfaces Map --*--+                      |
 *       |                                |                                 +--> Interface         |
 *       |                                +--> Exported Interfaces Map --*--+                      |
 *       |                                                                                         |
 *       +--> Process Environment List --*--> Process Environment --> Process List --*--> Process  |
 *       |                                                                                         |
 *       +--> File Imports List --*---------+                                                      |
 *       |                                  +------------------------------------------------------+
 *       +--> Included Files List --*-------+
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_OBJECT_MODEL_INCLUDE_GUARD
#define LEGATO_OBJECT_MODEL_INCLUDE_GUARD


#include <stdexcept>
#include <string>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Exception.h"   // Basic string exception type.
#include "Permissions.h"
#include "ConfigItem.h"
#include "MemoryPool.h"
#include "FilePath.h"    // Helper functions related to file paths.
#include "FileMapping.h"
#include "Interface.h"
#include "Component.h"
#include "ComponentInstance.h"
//#include "Bind.h"
#include "Executable.h"
#include "Process.h"
#include "ProcessEnvironment.h"
#include "App.h"
#include "CLanguage.h"  // Helper functions related to the C programming language.
#include "BuildParams.h"    // Build parameters collected from the command line.


#endif  // LEGATO_OBJECT_MODEL_INCLUDE_GUARD
