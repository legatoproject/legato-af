//--------------------------------------------------------------------------------------------------
/**
 * @file json.c JSON Parsing API implementation
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"


/// Maximum number of bytes allowed in a string value, object member name, or number's text
/// including the null terminator.
#define MAX_STRING_BYTES 1024


//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of different sets of things that are expected next.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    EXPECT_NOTHING, ///< Parsing stopped.
    EXPECT_OBJECT_OR_ARRAY,
    EXPECT_MEMBER_OR_OBJECT_END,
    EXPECT_COLON,
    EXPECT_VALUE,
    EXPECT_COMMA_OR_OBJECT_END,
    EXPECT_MEMBER,
    EXPECT_VALUE_OR_ARRAY_END,
    EXPECT_COMMA_OR_ARRAY_END,
    EXPECT_STRING,
    EXPECT_NUMBER,
    EXPECT_TRUE,
    EXPECT_FALSE,
    EXPECT_NULL,
}
Expected_t;


//--------------------------------------------------------------------------------------------------
/**
 * Each instance of the parser needs one of these to keep track of its state.
 *
 * These are allocated from the Parser Pool.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_json_ParsingSession
{
    Expected_t next;          ///< What's expected next?

    char buffer[MAX_STRING_BYTES];  ///< Buffer into which characters are copied
    size_t numBytes;                ///< # of bytes of content in the buffer.
    double number;                  ///< Value of last number parsed.

    int fd;                         ///< File descriptor to read the JSON document from, if parsing
                                    ///< from a document.
    le_fdMonitor_Ref_t fdMonitor;   ///< File Descriptor Monitor used to monitor the fd.
    const char *jsonString;         ///< String to read from, if parsing from a string.
    size_t bytesRead;               ///< # of bytes read from the file descriptor.
    size_t line;                    ///< Line number of the JSON document (starts at 1).

    le_json_ErrorHandler_t errorHandler; ///< Function to call when errors happen.
    void* opaquePtr;                ///< Client's opaque pointer passed to le_json_Parse().

    le_thread_DestructorRef_t threadDestructor; ///< Ref to thread death destructor for this parser.

    le_sls_List_t contextStack;     ///< Stack of Context records.
}
Parser_t;


//--------------------------------------------------------------------------------------------------
/**
 * Context record.  Keeps track of the event handler function and opaque pointer that belongs
 * to a given parsing context.
 *
 * These are allocated from the Context Pool and are kept on a Parser instance's Context Stack.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_Link_t link;     ///< Used to link this object into a Parser object's Context Stack.

    le_json_ContextType_t type;     ///< Type of JSON syntax structure being parsed.

    le_json_EventHandler_t  eventHandler;   ///< Called when parsing events happen in this context.
}
Context_t;


// Static memory pool the parser instances are allocated from
LE_MEM_DEFINE_STATIC_POOL(JSONParser, 1, sizeof(Parser_t));

// Memory pool reference for the pool that parser instance records are allocated from.
static le_mem_PoolRef_t ParserPool;

// Static memory pool that context records are allocated from
LE_MEM_DEFINE_STATIC_POOL(JSONContext, 10, sizeof(Context_t));

// Memory pool reference for the pool that context records are allocated from.
static le_mem_PoolRef_t ContextPool;


/// Thread-local data key for use by the event and error handler functions.
/// When inside a handler, this thread-local data value will be a pointer to a Parser object.
/// Outside handlers, it will be NULL.
static pthread_key_t HandlerKey;


//--------------------------------------------------------------------------------------------------
/**
 * @return true if parsing has not been stopped.
 **/
//--------------------------------------------------------------------------------------------------
static inline bool NotStopped
(
    Parser_t* parserPtr
)
//--------------------------------------------------------------------------------------------------
{
    return (parserPtr->next != EXPECT_NOTHING);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops parsing.  (Stopping a stopped parser is okay.)
 */
//--------------------------------------------------------------------------------------------------
static void StopParsing
(
    Parser_t* parserPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (NotStopped(parserPtr))
    {
        parserPtr->next = EXPECT_NOTHING;
        if (parserPtr->fdMonitor != NULL)
        {
            le_fdMonitor_Delete(parserPtr->fdMonitor);
            parserPtr->fdMonitor = NULL;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function for Parser objects.  Called when the object is about to be released back
 * into the Parser Pool.
 */
//--------------------------------------------------------------------------------------------------
static void ParserDestructor
(
    void* blockPtr
)
//--------------------------------------------------------------------------------------------------
{
    Parser_t* parserPtr = blockPtr;

    StopParsing(parserPtr);

    le_sls_Link_t* linkPtr;

    while ((linkPtr = le_sls_Pop(&parserPtr->contextStack)) != NULL)
    {
        le_mem_Release(CONTAINER_OF(linkPtr, Context_t, link));
    }

    le_thread_RemoveDestructor(parserPtr->threadDestructor);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the JSON parser module.
 *
 * Must be called exactly once at start-up before any other JSON module functions are called.
 */
//--------------------------------------------------------------------------------------------------
void json_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Create the memory pools.
    ParserPool = le_mem_InitStaticPool(JSONParser, 1, sizeof(Parser_t));
    le_mem_SetDestructor(ParserPool, ParserDestructor);
    ContextPool = le_mem_InitStaticPool(JSONContext, 10, sizeof(Context_t));

    // Initialize the thread-local data key.
    pthread_key_create(&HandlerKey, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called for each parser if the thread is dying.  Deletes the parser.
 */
//--------------------------------------------------------------------------------------------------
static void ThreadDeathHandler
(
    void* parserPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Release the client's reference to the parser object.
    le_mem_Release(parserPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the event handler function that is currently in effect, given the parser's current context.
 *
 * @return the event handler.
 */
//--------------------------------------------------------------------------------------------------
static le_json_EventHandler_t GetEventHandler
(
    Parser_t* parserPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_sls_Link_t* linkPtr = le_sls_Peek(&parserPtr->contextStack);

    LE_ASSERT(linkPtr != NULL);

    return CONTAINER_OF(linkPtr, Context_t, link)->eventHandler;
}


//--------------------------------------------------------------------------------------------------
/**
 * Report an parsing event to the client.
 */
//--------------------------------------------------------------------------------------------------
static void Report
(
    Parser_t* parserPtr,
    le_json_Event_t event
)
//--------------------------------------------------------------------------------------------------
{
    // Set the thread-local pointer to the parser object for the handler to use.
    pthread_setspecific(HandlerKey, parserPtr);

    // Call the client's handler function.
    GetEventHandler(parserPtr)(event);

    // Clear the thread-local pointer to the parser object so we can tell we are not in a handler.
    pthread_setspecific(HandlerKey, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Report an error and stop parsing.
 */
//--------------------------------------------------------------------------------------------------
static void Error
(
    Parser_t* parserPtr,
    le_json_Error_t error,
    const char* msg
)
//--------------------------------------------------------------------------------------------------
{
    // Add the line number to the error message
    char errorMessage[256];
    snprintf(errorMessage, sizeof(errorMessage), "%s (at line %" PRIuS ")", msg, parserPtr->line);

    StopParsing(parserPtr);

    // Set the thread-local pointer to the parser object for the handler to use.
    pthread_setspecific(HandlerKey, parserPtr);

    // Call the client's handler function.
    parserPtr->errorHandler(error, errorMessage);

    // Clear the thread-local pointer to the parser object so we can tell we are not in a handler.
    pthread_setspecific(HandlerKey, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the current parser for the currently running JSON parser event or error
 * handler function.
 *
 * @return Pointer to the Parser object.
 */
//--------------------------------------------------------------------------------------------------
static Parser_t* GetCurrentParser
(
    const char* callingFuncName ///< Name of the calling function (for diagnostics)
)
//--------------------------------------------------------------------------------------------------
{
    Parser_t* parserPtr = pthread_getspecific(HandlerKey);

    LE_FATAL_IF(parserPtr == NULL,
                "%s() Called from outside a JSON parser event or error handler function",
                callingFuncName);

    return parserPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the current context for a given parser.
 *
 * @return Pointer to the Context object.
 */
//--------------------------------------------------------------------------------------------------
static Context_t* GetContext
(
    Parser_t* parserPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_sls_Link_t* linkPtr = le_sls_Peek(&parserPtr->contextStack);

    LE_ASSERT(linkPtr != NULL);

    return CONTAINER_OF(linkPtr, Context_t, link);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the current context for the currently running JSON parser event or error
 * handler function.
 *
 * @return Pointer to the Context object.
 */
//--------------------------------------------------------------------------------------------------
static Context_t* GetCurrentContext
(
    const char* callingFuncName ///< Name of the calling function (for diagnostics)
)
//--------------------------------------------------------------------------------------------------
{
    return GetContext(GetCurrentParser(callingFuncName));
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a new context and push it onto the parser's context stack.
 */
//--------------------------------------------------------------------------------------------------
static void PushContext
(
    Parser_t* parserPtr,
    le_json_ContextType_t type,
    le_json_EventHandler_t eventHandler
)
//--------------------------------------------------------------------------------------------------
{
    Context_t* contextPtr = le_mem_ForceAlloc(ContextPool);

    contextPtr->link = LE_SLS_LINK_INIT;
    contextPtr->type = type;
    contextPtr->eventHandler = eventHandler;

    le_sls_Stack(&parserPtr->contextStack, &contextPtr->link);

    // Clear the value buffer.
    memset(parserPtr->buffer, 0, sizeof(parserPtr->buffer));
    parserPtr->numBytes = 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Pops the parser's context stack and sets the next expected thing according to the new context.
 */
//--------------------------------------------------------------------------------------------------
static void PopContext
(
    Parser_t* parserPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Don't do anything if a client handler has already stopped parsing.
    if (NotStopped(parserPtr))
    {
        // Pop the top one and release it.
        le_sls_Link_t* linkPtr = le_sls_Pop(&parserPtr->contextStack);
        le_mem_Release(CONTAINER_OF(linkPtr, Context_t, link));

        // Check the new context
        le_json_ContextType_t context = GetContext(parserPtr)->type;

        switch (context)
        {
            case LE_JSON_CONTEXT_DOC:

                // We've finished parsing the whole document.
                // Automatically stop parsing and report the document end to the client.
                StopParsing(parserPtr);
                Report(parserPtr, LE_JSON_DOC_END);
                break;

            case LE_JSON_CONTEXT_OBJECT:

                // We've finished parsing an object member.
                // Expect a comma separator or the end of the object next.
                parserPtr->next = EXPECT_COMMA_OR_OBJECT_END;
                break;

            case LE_JSON_CONTEXT_MEMBER:

                // We've finished parsing the value of an object member.
                // This is also the end of the member, so pop that context too.
                PopContext(parserPtr);
                break;

            case LE_JSON_CONTEXT_ARRAY:

                // We've finished parsing an array element value.
                // Expect a comma separator or the end of the array next.
                parserPtr->next = EXPECT_COMMA_OR_ARRAY_END;
                break;

            // These are all leaf contexts.  That is, we should never find one of these
            // contexts after popping another one off the stack.
            case LE_JSON_CONTEXT_STRING:
            case LE_JSON_CONTEXT_NUMBER:
            case LE_JSON_CONTEXT_TRUE:
            case LE_JSON_CONTEXT_FALSE:
            case LE_JSON_CONTEXT_NULL:

                LE_FATAL("Unexpected context after pop: %s", le_json_GetContextName(context));
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a byte to the parser's string buffer.
 */
//--------------------------------------------------------------------------------------------------
static void AddToBuffer
(
    Parser_t* parserPtr,
    char c
)
//--------------------------------------------------------------------------------------------------
{
    if (parserPtr->numBytes >= (sizeof(parserPtr->buffer) - 1))
    {
        Error(parserPtr, LE_JSON_READ_ERROR, "Content item too long to fit in internal buffer.");
    }
    else
    {
        parserPtr->buffer[parserPtr->numBytes] = c;
        parserPtr->numBytes++;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Process a character in a state when a value is expected to start.
 */
//--------------------------------------------------------------------------------------------------
static void ParseValue
(
    Parser_t* parserPtr,
    char c
)
//--------------------------------------------------------------------------------------------------
{
    // Throw away whitespace until something else comes along.
    if (!isspace(c))
    {
        if (c == '{')   // Start of an object.
        {
            PushContext(parserPtr, LE_JSON_CONTEXT_OBJECT, GetEventHandler(parserPtr));
            parserPtr->next = EXPECT_MEMBER_OR_OBJECT_END;
            Report(parserPtr, LE_JSON_OBJECT_START);
        }
        else if (c == '[')  // Start of an array.
        {
            PushContext(parserPtr, LE_JSON_CONTEXT_ARRAY, GetEventHandler(parserPtr));
            parserPtr->next = EXPECT_VALUE_OR_ARRAY_END;
            Report(parserPtr, LE_JSON_ARRAY_START);
        }
        else if (c == '"')
        {
            PushContext(parserPtr, LE_JSON_CONTEXT_STRING, GetEventHandler(parserPtr));
            parserPtr->next = EXPECT_STRING;
        }
        else if (c == 't')
        {
            PushContext(parserPtr, LE_JSON_CONTEXT_TRUE, GetEventHandler(parserPtr));
            AddToBuffer(parserPtr, c);
            parserPtr->next = EXPECT_TRUE;
        }
        else if (c == 'f')
        {
            PushContext(parserPtr, LE_JSON_CONTEXT_FALSE, GetEventHandler(parserPtr));
            AddToBuffer(parserPtr, c);
            parserPtr->next = EXPECT_FALSE;
        }
        else if (c == 'n')
        {
            PushContext(parserPtr, LE_JSON_CONTEXT_NULL, GetEventHandler(parserPtr));
            AddToBuffer(parserPtr, c);
            parserPtr->next = EXPECT_NULL;
        }
        else if (isdigit(c) || (c == '-'))
        {
            PushContext(parserPtr, LE_JSON_CONTEXT_NUMBER, GetEventHandler(parserPtr));
            AddToBuffer(parserPtr, c);
            parserPtr->next = EXPECT_NUMBER;
        }
        else
        {
            Error(parserPtr,
                  LE_JSON_SYNTAX_ERROR,
                  "Unexpected character at beginning of value.");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a fixed string, (true, false, or null).
 */
//--------------------------------------------------------------------------------------------------
static void ParseConstant
(
    Parser_t* parserPtr,
    char c,
    const char* expected
)
//--------------------------------------------------------------------------------------------------
{
    AddToBuffer(parserPtr, c);

    if (strncmp(parserPtr->buffer, expected, parserPtr->numBytes) != 0)
    {
        char msg[256];

        snprintf(msg,
                 sizeof(msg),
                 "Unexpected character '%c' (expected '%c' in '%s').",
                 c,
                 expected[parserPtr->numBytes - 1],
                 expected);

        Error(parserPtr, LE_JSON_SYNTAX_ERROR, msg);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Process a number that we have just received all the characters for.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessNumber
(
    Parser_t* parserPtr
)
//--------------------------------------------------------------------------------------------------
{
    char* endPtr;

    errno = 0;

    // Attempt conversion.
    parserPtr->number = strtod(parserPtr->buffer, &endPtr);

    // If it stopped before the end, then there are bad characters in the number.
    if (endPtr[0] != '\0')
    {
        Error(parserPtr, LE_JSON_SYNTAX_ERROR, "Invalid characters in number.");
    }
    // If errno has been set, then there was an underflow or overflow in the conversion.
    else if (errno == ERANGE)
    {
        if (parserPtr->number == 0)
        {
            Error(parserPtr, LE_JSON_READ_ERROR, "Numerical underflow occurred.");
        }
        else
        {
            Error(parserPtr, LE_JSON_READ_ERROR, "Numerical overflow occurred.");
        }
    }
    // If all's good, report the value match to the client and pop the context.
    else
    {
        Report(parserPtr, LE_JSON_NUMBER);
        PopContext(parserPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse string characters.  Could be in a string value or an object member name.
 */
//--------------------------------------------------------------------------------------------------
static void ParseString
(
    Parser_t* parserPtr,
    char c
)
//--------------------------------------------------------------------------------------------------
{
    // See if this is a string terminating '"' character.
    if (c == '"')
    {
        if ((parserPtr->numBytes != 0) && (parserPtr->buffer[parserPtr->numBytes - 1] == '\\'))
        {
            // Escaped ", replace with ".
            parserPtr->buffer[parserPtr->numBytes - 1] = '"';
        }
        else
        {
            // Make we have a valid UTF-8 string.
            if (!le_utf8_IsFormatCorrect(parserPtr->buffer))
            {
                Error(parserPtr, LE_JSON_SYNTAX_ERROR, "String is not valid UTF-8.");
            }
            else
            {
                // Handling of the end of the string depends on the context.
                le_json_ContextType_t contextType = GetContext(parserPtr)->type;

                if (contextType == LE_JSON_CONTEXT_STRING)
                {
                    Report(parserPtr, LE_JSON_STRING);
                    PopContext(parserPtr);
                }
                else if (contextType == LE_JSON_CONTEXT_MEMBER)
                {
                    Report(parserPtr, LE_JSON_OBJECT_MEMBER);
                    parserPtr->next = EXPECT_COLON;
                }
                else
                {
                    LE_FATAL("Unexpected context '%s' for string termination.",
                             le_json_GetContextName(contextType));
                }
            }
        }
    }
    else
    {
        AddToBuffer(parserPtr, c);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Processes the next character read from the JSON document.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessChar
(
    Parser_t* parserPtr,
    char c
)
//--------------------------------------------------------------------------------------------------
{
    switch (parserPtr->next)
    {
        case EXPECT_OBJECT_OR_ARRAY:

            // Throw away whitespace characters until '{' or '[' is found.
            if (c == '{')
            {
                PushContext(parserPtr, LE_JSON_CONTEXT_OBJECT, GetEventHandler(parserPtr));
                parserPtr->next = EXPECT_MEMBER_OR_OBJECT_END;
                Report(parserPtr, LE_JSON_OBJECT_START);
            }
            else if (c == '[')
            {
                PushContext(parserPtr, LE_JSON_CONTEXT_ARRAY, GetEventHandler(parserPtr));
                parserPtr->next = EXPECT_VALUE_OR_ARRAY_END;
                Report(parserPtr, LE_JSON_ARRAY_START);
            }
            else if (!isspace(c))
            {
                Error(parserPtr, LE_JSON_SYNTAX_ERROR, "Document must start with '{' or '['.");
            }
            return;

        case EXPECT_MEMBER_OR_OBJECT_END:

            // Throw away whitespace until a '"' or '}' is found.
            if (c == '}')   // Object end found.
            {
                Report(parserPtr, LE_JSON_OBJECT_END);
                PopContext(parserPtr);
            }
            else if (c == '"')  // Start of member name (string) found.
            {
                PushContext(parserPtr, LE_JSON_CONTEXT_MEMBER, GetEventHandler(parserPtr));
                parserPtr->next = EXPECT_STRING;
            }
            else if (!isspace(c))
            {
                Error(parserPtr,
                      LE_JSON_SYNTAX_ERROR,
                      "Expected end of object (}) or beginning of object member name (\").");
            }
            return;

        case EXPECT_COLON:

            // Throw away whitespace until a ':' is found.
            if (c == ':')
            {
                parserPtr->next = EXPECT_VALUE;
            }
            else if (!isspace(c))
            {
                Error(parserPtr,
                      LE_JSON_SYNTAX_ERROR,
                      "Expected ':' after object member name.");
            }
            return;

        case EXPECT_VALUE:

            ParseValue(parserPtr, c);
            return;

        case EXPECT_COMMA_OR_OBJECT_END:

            // Throw away whitespace until a ',' or '}' is found.
            if (c == '}')   // Object end found.
            {
                Report(parserPtr, LE_JSON_OBJECT_END);
                PopContext(parserPtr);
            }
            else if (c == ',')  // Comma separator found.
            {
                parserPtr->next = EXPECT_MEMBER;
            }
            else if (!isspace(c))
            {
                Error(parserPtr,
                      LE_JSON_SYNTAX_ERROR,
                      "Expected end of object (}) or beginning of object member name (\").");
            }
            return;

        case EXPECT_MEMBER:

            // Throw away whitespace until a '"' is found.
            if (c == '"')  // Start of member name (string) found.
            {
                PushContext(parserPtr, LE_JSON_CONTEXT_MEMBER, GetEventHandler(parserPtr));
                parserPtr->next = EXPECT_STRING;
            }
            else if (!isspace(c))
            {
                Error(parserPtr,
                      LE_JSON_SYNTAX_ERROR,
                      "Expected beginning of object member name (\").");
            }
            return;

        case EXPECT_VALUE_OR_ARRAY_END:

            if (c == ']')
            {
                Report(parserPtr, LE_JSON_ARRAY_END);
                PopContext(parserPtr);
            }
            else
            {
                ParseValue(parserPtr, c);
            }
            return;

        case EXPECT_COMMA_OR_ARRAY_END:

            // Throw away whitespace until a ',' or ']' is found.
            if (c == ']')   // Array end found.
            {
                Report(parserPtr, LE_JSON_ARRAY_END);
                PopContext(parserPtr);
            }
            else if (c == ',')  // Comma separator found.
            {
                parserPtr->next = EXPECT_VALUE;
            }
            else if (!isspace(c))
            {
                Error(parserPtr,
                      LE_JSON_SYNTAX_ERROR,
                      "Expected end of array (]) or a comma separator (,).");
            }
            return;

        case EXPECT_STRING:

            ParseString(parserPtr, c);
            return;

        case EXPECT_NUMBER:

            if ((c == '.') || isdigit(c))
            {
                AddToBuffer(parserPtr, c);
            }
            else
            {
                ProcessNumber(parserPtr);
                ProcessChar(parserPtr, c);
            }
            return;

        case EXPECT_TRUE:

            ParseConstant(parserPtr, c, "true");
            if (strcmp(parserPtr->buffer, "true") == 0)
            {
                Report(parserPtr, LE_JSON_TRUE);
                PopContext(parserPtr);
            }
            return;

        case EXPECT_FALSE:

            ParseConstant(parserPtr, c, "false");
            if (strcmp(parserPtr->buffer, "false") == 0)
            {
                Report(parserPtr, LE_JSON_FALSE);
                PopContext(parserPtr);
            }
            return;

        case EXPECT_NULL:

            ParseConstant(parserPtr, c, "null");
            if (strcmp(parserPtr->buffer, "null") == 0)
            {
                Report(parserPtr, LE_JSON_NULL);
                PopContext(parserPtr);
            }
            return;

        case EXPECT_NOTHING:    ///< Parsing stopped.
            return;
    }

    LE_FATAL("Internal error: Invalid JSON parser expectation %d.", parserPtr->next);
}


//--------------------------------------------------------------------------------------------------
/**
 * Read data from the JSON document file descriptor and process it.
 */
//--------------------------------------------------------------------------------------------------
static void ReadData
(
    Parser_t* parserPtr,
    int fd
)
//--------------------------------------------------------------------------------------------------
{
    while (NotStopped(parserPtr))
    {
        char c;
        ssize_t bytesRead;
        do
        {
            bytesRead = read(fd, &c, 1);
        }
        while ((bytesRead == -1) && (errno == EINTR));

        if (bytesRead == 0) // End of file?
        {
            // The document has been truncated.
            Error(parserPtr, LE_JSON_READ_ERROR, "Unexpected end-of-file.");
            return;
        }
        else if (bytesRead < 0)
        {
            switch (errno)
            {
                case EAGAIN:
                    // Nothing left to read.  Return back to FD Monitor to wait for more.
                    return;

                case EBADF:
                    // Someone closed the file descriptor, or it's not open for reading.
                    Error(parserPtr, LE_JSON_READ_ERROR, "File not open for reading.");
                    return;

                case EIO:
                    // I/O error.
                    Error(parserPtr, LE_JSON_READ_ERROR, "I/O error.");
                    return;

                case EINVAL:
                    // They gave us an fd that's not set up right.
                    Error(parserPtr, LE_JSON_READ_ERROR, "Invalid file descriptor.");
                    return;

                case EISDIR:
                    // They gave us a directory fd?!
                    Error(parserPtr, LE_JSON_READ_ERROR, "Can't read from a directory.");
                    return;

                default:
                    Error(parserPtr, LE_JSON_READ_ERROR, strerror(errno));
                    return;
            }
        }
        else
        {
            parserPtr->bytesRead++;
            if (c == '\n')
            {
                parserPtr->line++;
            }
            ProcessChar(parserPtr, c);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Event handler that gets called when an event occurs on a monitored file descriptor.
 */
//--------------------------------------------------------------------------------------------------
static void FdEventHandler
(
    int fd,
    short events
)
//--------------------------------------------------------------------------------------------------
{
    Parser_t* parserPtr = le_fdMonitor_GetContextPtr();

    // Increment the reference count on the Parser object so it won't go away until we are done
    // with it, even if the client calls le_json_Cleanup() for this parser.
    le_mem_AddRef(parserPtr);

    if (events & POLLIN)    // Data available to read?
    {
        ReadData(parserPtr, fd);
    }

    // Error or hang-up?
    if (NotStopped(parserPtr) && ((events & POLLERR) || (events & POLLHUP) || (events & POLLRDHUP)))
    {
        char msg[256];
        size_t bytesPrinted = snprintf(msg, sizeof(msg), "Read error flags set:");
        if (events & POLLERR)
        {
            bytesPrinted += snprintf(msg + bytesPrinted, sizeof(msg) - bytesPrinted, " POLLERR");
        }
        if (events & POLLHUP)
        {
            bytesPrinted += snprintf(msg + bytesPrinted, sizeof(msg) - bytesPrinted, " POLLHUP");
        }
        if (events & POLLRDHUP)
        {
            bytesPrinted += snprintf(msg + bytesPrinted, sizeof(msg) - bytesPrinted, " POLLRDHUP");
        }
        Error(parserPtr, LE_JSON_READ_ERROR, msg);
    }

    // We are finished with the parser object now.
    le_mem_Release(parserPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read data from the JSON document string and process it.
 */
//--------------------------------------------------------------------------------------------------
static void StringEventHandler
(
    Parser_t    *parserPtr,
    void        *unused
)
{
    char c;

    LE_UNUSED(unused);

    // Increment the reference count on the Parser object so it won't go away until we are done
    // with it, even if the client calls le_json_Cleanup() for this parser.
    le_mem_AddRef(parserPtr);

    while (NotStopped(parserPtr))
    {
        c = parserPtr->jsonString[parserPtr->bytesRead];
        if (c == '\0') // End of string?
        {
            // The document has been truncated.
            Error(parserPtr, LE_JSON_READ_ERROR, "Unexpected end of JSON string");
            break;
        }
        else
        {
            parserPtr->bytesRead++;
            if (c == '\n')
            {
                parserPtr->line++;
            }
            ProcessChar(parserPtr, c);
        }
    }

    // We are finished with the parser object now.
    le_mem_Release(parserPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a new parser instance.
 *
 * @return Reference to the JSON parsing session created by this function call.
 */
//--------------------------------------------------------------------------------------------------
static le_json_ParsingSessionRef_t NewParser
(
    le_json_EventHandler_t  eventHandler,   ///< Function to call when normal parsing events happen.
    le_json_ErrorHandler_t  errorHandler,   ///< Function to call when errors happen.
    void* opaquePtr   ///< Opaque pointer to be fetched by handlers using le_json_GetOpaquePtr().
)
{
    Parser_t *parserPtr;

    LE_UNUSED(eventHandler);

    // Create a Parser.
    parserPtr = le_mem_ForceAlloc(ParserPool);
    memset(parserPtr, 0, sizeof(*parserPtr));

    parserPtr->next = EXPECT_OBJECT_OR_ARRAY;
    parserPtr->line = 1;

    parserPtr->errorHandler = errorHandler;
    parserPtr->opaquePtr = opaquePtr;

    // Register a thread destructor to be called to clean up this parser if the thread dies.
    parserPtr->threadDestructor = le_thread_AddDestructor(ThreadDeathHandler, parserPtr);

    parserPtr->contextStack = LE_SLS_LIST_INIT;

    return parserPtr;
}

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
)
//--------------------------------------------------------------------------------------------------
{
    // Create a Parser.
    Parser_t* parserPtr = NewParser(eventHandler, errorHandler, opaquePtr);

    parserPtr->fd = fd;
    parserPtr->fdMonitor = le_fdMonitor_Create("le_json", fd, FdEventHandler, POLLIN);
    le_fdMonitor_SetContextPtr(parserPtr->fdMonitor, parserPtr);

    // Create the top-level context and push it onto the context stack.
    PushContext(parserPtr, LE_JSON_CONTEXT_DOC, eventHandler);

    return parserPtr;
}

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
)
{
    // Create a Parser.
    Parser_t* parserPtr = NewParser(eventHandler, errorHandler, opaquePtr);

    parserPtr->fd = -1;
    parserPtr->jsonString = jsonString;
    le_event_QueueFunction((le_event_DeferredFunc_t) &StringEventHandler, parserPtr, NULL);

    // Create the top-level context and push it onto the context stack.
    PushContext(parserPtr, LE_JSON_CONTEXT_DOC, eventHandler);

    return parserPtr;
}

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
)
//--------------------------------------------------------------------------------------------------
{
    StopParsing(session);

    // Release the client's reference to the parser object.
    le_mem_Release(session);
}


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
)
//--------------------------------------------------------------------------------------------------
{
    Context_t* contextPtr = GetCurrentContext(__func__);

    contextPtr->eventHandler = callbackFunc;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    Context_t* contextPtr = GetCurrentContext(__func__);

    return contextPtr->type;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    Parser_t* parserPtr = GetCurrentParser(__func__);

    parserPtr->opaquePtr = ptr;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    return GetCurrentParser(__func__)->opaquePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the error handler function.
 *
 * @warning This function can only be called inside event or error handlers.
 */
//--------------------------------------------------------------------------------------------------
void le_json_SetErrorHandler
(
    le_json_ErrorHandler_t  callbackFunc    ///< Function to call when parsing errors happen.
)
//--------------------------------------------------------------------------------------------------
{
    GetCurrentParser(__func__)->errorHandler = callbackFunc;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    Parser_t* parserPtr = GetCurrentParser(__func__);

    if (parserPtr->next != EXPECT_STRING)
    {
        LE_FATAL("String not available.");
    }

    return parserPtr->buffer;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    Parser_t* parserPtr = GetCurrentParser(__func__);

    if (parserPtr->next != EXPECT_NUMBER)
    {
        LE_FATAL("Number not available.");
    }

    return parserPtr->number;
}


//--------------------------------------------------------------------------------------------------
/**
 * @return Human readable string containing the name of a given JSON parsing event.
 */
//--------------------------------------------------------------------------------------------------
const char* le_json_GetEventName
(
    le_json_Event_t event
)
//--------------------------------------------------------------------------------------------------
{
    switch (event)
    {
        case LE_JSON_OBJECT_START:

            return "OBJECT_START";

        case LE_JSON_OBJECT_MEMBER:

            return "OBJECT_MEMBER";

        case LE_JSON_OBJECT_END:

            return "OBJECT_END";

        case LE_JSON_ARRAY_START:

            return "ARRAY_START";

        case LE_JSON_ARRAY_END:

            return "ARRAY_END";

        case LE_JSON_STRING:

            return "STRING";

        case LE_JSON_NUMBER:

            return "NUMBER";

        case LE_JSON_TRUE:

            return "TRUE";

        case LE_JSON_FALSE:

            return "FALSE";

        case LE_JSON_NULL:

            return "NULL";

        case LE_JSON_DOC_END:

            return "DOC_END";
    }

    LE_FATAL("Bad event (%d).", event);
}


//--------------------------------------------------------------------------------------------------
/**
 * @return Human readable string containing the name of a given JSON parsing context.
 */
//--------------------------------------------------------------------------------------------------
const char* le_json_GetContextName
(
    le_json_ContextType_t context
)
//--------------------------------------------------------------------------------------------------
{
    switch (context)
    {
        case LE_JSON_CONTEXT_DOC:

            return "document";

        case LE_JSON_CONTEXT_OBJECT:

            return "object";

        case LE_JSON_CONTEXT_MEMBER:

            return "object member";

        case LE_JSON_CONTEXT_ARRAY:

            return "array";

        case LE_JSON_CONTEXT_STRING:

            return "string";

        case LE_JSON_CONTEXT_NUMBER:

            return "number";

        case LE_JSON_CONTEXT_TRUE:

            return "true";

        case LE_JSON_CONTEXT_FALSE:

            return "false";

        case LE_JSON_CONTEXT_NULL:

            return "null";
    }

    LE_FATAL("Bad context (%d).", context);
}


//--------------------------------------------------------------------------------------------------
/**
 * @return The number of bytes that have been read from the input stream so far.
 */
//--------------------------------------------------------------------------------------------------
size_t le_json_GetBytesRead
(
    le_json_ParsingSessionRef_t session ///< Parsing session.
)
//--------------------------------------------------------------------------------------------------
{
    return session->bytesRead;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    return GetCurrentParser(__func__);
}
