//--------------------------------------------------------------------------------------------------
/**
 * @file apiParser.cpp  Implementation of the .api file parser.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


namespace parser
{

namespace api
{


//--------------------------------------------------------------------------------------------------
/**
 * Parse a USETYPES statement out of a file.
 *
 * @return the .api file path, or "" if the file stream is not positioned at the start of a
 *         USETYPES statement.
 **/
//--------------------------------------------------------------------------------------------------
std::string ParseUseTypesStatement
(
    std::ifstream& inputStream
)
//--------------------------------------------------------------------------------------------------
{
    const char matchString[] = "SETYPES";   // NOTE: 'U' has already been pulled from the stream.

    int c;

    // Match the "USETYPES"
    for (const char* matchPtr = matchString; *matchPtr != '\0'; matchPtr++)
    {
        c = inputStream.get();

        if (c != *matchPtr)
        {
            return "";
        }
    }

    // Skip whitespace
    for (;;)
    {
        c = inputStream.get();

        if (!isspace(c))
        {
            break;
        }
    }

    // Now match everything up to a space or semicolon as a .api file name.
    std::string result;
    while ((c != EOF) && (c != ';') && (!isspace(c)))
    {
        result += c;

        c = inputStream.get();
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a list of other .api files that a given .api file depends on.
 *
 * @throw mk::Exception_t if an error is encountered.
 */
//--------------------------------------------------------------------------------------------------
void GetDependencies
(
    const std::string& filePath,    ///< Path to .api file to be parsed.
    std::function<void (std::string&&)> handlerFunc ///< Function to call with dependencies.
)
//--------------------------------------------------------------------------------------------------
{
    // Make sure the file exists.
    if (!file::FileExists(filePath))
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("File not found: '%s'."), filePath)
        );
    }

    std::ifstream inputStream(filePath);

    // Make sure we were able to open the file.
    if (!inputStream.is_open())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for reading."), filePath)
        );
    }

    // Keep looking for USETYPES statements, skipping comments.
    for (int c = inputStream.get(); c != EOF; c = inputStream.get())
    {
        // Could this be the start of a USETYPES?
        if (c == 'U')
        {
            std::string dependency = ParseUseTypesStatement(inputStream);
            if (!dependency.empty())
            {
                handlerFunc(std::move(dependency));
            }
        }
        // Skip comments.
        else if (c == '/')
        {
            // Could be the start of a comment.
            c = inputStream.get();
            if (c == '/')
            {
                // Start of C++ style comment.  Skip to end of line or end of file.
                do
                {
                    c = inputStream.get();
                }
                while ((c != '\n') && (c != EOF));
            }
            else if (c == '*')
            {
                // Start of C style comment.  Skip to "*/".
                do
                {
                    c = inputStream.get();
                    if (c == '*')
                    {
                        c = inputStream.get();
                        if (c == '/')
                        {
                            break;
                        }
                    }
                }
                while (c != EOF);
            }
        }
    }

    if (inputStream.bad())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to read from file '%s'."), filePath)
        );
    }
}



} // namespace api

} // namespace parser

