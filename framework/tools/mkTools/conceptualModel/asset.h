//--------------------------------------------------------------------------------------------------
/**
 * @file asset.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_ASSET_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_ASSET_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Definition of an AirVantage asset and its data fields.
 */
//--------------------------------------------------------------------------------------------------
struct Asset_t
{
    typedef std::list<AssetField_t*> AssetFieldList_t;

    /// Constructor
    Asset_t() {}
    Asset_t(const std::string& newName, const std::initializer_list<AssetField_t*>& List)
    :   name(newName),
        fields(List)
    {
    }

    /// Getters
    const std::string& GetName() const { return name; }

    /// Setters
    void SetName(const std::string& newName) { name = newName; }

    AssetFieldList_t fields;  ///< Collection of fields that store the data.

private:
    std::string name;    ///< The name of the asset.
};


#endif // LEGATO_MKTOOLS_MODEL_ASSET_H_INCLUDE_GUARD
