//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the routines for building Component Instances.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "ComponentInstanceBuilder.h"
#include "InterfaceBuilder.h"
#include "Utilities.h"


//--------------------------------------------------------------------------------------------------
/**
 * Build IPC API interface instance libraries required by a given component instance.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentInstanceBuilder_t::BuildInterfaces
(
    legato::ComponentInstance& instance
)
//--------------------------------------------------------------------------------------------------
{
    // Create an Interface Builder object.
    InterfaceBuilder_t interfaceBuilder(m_Params);

    if (m_Params.IsVerbose())
    {
        if (instance.ProvidedApis().empty() && instance.RequiredApis().empty())
        {
            std::cout << "Component instance '" << instance.Name()
                      << "' doesn't have any IPC API interfaces." << std::endl;
        }
        else
        {
            std::cout << "Building interfaces for component instance '" << instance.Name() << "'."
                      << std::endl;
        }
    }

    // Build the IPC API libs and add them to the list of libraries that need
    // to be bundled in the application.
    for (auto& mapEntry : instance.ProvidedApis())
    {
        auto& interface = mapEntry.second;

        // We want the generated code and other intermediate output files to go into a separate
        // interface-specific directory to avoid confusion.
        interfaceBuilder.Build(interface, legato::CombinePath(m_Params.ObjOutputDir(),
                                                              interface.AppUniqueName()));
    }

    for (auto& mapEntry : instance.RequiredApis())
    {
        auto& interface = mapEntry.second;

        if (!interface.TypesOnly()) // If only using types, don't need a library.
        {
            // We want the generated code and other intermediate output files to go into a separate
            // interface-specific directory to avoid confusion.
            interfaceBuilder.Build(interface, legato::CombinePath(m_Params.ObjOutputDir(),
                                                                  interface.AppUniqueName()));
        }
        else if (m_Params.IsVerbose())
        {
            std::cout << "Nothing needs to be done for [types-only] interface '"
                      << interface.InternalName() << "'." << std::endl;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Builds a component instance library.
 *
 * @todo    Split out the parts that are common to all instances of the same component and put
 *          those parts in a common component library to save space on-target.
 */
//--------------------------------------------------------------------------------------------------
void ComponentInstanceBuilder_t::Build
(
    legato::ComponentInstance& instance
)
//--------------------------------------------------------------------------------------------------
{
    // Recursively (depth-first) build sub-instances.
    for (auto subInstancePtr : instance.SubInstances())
    {
        Build(*subInstancePtr);
    }

    // Build the IPC interface libraries needed by this component instance.
    BuildInterfaces(instance);
}
