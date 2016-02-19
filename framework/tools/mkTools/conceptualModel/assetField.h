//--------------------------------------------------------------------------------------------------
/**
 * @file assetField.h
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_ASSET_FIELD_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_ASSET_FIELD_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Stores the data for a single asset data field.
 */
//--------------------------------------------------------------------------------------------------
struct AssetField_t
{
    typedef enum
    {
        TYPE_SETTING,
        TYPE_VARIABLE,
        TYPE_COMMAND,
        TYPE_UNSET
    }
    ActionType_t;

    /// Constructor
    AssetField_t(ActionType_t newActionType,
                 const std::string& newDataType,
                 const std::string& newName,
                 const std::string& newDefaultValue = "")
    :   actionType(newActionType),
        dataType(newDataType),
        name(newName),
        defaultValue(newDefaultValue) {}

    /// Getters
    ActionType_t GetActionType() const { return actionType; }
    const std::string& GetDataType() const { return dataType; }
    const std::string& GetName() const { return name; }
    const std::string& GetDefaultValue() const { return defaultValue; }

    /// Setters
    void SetActionType(ActionType_t newActionType) { actionType = newActionType; }
    void SetDataType(const std::string& newDataType) { dataType = newDataType; }
    void SetName(const std::string& newName) { name = newName; }
    void SetDefaultValue(const std::string& newDefaultValue) { defaultValue = newDefaultValue; }

private:
    ActionType_t actionType;   ///< Indicates the type of activity allowed on the field.
    std::string dataType;      ///< The type of data the field can take, (int, string, etc...)
    std::string name;          ///< Name of the field.
    std::string defaultValue;  ///< Optional default value for this field.
};


#endif // LEGATO_MKTOOLS_MODEL_ASSET_FIELD_H_INCLUDE_GUARD
