//--------------------------------------------------------------------------------------------------
/**
 * @file jsonGenerator.h
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_JSON_GENERATOR_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_JSON_GENERATOR_H_INCLUDE_GUARD

namespace json
{


void GenerateModel
(
    std::ostream& out,
    model::Component_t* component,
    const mk::BuildParams_t& buildParams
);


void GenerateModel
(
    std::ostream& out,
    model::App_t* application,
    const mk::BuildParams_t& buildParams
);


void GenerateModel
(
    std::ostream& out,
    model::System_t* system,
    const mk::BuildParams_t& buildParams
);


void GenerateModel
(
    std::ostream& out,
    model::Module_t* module,
    const mk::BuildParams_t& buildParams
);


} // namespace json


#endif // LEGATO_MKTOOLS_JSON_GENERATOR_H_INCLUDE_GUARD
