//--------------------------------------------------------------------------------------------------
/**
 * @file conceptualModel.h
 *
 * Header that includes all the header files in the Conceptual Model.
 *
 * @page mkTools_ConceptualModel Conceptual Model
 *
 * The conceptual object model looks like this: (note: '*' = "multiple")
 *
@verbatim

System --+--> Bindings --*--> Binding
         |
         +--> Commands --*--> Command
         |
         +--> Apps
                |
                +-*--> App
                        |
                        +--> Required Files
                        |
                        +--> Required Dirs
                        |
                        +--> Bundled Files
                        |
                        +--> Bundled Dirs
                        |
                        +--> Imported Interfaces
                        |
                        +--> Exported Interfaces
                        |
                        +--> Components --*--> Component --+--> Sources
                        |                          ^       |
                        |                          |       +--> CFLAGS
                        |                          |       |
                        |                          |       +--> CXXFLAGS
                        |                          |       |
                        |                          |       +--> LDFLAGS
                        |                          |       |
                        |                          |       +--> Required APIs
                        |                          |       |
                        |                          |       +--> Provided APIs
                        |                          |       |
                        |                          |       ...
                        |                          |
                        |                          +---------------------------+
                        |                                                      |
                        +--> Executables --*--> Executable                     |
                        |    List                  ^  |                        |
                        |                          |  |                        |
                        |                          |  +--> Component --*--> Component
                        |                          |       Instances        Instance
                        |                          |                           |
                        |                          |                           +--> Server-Side -+
                        |                          |                           |     APIs        |
                        |                          |                           |                 *
                        |                          |                           +--> Client-Side  |
                        |                          |                                 APIs   |    |
                        |                          |                                        |    |
                        |                          +----------------------------+           *    |
                        |                                                       |           |    |
                        |                                                       |           |    |
                        |                                                       +--> Args   |    |
                        |                                                       |    List   |    |
                        |                                                       |           |    |
                        +--> Process  ----*--> Process  ----> Process --*--> Process        |    |
                        |    Environments      Environment     List                         |    |
                        |                                                                   v    |
                        +--> Users --*--> User --*--> Bindings --*--> Binding <------ Client API |
                        |                                |                            Instance   |
                        ...                              |                                       |
                                                         +----------------------> Server API <---+
@endverbatim                                                                      Instance
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_CONCEPTUAL_MODEL_H_INCLUDE_GUARD
#define LEGATO_CONCEPTUAL_MODEL_H_INCLUDE_GUARD

namespace model
{

#include "programmingLanguage.h"
#include "objectFile.h"
#include "javaPackage.h"
#include "appLimit.h"
#include "api.h"
#include "binding.h"
#include "user.h"
#include "permissions.h"
#include "fileSystemObject.h"
#include "assetField.h"
#include "asset.h"
#include "component.h"
#include "exe.h"
#include "process.h"
#include "processEnvironment.h"
#include "app.h"
#include "module.h"
#include "command.h"
#include "system.h"


} // namespace model

#endif // LEGATO_CONCEPTUAL_MODEL_H_INCLUDE_GUARD

