//--------------------------------------------------------------------------------------------------
/** @file legatoJni.c
 *
 * Shim layer to provide access to the Legato C library from Java.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "log.h"
#include "legatoJni.h"




/// Global pointer to the Java virtual machine this library has been loaded into.
static JavaVM* JvmPtr;




void _HexDump
(
    void* dataBuffer,
    size_t dataSize,
    const char* filenamePtr,
    const char* functionNamePtr,
    const unsigned int lineNumber
)
{
    char* ptr = (char*)dataBuffer;
    int i;

    for (i = 0; i < dataSize; i += 16)
    {
        _le_log_Send(LE_LOG_INFO, NULL, LE_LOG_SESSION, filenamePtr, functionNamePtr, lineNumber,
                     "%02x %02x %02x %02x %02x %02x %02x %02x -- %02x %02x %02x %02x %02x %02x"
                     " %02x %02x",
                     ptr[i +  0], ptr[i +  1], ptr[i +  2], ptr[i +  3],
                     ptr[i +  4], ptr[i +  5], ptr[i +  6], ptr[i +  7],
                     ptr[i +  8], ptr[i +  9], ptr[i + 10], ptr[i + 11],
                     ptr[i + 12], ptr[i + 13], ptr[i + 14], ptr[i + 15]);
    }

    _le_log_Send(LE_LOG_INFO, NULL, LE_LOG_SESSION, filenamePtr, functionNamePtr, lineNumber, "--");
}


#define HexDump(dataBuffer, dataSize) \
    _HexDump(dataBuffer, dataSize, STRINGIZE(LE_FILENAME), __func__, __LINE__)





//--------------------------------------------------------------------------------------------------
/**
 *  Construct a Java object to hold onto the active connection to the logging system.
 *
 *  @return: A new instance of a log handle object.
 */
//--------------------------------------------------------------------------------------------------
static jobject NewLogHandle
(
    JNIEnv* envPtr,                    ///< [IN] The Java environment to work out of.
    le_log_SessionRef_t logSession,    ///< [IN] The Legato log session to report on.
    le_log_Level_t* logLevelFilterPtr  ///< [IN] THe current filter level for the logs.
)
//--------------------------------------------------------------------------------------------------
{
    jclass classPtr = (*envPtr)->FindClass(envPtr, "io/legato/LogHandler$LogHandle");

    if (classPtr == NULL)
    {
        (*envPtr)->ExceptionDescribe(envPtr);
        (*envPtr)->ExceptionClear(envPtr);
        return NULL;
    }

    jmethodID constructorPtr = (*envPtr)->GetMethodID(envPtr,
                                                      classPtr,
                                                      "<init>",
                                                      "(Lio/legato/LogHandler;JJ)V");

    if (constructorPtr == NULL)
    {
        (*envPtr)->ExceptionDescribe(envPtr);
        (*envPtr)->ExceptionClear(envPtr);
        return NULL;
    }

    return (*envPtr)->NewObject(envPtr,
                                classPtr,
                                constructorPtr,
                                NULL,
                                (jlong)(intptr_t)logSession,
                                (jlong)(intptr_t)logLevelFilterPtr);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Construct a new object instance of the named Java class.  This function looks for a constructor
 *  that can take a single long parameter.  If this function fails then  an exception will be raised
 *  in the Java VM on exit from the JNI code.
 *
 *  @return A new pointer to a Java object if successful, NULL if the construction fails.
 */
//--------------------------------------------------------------------------------------------------
static jobject ConstructObjectFromHandle
(
    JNIEnv* envPtr,         ///< [IN] The Java environment to work out of.
    const char* className,  ///< [IN] The name of the class to construct an object from.
    jlong ref               ///< [IN] The "native" object handle to construct the class around.
)
//--------------------------------------------------------------------------------------------------
{
    // First, try to find the named class.
    jclass classPtr = (*envPtr)->FindClass(envPtr, className);
    if (classPtr == NULL)
    {
        // An exception will have been thrown now.
        return NULL;
    }

    // Now try to find the constructor.  This will fail if the class in question does not have one
    // that matches our requirements.
    jmethodID constructorPtr = (*envPtr)->GetMethodID(envPtr, classPtr, "<init>", "(J)V");
    if (constructorPtr == NULL)
    {
        // An exception will have been thrown now.
        return NULL;
    }

    // Finally, create and return the new object.
    return (*envPtr)->NewObject(envPtr, classPtr, constructorPtr, ref);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Construct a Java file descriptor object based off of a native Unix file descriptor.
 *
 *  @return A Java file descriptor object if successful.  A NULL and a raised exception if failed.
 */
//--------------------------------------------------------------------------------------------------
static jobject CreateFileDescriptor
(
        JNIEnv* envPtr,  ///< [IN] The Java environment to work out of.
        int fd           ///< [IN] The descriptor to construct an object around.
)
//--------------------------------------------------------------------------------------------------
{
    // Find the java class.
    jclass descriptorClassPtr = (*envPtr)->FindClass(envPtr, "java/io/FileDescriptor");
    if (descriptorClassPtr == NULL)
    {
        return NULL;
    }

    // Look for and invoke the constructor that can take a descriptor.
    jmethodID constructorPtr = (*envPtr)->GetMethodID(envPtr, descriptorClassPtr, "<init>", "(I)V");
    if (constructorPtr == NULL)
    {
        return NULL;
    }

    return (*envPtr)->NewObject(envPtr, descriptorClassPtr, constructorPtr, fd);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Extract a native file descriptor from a Java file descriptor object.
 *
 *  @return The value if the FD field from within the FileDescriptor object.
 */
//--------------------------------------------------------------------------------------------------
static int ExtractFd
(
    JNIEnv* envPtr,            ///< [IN] The Java environment to work out of.
    jobject fileDescriptorPtr  ///< [IN] The file descriptor object to read.
)
//--------------------------------------------------------------------------------------------------
{
    // Get the class for the descriptor and lookup the fd field.  Once that's done, read the field
    // and return the value to the caller.
    jclass descriptorClassPtr = (*envPtr)->GetObjectClass(envPtr, fileDescriptorPtr);

    jfieldID fdFieldPtr = (*envPtr)->GetFieldID(envPtr, descriptorClassPtr, "fd", "I");
    if (fdFieldPtr == NULL)
    {
        return -1;
    }

    return (*envPtr)->GetIntField(envPtr, fileDescriptorPtr, fdFieldPtr);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Function used on event callback objects.  This will take the given Java object and lookup it's
 *  "handle" method.  Once that's done, it'll take the given object parameter and pass it along to
 *  the Java method on the object.
 */
//--------------------------------------------------------------------------------------------------
static void CallHandleMethod
(
    JNIEnv* envPtr,               ///< [IN] The Java environment to work out of.
    jobject objectPtr,            ///< [IN] The object we're calling a method on.
    const char* methodSignature,  ///< [IN] Signature of the method we're about to lookup.
    jobject parameterPtr          ///< [IN] The parameter we're passing to the object.
)
//--------------------------------------------------------------------------------------------------
{
    jclass eventClass = (*envPtr)->GetObjectClass(envPtr, objectPtr);
    if (eventClass == NULL)
    {
        (*envPtr)->ExceptionDescribe(envPtr);
        return;
    }

    jmethodID handerIdPtr = (*envPtr)->GetMethodID(envPtr,
                                                   eventClass,
                                                   "handle",
                                                   methodSignature);
    if (handerIdPtr == NULL)
    {
        (*envPtr)->ExceptionDescribe(envPtr);
        return;
    }

    (*envPtr)->CallVoidMethod(envPtr, objectPtr, handerIdPtr, parameterPtr);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Native function to handle Session Events.  This function will in turn attempt to call the Java
 *  based method to do the real work of the callback.
 */
//--------------------------------------------------------------------------------------------------
static void SessionEventHandler
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] Reference to the session the event occurred on.
    void* contextPtr                 ///< [IN] Stored context for the event.
)
//--------------------------------------------------------------------------------------------------
{
    // Get an environment pointer from the JVM we're running under.
    JNIEnv* envPtr = NULL;
    jint rs = (*JvmPtr)->AttachCurrentThread(JvmPtr, (void**)&envPtr, NULL);
    LE_ASSERT(rs == JNI_OK);

    // Now, construct a session object from the reference.
    jobject sessionPtr = ConstructObjectFromHandle(envPtr,
                                                   "io/legato/Session",
                                                   (jlong)(intptr_t)sessionRef);
    if (sessionPtr == NULL)
    {
        (*envPtr)->ExceptionDescribe(envPtr);
        return;
    }

    // Finally pass the new session object to the handler object's handle method.
    jobject handlerObjPtr = (jobject)contextPtr;
    CallHandleMethod(envPtr, handlerObjPtr, "(Ljava/lang/Object;)V", sessionPtr);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Low level callback for session message events.  This function will in turn attempt to call the
 *  registered Java handler for this callback.
 */
//--------------------------------------------------------------------------------------------------
static void SessionReceiveHandler
(
    le_msg_MessageRef_t msgRef,  ///< [IN] The message received.
    void* contextPtr             ///< [IN] Context pointer for the message handler.
)
//--------------------------------------------------------------------------------------------------
{
    // Use the JVM to get an environment context for this thread.
    JNIEnv* envPtr;
    jint rs = (*JvmPtr)->AttachCurrentThread(JvmPtr, (void**)&envPtr, NULL);
    LE_ASSERT(rs == JNI_OK);

    // Construct a Java message object wrapper for the handle we received.
    jobject messagePtr = ConstructObjectFromHandle(envPtr,
                                                   "io/legato/Message",
                                                   (jlong)(intptr_t)msgRef);
    if (messagePtr == NULL)
    {
        (*envPtr)->ExceptionDescribe(envPtr);
        return;
    }

    // Now call the handle method with this message object.
    jobject handlerObjPtr = (jobject)contextPtr;
    CallHandleMethod(envPtr, handlerObjPtr, "(Lio/legato/Message;)V", messagePtr);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Callback that will call a component's component init method.  This callback is used so that the
 *  component init method can be called from within the context of a Legato event loop.
 */
//--------------------------------------------------------------------------------------------------
static void InternalComponentInit
(
    void* componentPtr,  ///< [IN] The component object to be called.
    void* nothingPtr     ///< [IN] Not used here.
)
//--------------------------------------------------------------------------------------------------
{
    // Grab a reference to the JVM's environment for this thread.
    JNIEnv* envPtr;
    jint rs = (*JvmPtr)->AttachCurrentThread(JvmPtr, (void**)&envPtr, NULL);
    LE_ASSERT(rs == JNI_OK);

    // Find the component interface.
    jclass componentInterfacePtr = (*envPtr)->FindClass(envPtr, "io/legato/Component");
    if (componentInterfacePtr == NULL)
    {
        // TODO: Log exception.
        (*envPtr)->ExceptionDescribe(envPtr);
        (*envPtr)->ExceptionClear(envPtr);
        (*envPtr)->DeleteGlobalRef(envPtr, componentPtr);
        return;
    }

    // Find the init method on the component interface.
    jmethodID initMethodPtr = (*envPtr)->GetMethodID(envPtr,
                                                     componentInterfacePtr,
                                                     "componentInit",
                                                     "()V");

    if (initMethodPtr == NULL)
    {
        // TODO: Log exception.
        (*envPtr)->ExceptionDescribe(envPtr);
        (*envPtr)->ExceptionClear(envPtr);
        (*envPtr)->DeleteGlobalRef(envPtr, componentPtr);
        return;
    }

    // Finally call the init method, and free our reference to the object.
    (*envPtr)->CallVoidMethod(envPtr, componentPtr, initMethodPtr);

    if ((*envPtr)->ExceptionOccurred(envPtr) != NULL)
    {
        (*envPtr)->ExceptionDescribe(envPtr);
        (*envPtr)->ExceptionClear(envPtr);
    }

    (*envPtr)->DeleteGlobalRef(envPtr, componentPtr);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Construct a location value for returning a value and a size, (in bytes,) of that value.
 */
//--------------------------------------------------------------------------------------------------
static jobject NewLocationValue
(
    JNIEnv* envPtr,     ///< [IN] The Java environment to work out of.
    jobject parentPtr,  ///< [IN] Location value is a nested class of MessageBuffer, so it's
                        ///<      constructor can take a pointer to that parent object.
    jint byteSize,      ///< [IN] The size in bytes of the value in question.
    jstring valuePtr    ///< [IN] The value itself.
)
//--------------------------------------------------------------------------------------------------
{
    // Look up the class info, and find it's constructor.
    jclass classPtr = (*envPtr)->FindClass(envPtr, "io/legato/MessageBuffer$LocationValue");
    if (classPtr == NULL)
    {
        (*envPtr)->ExceptionDescribe(envPtr);
        (*envPtr)->ExceptionClear(envPtr);
        return NULL;
    }

    jmethodID constructorPtr =
                          (*envPtr)->GetMethodID(envPtr,
                                                 classPtr,
                                                 "<init>",
                                                 "(Lio/legato/MessageBuffer;ILjava/lang/Object;)V");
    if (constructorPtr == NULL)
    {
        (*envPtr)->ExceptionDescribe(envPtr);
        (*envPtr)->ExceptionClear(envPtr);
        return NULL;
    }

    // Now construct the new object, and call the constructor we picked out.
    return (*envPtr)->NewObject(envPtr, classPtr, constructorPtr, parentPtr, byteSize, valuePtr);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Init the C layer of the of the Legato interface.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_Init
(
    JNIEnv* envPtr,      ///< [IN] The Java environment to work out of.
    jclass callClassPtr  ///< [IN] The java class that called this function.
)
//--------------------------------------------------------------------------------------------------
{
    // Grab a reference to the JVM as it is not safe to cache the JNI environment pointer, and our
    // callbacks will need a way to access the Java environment in a safe manor.
    jint rs = (*envPtr)->GetJavaVM(envPtr, &JvmPtr);
    LE_ASSERT(rs == JNI_OK);

    // TODO: Cache required Java method IDs so that the various functions that need to call Java
    //       methods can be faster.

    // TODO: Convert to a JNI_OnUnload method?
}




//--------------------------------------------------------------------------------------------------
/**
 *  Given a pointer to a component object, schedule it's component init on this threads Legato
 *  event loop.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_ScheduleComponentInit
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jobject componentPtr  ///< [IN] The component we're calling.
)
//--------------------------------------------------------------------------------------------------
{
    // Queue the function, and acquire a longer term reference to the component.  Otherwise the
    // reference we are given can become invalid once this function returns.
    le_event_QueueFunction(InternalComponentInit,
                           (*envPtr)->NewGlobalRef(envPtr, componentPtr),
                           NULL);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Run the Legato event loop.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_RunLoop
(
    JNIEnv* envPtr,      ///< [IN] The Java environment to work out of.
    jclass callClassPtr  ///< [IN] The java class that called this function.
)
//--------------------------------------------------------------------------------------------------
{
    le_event_RunLoop();
}




//--------------------------------------------------------------------------------------------------
/**
 *  @return The size of a native pointer, either 4 or 8.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jint JNICALL Java_io_legato_LegatoJni_NativePointerSize
(
    JNIEnv* envPtr,      ///< [IN] The Java environment to work out of.
    jclass callClassPtr  ///< [IN] The java class that called this function.
)
//--------------------------------------------------------------------------------------------------
{
    return sizeof(void*);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Register a component with the Legato logging system.
 *
 *  @return: A log handle object to use with future logging requests.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jobject JNICALL Java_io_legato_LegatoJni_RegComponent
(
    JNIEnv* envPtr,         ///< [IN] The Java environment to work out of.
    jclass callClassPtr,    ///< [IN] The java class that called this function.
    jstring componentName   ///< [IN] Name of the component to register.
)
//--------------------------------------------------------------------------------------------------
{
    le_log_SessionRef_t logSession;
    le_log_Level_t* logLevelFilterPtr;

    const char* nameString = (*envPtr)->GetStringUTFChars(envPtr, componentName, NULL);

    size_t length = le_utf8_NumBytes(nameString) + 1;
    char* nameCopyStr = malloc(length);
    LE_ASSERT(le_utf8_Copy(nameCopyStr, nameString, length, NULL) == LE_OK);

    logSession = log_RegComponent(nameCopyStr, &logLevelFilterPtr);

    (*envPtr)->ReleaseStringUTFChars(envPtr, componentName, nameString);

    return NewLogHandle(envPtr, logSession, logLevelFilterPtr);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Connect to the log control daemon for log level updates.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_ConnectToLogControl
(
    JNIEnv* envPtr,      ///< [IN] The Java environment to work out of.
    jclass callClassPtr  ///< [IN] The java class that called this function.
)
//--------------------------------------------------------------------------------------------------
{
    log_ConnectToControlDaemon();
}




//--------------------------------------------------------------------------------------------------
/**
 *  Log a message to the legato event log.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_LogMessage
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong sessionRef,     ///< [IN] The session to report on.
    jlong logLevelRef,    ///< [IN] Ref to the log level variable.
    jint severity,        ///< [IN] The level of the log message.
    jstring file,         ///< [IN] File where the log was generated from.
    jstring method,       ///< [IN] Method the log was generated from.
    jint line,            ///< [IN] Line in the file the log was generated from.
    jstring message       ///< [IN] Message to be logged.
)
//--------------------------------------------------------------------------------------------------
{
    le_log_Level_t* nLogLevelPtr = (le_log_Level_t*)(intptr_t)logLevelRef;

    if (   (nLogLevelPtr != NULL)
        && (severity >= *nLogLevelPtr))
    {
        le_log_SessionRef_t nLogSessionRef = (le_log_SessionRef_t)(intptr_t)sessionRef;

        const char* nFilePtr = (*envPtr)->GetStringUTFChars(envPtr, file, NULL);
        const char* nMethodPtr = (*envPtr)->GetStringUTFChars(envPtr, method, NULL);
        const char* nMessagePtr = (*envPtr)->GetStringUTFChars(envPtr, message, NULL);

        _le_log_Send((le_log_Level_t)severity,
                     NULL,
                     nLogSessionRef,
                     nFilePtr,
                     nMethodPtr,
                     (unsigned int)line,
                     "%s",
                     nMessagePtr);

        (*envPtr)->ReleaseStringUTFChars(envPtr, file, nFilePtr);
        (*envPtr)->ReleaseStringUTFChars(envPtr, method, nMethodPtr);
        (*envPtr)->ReleaseStringUTFChars(envPtr, message, nMessagePtr);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Call the Legato function le_msg_GetProtocolRef.  This will get a reference to an existing
 *  protocol or create a new one if required.
 *
 *  @return A reference to the protocol in question.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jlong JNICALL Java_io_legato_LegatoJni_GetProtocolRef
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jstring jProtocolId,  ///< [IN] Name of the protocol.
    jint jLargestMsgSize  ///< [IN] The size of it's message buffer.
)
//--------------------------------------------------------------------------------------------------
{
    const char* protocolIdPtr = (*envPtr)->GetStringUTFChars(envPtr, jProtocolId, NULL);

    le_msg_ProtocolRef_t protocolRef = le_msg_GetProtocolRef(protocolIdPtr, jLargestMsgSize);
    (*envPtr)->ReleaseStringUTFChars(envPtr, jProtocolId, protocolIdPtr);

    return (jlong)(intptr_t)protocolRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the name of a protocol.
 *
 *  @return The ID of the protocol.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jstring JNICALL Java_io_legato_LegatoJni_GetProtocolIdStr
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong protocolRef     ///< [IN] The protocol object to read.
)
//--------------------------------------------------------------------------------------------------
{
    return (*envPtr)->NewStringUTF(envPtr,
                              le_msg_GetProtocolIdStr((le_msg_ProtocolRef_t)(intptr_t)protocolRef));
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the message size of a protocol.
 *
 *  @return The max message size of the protocol in question.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jint JNICALL Java_io_legato_LegatoJni_GetProtocolMaxMsgSize
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong protocolRef     ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    return (jint)le_msg_GetProtocolMaxMsgSize((le_msg_ProtocolRef_t)(intptr_t)protocolRef);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new client connection to a service.
 *
 *  @return A reference to the new Legato session.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jlong JNICALL Java_io_legato_LegatoJni_CreateSession
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong protocolRef,    ///< [IN] The session should use this protocol.
    jstring jServiceName  ///< [IN] The session should connect to this service.
)
//--------------------------------------------------------------------------------------------------
{
    const char* serviceNamePtr = (*envPtr)->GetStringUTFChars(envPtr, jServiceName, NULL);

    le_msg_SessionRef_t sessionRef =
                  le_msg_CreateSession((le_msg_ProtocolRef_t)(intptr_t)protocolRef, serviceNamePtr);

    (*envPtr)->ReleaseStringUTFChars(envPtr, jServiceName, serviceNamePtr);

    return (jlong)(intptr_t)sessionRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Close the session, disconnecting the client from the server.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_CloseSession
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong sessionRef      ///< [IN] The session we're closing. Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_CloseSession((le_msg_SessionRef_t)(intptr_t)sessionRef);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Free up the session.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_DeleteSessionRef
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong sessionRef      ///< [IN] The session to delete. Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_DeleteSession((le_msg_SessionRef_t)(intptr_t)sessionRef);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new message for a session.
 *
 *  @return A new message object for the session.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jlong JNICALL Java_io_legato_LegatoJni_CreateMessage
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong sessionRef      ///< [IN] The session we're creating a message on. Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    return (jlong)(intptr_t)le_msg_CreateMsg((le_msg_SessionRef_t)(intptr_t)sessionRef);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get a reference to the protocol the session is using.
 *
 *  @return A protocol reference.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jlong JNICALL Java_io_legato_LegatoJni_GetSessionProtocol
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong sessionRef      ///< [IN] The session object to read. Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    return (jlong)(intptr_t)le_msg_GetSessionProtocol((le_msg_SessionRef_t)(intptr_t)sessionRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Synchronously open a session with a service.  Blocks until the session is open or the attempt
 * is rejected.
 *
 * This function logs a fatal error and terminates the calling process if unsuccessful.
 *
 * @note    Only clients open sessions.  Servers' must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server do not agree on the maximum message size for the protocol,
 *          then an attempt to open a session between that client and server will result in a fatal
 *          error being logged and the client process being killed.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_OpenSessionSync
(
    JNIEnv* envPtr,         ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong sessionRef      ///< [IN] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_OpenSessionSync((le_msg_SessionRef_t)(intptr_t)sessionRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Sets the handler callback function to be called when the session is closed from the other
 * end.  A local termination of the session will not trigger this callback.
 *
 * The handler function will be called by the Legato event loop of the thread that created
 * the session.
 *
 * @note
 * - If this isn't set on the client side, the framework assumes the client is not designed
 *   to recover from the server terminating the session, and the client process will terminate
 *   if the session is terminated by the server.
 * - This is a client-only function.  Servers are expected to use le_msg_AddServiceCloseHandler()
 *   instead.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_SetSessionCloseHandler
(
    JNIEnv* envPtr,        ///< [IN] The Java environment to work out of.
    jclass callClassPtr,   ///< [IN] The java class that called this function.
    jlong sessionRef,      ///< [IN] The session we're registering on. Reference to the session.
    jobject handlerPtr     ///< [IN] The session event Java object that will handle the Java side of things.
)
//--------------------------------------------------------------------------------------------------
{
    // Make the local handler pointer global so that it remains valid after this function returns.
    le_msg_SetSessionCloseHandler((le_msg_SessionRef_t)(intptr_t)sessionRef,
                                  SessionEventHandler,
                                  (*envPtr)->NewGlobalRef(envPtr, handlerPtr));
}




//--------------------------------------------------------------------------------------------------
/**
 * Sets the receive handler callback function to be called when a non-response message arrives
 * on this session.
 *
 * The handler function will be called by the Legato event loop of the thread that created
 * the session.
 *
 * @note    This is a client-only function.  Servers are expected to use
 *          le_msg_SetServiceRecvHandler() instead.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_SetSessionReceiveHandler
(
    JNIEnv* envPtr,        ///< [IN] The Java environment to work out of.
    jclass callClassPtr,   ///< [IN] The java class that called this function.
    jlong sessionRef,      ///< [IN] Reference to the session.
    jobject handlerPtr     ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_SetSessionRecvHandler((le_msg_SessionRef_t)(intptr_t)sessionRef,
                                 SessionReceiveHandler,
                                 (*envPtr)->NewGlobalRef(envPtr, handlerPtr));
}




//--------------------------------------------------------------------------------------------------
/**
 * Fetches the user ID of the client at the far end of a given IPC session.
 *
 * @warning This function can only be called for the server-side of a session.
 *
 * @return LE_OK if successful.
 *         LE_CLOSED if the session has closed.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jint JNICALL Java_io_legato_LegatoJni_GetClientUserId
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong sessionRef      ///< [IN] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    uid_t userId;

    le_result_t result = le_msg_GetClientUserId((le_msg_SessionRef_t)(intptr_t)sessionRef, &userId);

    if (result != LE_OK)
    {
        // TODO: Generate exception.
    }

    return (jint)userId;
}




//--------------------------------------------------------------------------------------------------
/**
 * Fetches the user PID of the client at the far end of a given IPC session.
 *
 * @warning This function can only be called for the server-side of a session.
 *
 * @return LE_OK if successful.
 *         LE_CLOSED if the session has closed.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jint JNICALL Java_io_legato_LegatoJni_GetClientProcessId
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong sessionRef      ///< [IN] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    pid_t processId;

    le_result_t result = le_msg_GetClientProcessId((le_msg_SessionRef_t)(intptr_t)sessionRef,
                                                   &processId);

    if (result != LE_OK)
    {
        // TODO: Generate exception.
    }

    return (jint)processId;
}




//--------------------------------------------------------------------------------------------------
/**
 * Creates a service that is accessible using a protocol.
 *
 * @return  Service reference.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jlong JNICALL Java_io_legato_LegatoJni_CreateService
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong protocolRef,    ///< [IN] Reference to the protocol to be used.
    jstring jServiceName  ///< [IN] Service instance name.
)
//--------------------------------------------------------------------------------------------------
{
    const char* serviceNamePtr = (*envPtr)->GetStringUTFChars(envPtr, jServiceName, NULL);

    le_msg_ServiceRef_t serviceRef =
                  le_msg_CreateService((le_msg_ProtocolRef_t)(intptr_t)protocolRef, serviceNamePtr);

    (*envPtr)->ReleaseStringUTFChars(envPtr, jServiceName, serviceNamePtr);

    return (jlong)(intptr_t)serviceRef;
}




//--------------------------------------------------------------------------------------------------
/**
 * Deletes a service. Any open sessions will be terminated.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_DeleteService
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong serviceRef      ///< [IN] Reference to the service.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_DeleteService((le_msg_ServiceRef_t)(intptr_t)serviceRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called when clients open sessions with this service.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_AddServiceOpenHandler
(
    JNIEnv* envPtr,        ///< [IN] The Java environment to work out of.
    jclass callClassPtr,   ///< [IN] The java class that called this function.
    jlong serviceRef,      ///< [IN] Reference to the service.
    jobject handlerObjPtr  ///< [IN] The object that will handle the callback.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_AddServiceOpenHandler((le_msg_ServiceRef_t)(intptr_t)serviceRef,
                                 SessionEventHandler,
                                 (*envPtr)->NewGlobalRef(envPtr, handlerObjPtr));
}




//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_AddServiceCloseHandler
(
    JNIEnv* envPtr,        ///< [IN] The Java environment to work out of.
    jclass callClassPtr,   ///< [IN] The java class that called this function.
    jlong serviceRef,      ///< [IN] Reference to the service.
    jobject handlerObjPtr  ///< [IN] The object that will handle the callback.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_AddServiceCloseHandler((le_msg_ServiceRef_t)(intptr_t)serviceRef,
                                  SessionEventHandler,
                                  (*envPtr)->NewGlobalRef(envPtr, handlerObjPtr));
}




//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called when messages are received from clients via sessions
 * that they have open with this service.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_SetServiceReceiveHandler
(
    JNIEnv* envPtr,        ///< [IN] The Java environment to work out of.
    jclass callClassPtr,   ///< [IN] The java class that called this function.
    jlong serviceRef,      ///< [IN] Reference to the service.
    jobject handlerObjPtr  ///< [IN] The object that will handle the callback.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_SetServiceRecvHandler((le_msg_ServiceRef_t)(intptr_t)serviceRef,
                                 SessionReceiveHandler,
                                 (*envPtr)->NewGlobalRef(envPtr, handlerObjPtr));
}




//--------------------------------------------------------------------------------------------------
/**
 * Makes a given service available for clients to find.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_AdvertiseService
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong serviceRef      ///< [IN] Reference to the service.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_AdvertiseService((le_msg_ServiceRef_t)(intptr_t)serviceRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Makes a specified service unavailable for clients to find without terminating any ongoing
 * sessions.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_HideService
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong serviceRef      ///< [IN] Reference to the service.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_HideService((le_msg_ServiceRef_t)(intptr_t)serviceRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Creates a message to be sent over a given session.
 *
 * @return  Message reference.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jlong JNICALL Java_io_legato_LegatoJni_CreateMsg
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong sessionRef      ///< [IN] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    return (jlong)(intptr_t)le_msg_CreateMsg((le_msg_SessionRef_t)(intptr_t)sessionRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Adds to the reference count on a message object.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_AddMessageRef
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef      ///< [IN] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_AddRef((le_msg_MessageRef_t)(intptr_t)messageRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Releases a message object, decrementing its reference count.  If the reference count has reached
 * zero, the message object is deleted.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_ReleaseMessage
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef      ///< [IN] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_ReleaseMsg((le_msg_MessageRef_t)(intptr_t)messageRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Sends a message.  No response expected.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_Send
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef      ///< [IN] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_Send((le_msg_MessageRef_t)(intptr_t)messageRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Requests a response from a server by sending it a request.  Blocks until the response arrives
 * or until the transaction terminates without a response (i.e., if the session terminates or
 * the server deletes the request without responding).
 *
 * @return  Reference to the response message, or NULL if the transaction terminated without a
 *          response.
 *
 * @note
 *        - To prevent deadlocks, this function can only be used on the client side of a session.
 *          Servers can't use this function.
 *        - To prevent race conditions, only the client thread attached to the session
 *          (the thread that created the session) is allowed to perform a synchronous
 *          request-response transaction.
 *
 * @warning
 *        - The calling (client) thread will be blocked until the server responds, so no other
 *          event handling will happen in that client thread until the response is received (or the
 *          server dies).  This function should only be used when the server is certain
 *          to respond quickly enough to ensure that it will not cause any event response time
 *          deadlines to be missed by the client.  Consider using le_msg_RequestResponse()
 *          instead.
 *        - If this function is used when the client and server are in the same thread, then the
 *          message will be discarded and NULL will be returned.  This is a deadlock prevention
 *          measure.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jlong JNICALL Java_io_legato_LegatoJni_RequestSyncResponse
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef      ///< [IN] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    return (jlong)(intptr_t)le_msg_RequestSyncResponse((le_msg_MessageRef_t)(intptr_t)messageRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Sends a response back to the client that send the request message.
 *
 * Takes a reference to the request message.  Copy the response payload (if any) into the
 * same payload buffer that held the request payload, then call le_msg_Respond().
 *
 * The messaging system will delete the message automatically when it's finished sending
 * the response.
 *
 * @note    Function can only be used on the server side of a session.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_Respond
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef      ///< [IN] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_Respond((le_msg_MessageRef_t)(intptr_t)messageRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a message requires a response or not.
 *
 * @note    This is intended for use on the server side only.
 *
 * @return
 *  - TRUE if the message needs to be responded to using le_msg_Respond().
 *  - FALSE if the message does not need to be responded to, and should be disposed of using
 *    le_msg_ReleaseMsg() when it is no longer needed.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jboolean JNICALL Java_io_legato_LegatoJni_NeedsResponse
(
    JNIEnv* envPtr,         ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef      ///< [IN] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    if (le_msg_NeedsResponse((le_msg_MessageRef_t)(intptr_t)messageRef))
    {
        return JNI_TRUE;
    }

    return JNI_FALSE;
}




//--------------------------------------------------------------------------------------------------
/**
 * Gets the size, in bytes, of the message payload memory buffer.
 *
 * @return The size, in bytes.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jint JNICALL Java_io_legato_LegatoJni_GetMaxPayloadSize
(
    JNIEnv* envPtr,         ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef      ///< [IN] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    return (jint)le_msg_GetMaxPayloadSize((le_msg_MessageRef_t)(intptr_t)messageRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to the session to which a given message belongs.
 *
 * @return Session reference.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jlong JNICALL Java_io_legato_LegatoJni_GetSession
(
    JNIEnv* envPtr,         ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef      ///< [IN] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    return (jlong)(intptr_t)le_msg_GetSession((le_msg_MessageRef_t)(intptr_t)messageRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Fetches a received file descriptor from the message.
 *
 * @return A Java FileDescritpor object.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jobject JNICALL Java_io_legato_LegatoJni_GetMessageFd
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef      ///< [IN] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    int fd = le_msg_GetFd((le_msg_MessageRef_t)(intptr_t)messageRef);

    return CreateFileDescriptor(envPtr, fd);
}




//--------------------------------------------------------------------------------------------------
/**
 * Sets the file descriptor to be sent with this message.
 *
 * This file descriptor will be closed when the message is sent (or when it is deleted without
 * being sent).
 *
 * At most one file descriptor is allowed to be sent per message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_SetMessageFd
(
    JNIEnv* envPtr,         ///< [IN] The Java environment to work out of.
    jclass callClassPtr,    ///< [IN] The java class that called this function.
    jlong messageRef,       ///< [IN] Reference to the message.
    jobject fileDescriptor  ///< [IN] A file descriptor object.
)
//--------------------------------------------------------------------------------------------------
{
    int fd = ExtractFd(envPtr, fileDescriptor);

    le_msg_SetFd((le_msg_MessageRef_t)(intptr_t)messageRef, fd);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Read a boolean value from the message.
 *
 *  @return A boolean read from the message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jboolean JNICALL Java_io_legato_LegatoJni_GetMessageBool
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition   ///< [IN] Where in the message buffer to read the value from.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    bool test = *((bool*)&msgBufferPtr[bufferPosition]);

    if (test)
    {
        return JNI_TRUE;
    }

    return JNI_FALSE;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value into the message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_SetMessageBool
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition,  ///< [IN] Where in the message buffer to read the value from.
    jboolean value        ///< [IN] The value to write.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    *((jshort*)&msgBufferPtr[bufferPosition]) = value == JNI_FALSE ? false : true;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Read a single byte from the message.
 *
 *  @return The byte read from the message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jbyte JNICALL Java_io_legato_LegatoJni_GetMessageByte
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition   ///< [IN] Where in the message buffer to read the value from.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    return (jbyte)msgBufferPtr[bufferPosition];
}




//--------------------------------------------------------------------------------------------------
/**
 *  Write a byte into the message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_SetMessageByte
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition,  ///< [IN] Where in the message buffer to read the value from.
    jbyte value           ///< [IN] The value to write.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    msgBufferPtr[bufferPosition] = (uint8_t)value;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Read a two byte short from the message.
 *
 *  @return The short value read from the message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jshort JNICALL Java_io_legato_LegatoJni_GetMessageShort
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition   ///< [IN] Where in the message buffer to read the value from.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    return *((jshort*)&msgBufferPtr[bufferPosition]);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Write a two byte short into a message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_SetMessageShort
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition,  ///< [IN] Where in the message buffer to read the value from.
    jshort value          ///< [IN] The value to write.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    *((jshort*)&msgBufferPtr[bufferPosition]) = value;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Read a 4 byte integer from the message.
 *
 *  @return An integer read from the message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jint JNICALL Java_io_legato_LegatoJni_GetMessageInt
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition   ///< [IN] Where in the message buffer to read the value from.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    return *((jint*)&msgBufferPtr[bufferPosition]);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Write a 4 byte integer into the message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_SetMessageInt
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition,  ///< [IN] Where in the message buffer to read the value from.
    jint value            ///< [IN] The value to write.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    *((jint*)&msgBufferPtr[bufferPosition]) = value;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Read an 8 byte long from the message.
 *
 *  @return The long value read from the message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jlong JNICALL Java_io_legato_LegatoJni_GetMessageLong
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition   ///< [IN] Where in the message buffer to read the value from.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    return *((jlong*)&msgBufferPtr[bufferPosition]);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Write an 8 byte long value into a message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_SetMessageLong
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition,  ///< [IN] Where in the message buffer to read the value from.
    jlong value           ///< [IN] The value to write.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    *((jlong*)&msgBufferPtr[bufferPosition]) = value;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Read an 8 byte floating point value from the message.
 *
 *  @return The double value read from the message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jdouble JNICALL Java_io_legato_LegatoJni_GetMessageDouble
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition   ///< [IN] Where in the message buffer to read the value from.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    return *((jdouble*)&msgBufferPtr[bufferPosition]);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Write an 8 byte double into a message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_SetMessageDouble
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition,  ///< [IN] Where in the message buffer to read the value from.
    jdouble value         ///< [IN] The value to write.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    *((jdouble*)&msgBufferPtr[bufferPosition]) = value;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Read a string from the message.  Return a string and the number of bytes actually read.
 *
 *  @return An io.legato.MessageBuffer.LocationValue that holds the string and number of bytes read
 *          from the message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jobject JNICALL Java_io_legato_LegatoJni_GetMessageString
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The Java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition   ///< [IN] Where in the message buffer to read the value from.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    char* msgBufferPtr = (char*)le_msg_GetPayloadPtr(nRef) + bufferPosition;

    uint32_t strSize;
    const uint32_t sizeOfStrSize = sizeof(strSize);

    memcpy(&strSize, msgBufferPtr, sizeOfStrSize);
    msgBufferPtr += sizeOfStrSize;

    jstring newStr;

    if (strSize > 0)
    {
        // TODO: Check for realistic size.
        char* charBuffer = malloc(strSize + 1);
        if (!charBuffer)
        {
           return 0;
        }

        memcpy(charBuffer, msgBufferPtr, strSize);
        charBuffer[strSize] = 0;

        newStr = (*envPtr)->NewStringUTF(envPtr, charBuffer);

        free(charBuffer);
    }
    else
    {
        newStr = (*envPtr)->NewStringUTF(envPtr, "");
    }

    return NewLocationValue(envPtr,
                            NULL,
                            strSize + sizeOfStrSize,
                            newStr);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Write a string value into a message.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jint JNICALL Java_io_legato_LegatoJni_SetMessageString
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition,  ///< [IN] Where in the message buffer to read the value from.
    jstring value,        ///< [IN] The value to write.
    jint maxByteSize      ///< [IN] The maximum string size to write to the buffer.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    const char* rawStrPtr = (*envPtr)->GetStringUTFChars(envPtr, value, NULL);

    if (rawStrPtr == NULL)
    {
        return 0;
    }

    msgBufferPtr += bufferPosition;

    uint32_t strSize = strlen(rawStrPtr);
    const uint32_t sizeOfStrSize = sizeof(strSize);

    // Always pack the string size first, and then the string itself.
    memcpy(msgBufferPtr, &strSize, sizeOfStrSize);
    msgBufferPtr += sizeOfStrSize;

    memcpy(msgBufferPtr, rawStrPtr, strSize);

    return sizeOfStrSize + strSize;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Read a reference value from the message.
 *
 *  @note References are system dependent, (either 32 or 64 bits.)  However Java is system agnostic,
 *        so we always convert a reference into a long, but we only use the appropriate amount of
 *        bytes from the buffer.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT jlong JNICALL Java_io_legato_LegatoJni_GetMessageLongRef
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition   ///< [IN] Where in the message buffer to read the value from.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    return *((intptr_t*)&msgBufferPtr[bufferPosition]);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Write a Legato reference to a message.
 *
 *  @note References are system dependent, (either 32 or 64 bits.)  However Java is system agnostic,
 *        so we always read a reference from a long, but we only use the appropriate amount of bytes
 *        from the buffer.
 */
//--------------------------------------------------------------------------------------------------
JNIEXPORT void JNICALL Java_io_legato_LegatoJni_SetMessageLongRef
(
    JNIEnv* envPtr,       ///< [IN] The Java environment to work out of.
    jclass callClassPtr,  ///< [IN] The java class that called this function.
    jlong messageRef,     ///< [IN] Reference to the message.
    jint bufferPosition,  ///< [IN] Where in the message buffer to read the value from.
    jlong value           ///< [IN] The value to write.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t nRef = (le_msg_MessageRef_t)(intptr_t)messageRef;
    uint8_t* msgBufferPtr = (uint8_t*)le_msg_GetPayloadPtr(nRef);

    *((intptr_t*)&msgBufferPtr[bufferPosition]) = value;
}
