//--------------------------------------------------------------------------------------------------
/**
 * @page c_json JSON Parsing API
 *
 * @warning This API is experimental, and is therefore likely to change.
 *
 * @subpage le_json.h "API Reference" <br>
 *
 * <hr>
 *
 * The JSON Parsing API is intended to provide fast parsing of a JSON data stream with very
 * little memory required.  It is an event-driven API that uses callbacks (handlers) to report
 * when things are found in the JSON document.  The parser does not build a document structure
 * for you.  You build your structure as needed in response to callbacks from the parser.
 * In this way, the JSON parser avoids potential memory fragmentation issues that can arise
 * when document object models are constructed on the heap (e.g., using malloc()).
 *
 *  @section c_json_start Starting and Stopping Parsing
 *
 * The function le_json_Parse() is used to start parsing a JSON document obtained from a
 * file descriptor.  This function returns immediately, and further parsing proceeds in an
 * event-driven manner: As JSON data is received, asynchronous call-back functions are called
 * to deliver parsed information or an error message.
 *
 * Parsing stops automatically when the end of the document is reached or an error is encountered.
 *
 * le_json_Cleanup() must be called to release memory resources allocated by the parser.
 *
 * If the document starts with a '{', then it will finish with the matching '}'.
 *
 * If it starts with a '[', then it will finish with the matching ']'.
 *
 * All documents must start with either '{' or '['.
 *
 * To stop parsing early, call le_json_Cleanup() early.
 *
 * @warning Be sure to stop parsing before closing the file descriptor.
 *
 *  @section c_json_events Event Handling
 *
 * As parsing progresses and the parser finds things inside the JSON document, the parser calls
 * the event handler function to report the findings.
 *
 * For example, when the parser finds an object member, it calls the event handler function
 * with the event code LE_JSON_OBJECT_MEMBER; and when if finds a string value, an LE_JSON_STRING
 * event is reported.
 *
 * The event handler function can call functions to fetch values, depending on the event:
 * - LE_JSON_OBJECT_MEMBER: le_json_GetString() fetches the object member name.
 * - LE_JSON_STRING: le_json_GetString() fetches the string value.
 * - LE_JSON_NUMBER: le_json_GetNumber() fetches the number value.
 *
 * le_json_GetString() and le_json_GetNumber() can only be called from inside of a JSON parsing
 * event handler function or any function being called (directly or indirectly) from a JSON
 * parsing event handler.  Calling these functions elsewhere will be fatal to the calling process.
 *
 *  @section c_json_context Context
 *
 * Each JSON object, object member and array in the JSON document is a "context".
 * Each context has an event handler function and an opaque pointer associated with it.
 * The top level context's event handler and opaque pointer are passed into le_json_Parse().
 * Sub-contexts (object members or array elements) will inherit their context from their parent.
 *
 * The current context's event handler can be changed from within an event handler function by
 * calling le_json_SetEventHandler().  This will remain in effect until the parser finishes parsing
 * that part of the document and returns back to its parent, at which time the current context
 * will be automatically restored to the parent's context.
 *
 *  @section c_json_errors Error Handling
 *
 * There is a global error handler that is also set when the parsing is started, and can be changed
 * by calling le_json_SetErrorHandler().  Unlike other event handlers, this is not part of the
 * context, and will therefore not get restored to a previous handler when the parsing of a member
 * finishes.  The error handler function is passed parameters that indicate what type of error
 * occurred.
 *
 *  @section c_json_otherFunctions Other Functions
 *
 * For diagnostic purposes, le_json_GetEventName() can be called to get a human-readable
 * string containing the name of a given event.
 *
 * To get the number of bytes that have been read by the parser since le_json_Parse() was called,
 * call le_json_GetBytesRead().
 *
 *  @section c_json_example Example
 *
 * If the JSON document is
 *
 * @code
 * { "x":1, "y":2, "name":"joe" }
 * @endcode
 *
 * The following sequence of events will be reported by the parser:
 * 1. LE_JSON_OBJECT_START
 * 2. LE_JSON_OBJECT_MEMBER - If the event handler calls le_json_GetString(), it will return "x".
 * 3. LE_JSON_NUMBER - If the event handler calls le_json_GetNumber(), it will return 1.
 * 4. LE_JSON_OBJECT_MEMBER - If the event handler calls le_json_GetString(), it will return "y".
 * 5. LE_JSON_NUMBER - If the event handler calls le_json_GetNumber(), it will return 2.
 * 6. LE_JSON_OBJECT_MEMBER - If the event handler calls le_json_GetString(), it will return "name".
 * 7. LE_JSON_STRING - If the event handler calls le_json_GetString(), it will return "joe".
 * 8. LE_JSON_OBJECT_END
 * 8. LE_JSON_DOC_END - At this point, parsing stops.
 *
 * If the handler function passed to le_json_Parse() is called "<c>TopLevelHandler()</c>",
 * TopLevelHandler() will be called for the all events.  But, when <c>TopLevelHandler()</c> gets
 * the event LE_JSON_OBJECT_MEMBER for the member "x" and responds by calling
 * <c>le_json_SetEventHandler(XHandler(), NULL)</c>, then the LE_JSON_NUMBER event for "x" will
 * be passed to <c>XHandler()</c>.  But, the following LE_JSON_OBJECT_MEMBER event for "y" will
 * still go to <c>TopLevelHandler()</c>, because the context returns to the top level object
 * after the parser finishes parsing member "x".
 *
 *  @section c_json_threads Multi-Threading
 *
 * This API is not thread safe.  DO NOT attempt to SHARE parsers between threads.
 *
 * If a thread dies, any parsers in use by that thread that have not been cleaned-up by calls to
 * le_json_Cleanup() will be cleaned up automatically.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

/** @file le_json.h
 *
 * Legato @ref c_json include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_JSON_H_INCLUDE_GUARD
#define LEGATO_JSON_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of all the different events that can be reported during JSON document parsing.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_JSON_OBJECT_START,   ///< object started, subsequent object members are part of this object
    LE_JSON_OBJECT_MEMBER,  ///< object member name received: Call le_json_GetString() to get name
    LE_JSON_OBJECT_END,     ///< object finished, subsequent members/values are outside this object
    LE_JSON_ARRAY_START,    ///< array started, upcoming values are elements of this array
    LE_JSON_ARRAY_END,      ///< array finished, subsequent values are outside this array
    LE_JSON_STRING,         ///< string value received: call le_json_GetString() to get value
    LE_JSON_NUMBER,         ///< number value received: call le_json_GetNumber() to get value
    LE_JSON_TRUE,           ///< true value received
    LE_JSON_FALSE,          ///< false value received
    LE_JSON_NULL,           ///< null value received
    LE_JSON_DOC_END,        ///< End of the document reached.  Parsing has stopped.
}
le_json_Event_t;


//--------------------------------------------------------------------------------------------------
/**
 * Callbacks for (non-error) parsing events look like this.
 *
 * @param event     [in] Indicates what type of event occured.
 */
//--------------------------------------------------------------------------------------------------
typedef void (* le_json_EventHandler_t)
(
    le_json_Event_t event
);


//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of the different types of errors that can be reported during JSON document parsing.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_JSON_SYNTAX_ERROR,   ///< Syntax error, such as a missing comma or extra comma.
    LE_JSON_READ_ERROR,     ///< Error when reading from the input byte stream.
}
le_json_Error_t;


//--------------------------------------------------------------------------------------------------
/**
 * Callbacks for errors look like this.
 *
 * @param error     [in] Indicates what type of error occured.
 *
 * @param msg       [in] Human-readable message describing the error. (Valid until handler returns.)
 */
//--------------------------------------------------------------------------------------------------
typedef void (* le_json_ErrorHandler_t)
(
    le_json_Error_t error,
    const char* msg
);


//--------------------------------------------------------------------------------------------------
/**
 * Parsing session reference.  Refers to a parsing session started by le_json_Parse().  Pass this
 * to le_json_Cleanup() to stop the parsing and clean up memory allocated by the parser.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_json_ParsingSession* le_json_ParsingSessionRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Parse a JSON document received via a file descriptor.
 *
 * @return Reference to the JSON parsing session started by this function call.
 */
//--------------------------------------------------------------------------------------------------
le_json_ParsingSessionRef_t le_json_Parse
(
    int fd, ///< File descriptor to read the JSON document from.
    le_json_EventHandler_t  eventHandler,   ///< Function to call when normal parsing events happen.
    le_json_ErrorHandler_t  errorHandler,   ///< Function to call when errors happen.
    void* opaquePtr   ///< Opaque pointer to be fetched by handlers using le_json_GetOpaquePtr().
);

//--------------------------------------------------------------------------------------------------
/**
 * Parse a JSON document received via C string.
 *
 * @return Reference to the JSON parsing session started by this function call.
 */
//--------------------------------------------------------------------------------------------------
le_json_ParsingSessionRef_t le_json_ParseString
(
    const char *jsonString, ///< JSON string to parse.
    le_json_EventHandler_t  eventHandler,   ///< Function to call when normal parsing events happen.
    le_json_ErrorHandler_t  errorHandler,   ///< Function to call when errors happen.
    void* opaquePtr   ///< Opaque pointer to be fetched by handlers using le_json_GetOpaquePtr().
);

//--------------------------------------------------------------------------------------------------
/**
 * Stops parsing and cleans up memory allocated by the parser.
 *
 * @warning Be sure to stop parsing before closing the file descriptor.
 */
//--------------------------------------------------------------------------------------------------
void le_json_Cleanup
(
    le_json_ParsingSessionRef_t session ///< The parsing session to clean up (see le_json_Parse()).
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the current context's event handler function.
 *
 * @warning This function can only be called inside event or error handlers.
 */
//--------------------------------------------------------------------------------------------------
void le_json_SetEventHandler
(
    le_json_EventHandler_t  callbackFunc  ///< Function to call when parsing events happen.
);


//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of the different types of "contexts" that can exist during a parsing session.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_JSON_CONTEXT_DOC,    ///< Top level of document, outside the main object/array.
    LE_JSON_CONTEXT_OBJECT, ///< Parsing an object (set of named members).
    LE_JSON_CONTEXT_MEMBER, ///< Parsing a member of an object.
    LE_JSON_CONTEXT_ARRAY,  ///< Parsing an array (list of unnamed elements).
    LE_JSON_CONTEXT_STRING, ///< Parsing a string value.
    LE_JSON_CONTEXT_NUMBER, ///< Parsing number value.
    LE_JSON_CONTEXT_TRUE,   ///< Parsing a true value.
    LE_JSON_CONTEXT_FALSE,  ///< Parsing a false value.
    LE_JSON_CONTEXT_NULL,   ///< Parsing a null value.
}
le_json_ContextType_t;


//--------------------------------------------------------------------------------------------------
/**
 * Get the type of parsing context that the parser is currently in.
 *
 * @return The context type.
 *
 * @warning This function can only be called inside event or error handlers.
 */
//--------------------------------------------------------------------------------------------------
le_json_ContextType_t le_json_GetContextType
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the opaque pointer attached to the parser.
 *
 * @warning This function can only be called inside event or error handlers.
 */
//--------------------------------------------------------------------------------------------------
void le_json_SetOpaquePtr
(
    void* ptr    ///< Opaque pointer to be fetched using le_json_GetOpaquePtr().
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the opaque pointer attached to the parser.
 *
 * @return The pointer previously set by le_json_Parse() or a subsequent call to
 *         le_json_SetOpaquePtr().
 *
 * @warning This function can only be called inside event or error handlers.
 */
//--------------------------------------------------------------------------------------------------
void* le_json_GetOpaquePtr
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the error handler function
 *
 * @warning This function can only be called inside event or error handlers.
 */
//--------------------------------------------------------------------------------------------------
void le_json_SetErrorHandler
(
    le_json_ErrorHandler_t  callbackFunc    ///< Function to call when parsing errors happen.
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a pointer to a string value or object member name.
 *
 * This pointer is only valid until the event handler returns.
 *
 * @warning This function can only be called inside event handlers when LE_JSON_OBJECT_MEMBER
 *          or LE_JSON_STRING events are being handled.
 */
//--------------------------------------------------------------------------------------------------
const char* le_json_GetString
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the value of a parsed number.
 *
 * @warning This function can only be called inside event handlers when LE_JSON_NUMBER events are
 *          being handled.
 */
//--------------------------------------------------------------------------------------------------
double le_json_GetNumber
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * @return Human readable string containing the name of a given JSON parsing event.
 */
//--------------------------------------------------------------------------------------------------
const char* le_json_GetEventName
(
    le_json_Event_t event
);


//--------------------------------------------------------------------------------------------------
/**
 * @return Human readable string containing the name of a given JSON parsing context.
 */
//--------------------------------------------------------------------------------------------------
const char* le_json_GetContextName
(
    le_json_ContextType_t context
);


//--------------------------------------------------------------------------------------------------
/**
 * @return The number of bytes that have been read from the input stream so far.
 */
//--------------------------------------------------------------------------------------------------
size_t le_json_GetBytesRead
(
    le_json_ParsingSessionRef_t session ///< Parsing session.
);


//--------------------------------------------------------------------------------------------------
/**
 * For use by an event handler or error handler to fetch the JSON parsing session reference for
 * the session that called its handler function.
 *
 * @return The session reference of the JSON parsing session.
 *
 * @warning This function can only be called inside event or error handlers.
 */
//--------------------------------------------------------------------------------------------------
le_json_ParsingSessionRef_t le_json_GetSession
(
    void
);


#endif // LEGATO_JSON_H_INCLUDE_GUARD
