//--------------------------------------------------------------------------------------------------
/**
 * @file conceptualModel.h
 *
 * Header that includes all the header files in the Conceptual Model.
 *
 * @page defTools_ConceptualModel Conceptual Model
 *
 * The conceptual object model looks like this: (note: '*' = "multiple")
 *
@verbatim

System --+--> Bindings --*--> Binding
         |
         +--> Commands --*--> Command
         |
         +--> Apps --*--> App
                           |
    +----------------------+
    |
    +--> Required Files
    |
    +--> Required Dirs
    |
    +--> Bundled Files
    |
    +--> Bundled Dirs
    |
    +--> Extern Client Interfaces --*--> Extern Client --------+------------------------+
    |                                    API Interface <-+     |                        |
    |                                                    |     |                        |
    +--> Extern Server Interfaces --*--> Extern Server -----+--|-------+                |
    |                                    API Interface   |  |  |       |                |
    |                                     ^              |  |  |       |                |
    |                                     |              |  |  |       |                |
    +--> Pre-Built Server Interfaces --*--+              |  |  |       |                |
    |                                                    |  |  |       |                |
    +--> Pre-Built Client Interfaces --*-----------------+  |  |       |                |
    |                                                       |  |       |                |
    +--> Components --*--> Component --+--> Sources         |  |       |                |
    |                          ^       |                    |  |       |                |
    |                          |       +--> CFLAGS          |  |       |                |
    |                          |       |                    |  |       |                |
    |                          |       +--> CXXFLAGS        |  |       |                |
    |                          |       |                    |  |       |                |
    |                          |       +--> LDFLAGS         |  |       |                |
    |                          |       |                    |  |       v                v
    |                          |       +--> Required APIs   |  |   API Server      API Client
    |                          |       |                    |  |   I/F Instance    I/F Instance
    |                          |       +--> Provided APIs   |  |     ^   ^              |  ^
    |                          |       |                    |  |     |   |              |  |
    |                          |       ...                  |  |     |   +-- Binding <--+  |
    |                          |                            |  |     |                     |
    |                          +-------------------------+  |  |     |                     |
    |                                                    |  |  |     |                     |
    +--> Executables --*--> Executable                   |  |  |     |                     |
    |    List                  ^  |                      |  |  |     |                     |
    |                          |  |                      |  v  v     |                     |
    |                          |  +--> Component --*--> Component    +-----------+         |
    |                          |       Instances        Instance                 |         |
    |                          |                           |                     |         |
    |                          |                           +--> Server-Side --*--+         |
    |                          |                           |     APIs                      |
    |                          |                           |                               |
    |                          |                           +--> Client-Side --*------------+
    |                          |                                 APIs
    |                          |
    |                          +----------------------------+
    |                                                       |
    |                                                       |
    +--> Process  ----*--> Process  ----> Process --*--> Process --> Args List
    |    Environments      Environment     List
    |
    ...

@endverbatim
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_CONCEPTUAL_MODEL_H_INCLUDE_GUARD
#define LEGATO_CONCEPTUAL_MODEL_H_INCLUDE_GUARD

namespace model
{

#include "targetInfo.h"
#include "programmingLanguage.h"
#include "objectFile.h"
#include "javaPackage.h"
#include "pythonPackage.h"
#include "appLimit.h"
#include "api.h"
#include "binding.h"
#include "user.h"
#include "permissions.h"
#include "fileSystemObject.h"
#include "module.h"
#include "component.h"
#include "exe.h"
#include "process.h"
#include "processEnvironment.h"
#include "app.h"
#include "command.h"
#include "system.h"

} // namespace model

#endif // LEGATO_CONCEPTUAL_MODEL_H_INCLUDE_GUARD

