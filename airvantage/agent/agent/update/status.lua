local M = { }

local num2string = {
    [200]  = "OK",
-- Package checking errors
    [450] = "INVALID_UDPATE_FILE_NAME",
    [451] = "UDPATE_FILE_NAME_PARSING_ERROR",
    [452] = "EXTRACTION_FOLDER_CREATION_FAILED",
    [453] = "PACKAGE_EXTRACTION_FAILED",
    [454] = "MANIFEST_OPENING_FAILED",
    [455] = "MANIFEST_READING_FAILED",
    [456] = "MANIFEST_LOADING_FAILED",
    [457] = "MANIFEST_INVALID_LUA_FILE",
    [458] = "MANIFEST_INVALID_CONTENT_TYPE",
    [459] = "DEPENDENCY_ERROR",
    [479] = "MANIFEST_DEFAULT_ERROR",
-- Update process errors
    [461] = "TOO_MUCH_COMPONENT_RETRIES",
    [462] = "ASSET_UPDATE_EXECUTION_FAILED",
    [463] = "SENDING_UPDATE_TO_ASSET_FAILED",
    [464] = "INVALID_STATE_DURING_DISPATCH",
    [465] = "DEFAULT_RESULT_CODE",
    [600] = "ABORTED_BY_USER_REQUEST",
-- Agent Internal updaters errors
    [473] = "INVALID_UPDATE_PATH",
    [474] = "MISSING_INSTALL_SCRIPT",
    [475] = "CANNOT_LOAD_INSTALL_SCRIPT",
    [476] = "CANNOT_RUN_INSTALL_SCRIPT",
    [477] = "APPLICATION_CONTAINER_NOT_RUNNING",
    [478] = "APPLICATION_CONTAINER_ERROR",
-- Internal errors
    [400] = "DEFAULT_INTERNAL_ERROR",
    [501] = "GET_FREE_SPACE_FAILED",
    [505] = "MALFORMED_UPDATE_DATA",
    [506] = "OTHER_UPDATE_IN_PROGRESS",
-- Download errors
    [551] = "INVALID_UPDATE_COMMAND_PARAMS",
    [552] = "DOWNLOAD_RETRIES_EXHAUSTED",
    [553] = "PACKAGE_SIGNATURE_MISMATCH",
    [554] = "INVALID_DOWNLOAD_INFO",
    [555] = "NOT_ENOUGH_FREE_SPACE",
    [557] = "INVALID_DOWNLOAD_PROTOCOL",
    [558] = "UPDATE_REJECTED"
}

local string2num = { }

for k, v in pairs(num2string) do string2num[v]=k end

function M.tonumber(str)
    checks('string')
    return assert(string2num[str], "Unknown Agent Update status name")
end

function M.tostring(num)
    checks('number')
    return assert(num2string[num], "Unknown Agent Update status number")
end

return M