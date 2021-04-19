/**
 * @file le_rpcProxyFileStream.h
 *
 * Header for rpc Proxy file stream feature. <br>
 * @ref rpcfstream_workflow <br>
 * @ref rpcfstream_flags <br>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "le_rpcProxy.h"
#include "le_rpcProxyNetwork.h"

//--------------------------------------------------------------------------------------------------
/**
 *
 * @section rpcfstream_workflow Workflow
 *
 * Wherever an ipc message is being converted to an rpc message,
 * @ref rpcFStream_HandleFileDescriptor checks the ipc message for any file descriptor to send.
 * If a file descriptor is found, a @ref rpcProxy_FileStream_t instance is created to represent and
 * hold that file descriptor and related information like direction. The file Stream id and its
 * flags are then embedded into the metadata of the rpc message being generated.
 * On the receiving side, whenever an rpc message is received, @ref rpcFStream_HandleStreamId scans
 * its metadata for any stream id, if a valid filestream id and its flag are found, a filestream
 * instance is created based on that.
 * After this step, both sides will have their corresponding file stream instances. Then the side
 * with INCOMING direction will request as much data as its local file descriptor can accept. The
 * OUTGOING side will then try to transmit the same amount of data if available.
 *
 * Some important concepts to keep in mind when following the below diagram:
 *
 * 1. File stream owner:
 * The owner of a file stream is the side that first creates it. Ownership is independent of the
 * client server roles. If the file descriptor was an input argument, IN in the api file, it will be
 * first seen in an IPC message by the client, making the client the owner. If the file descriptor
 * was an output argument, OUT in the api file, it will be first seen by the server,  making the
 * server the owner.
 *
 * 2. File stream direction:
 * A file stream's direction is either INCOMING or OUTGOING. Bidirectional file streams are not
 * supported at this point. This property is independent of the client-server roles and also of the
 * owner flag. File stream direction represents the direction at which data flows and is determined
 * by the owner using file descriptor's permission flags. If the owner finds a read-only file
 * descriptor in the IPC message, it will create an OUTGOING file descriptor. If the owner finds a
 * write-only file descriptor in the IPC message, it will create an INCOMING file descriptor.
 *
 * The owner will the set the direction in the initialization flags from its own perspective.
 * The other side will inversely reciprocate the direction when creating its own instance of the
 * file stream. In other words, an OUTGOING file stream instance in the owner system will be matched
 * with an INCOMING file stream instance in the other side and vice versa.
 *
 * 3. Stream ID:
 * Each side holds a file descriptor, named rpcFd in the below diagram. This value is not
 * transmitted in any RPC message. Instead, each file stream has an ID that is used to refer to a
 * certain file stream instance by both sides.
 *
 *
 * Follow the diagram below for more detail:
@verbatim

                                     +
                                     |
                                     |
                                     |
  Client             rpcServer       |       rpcClient                Server
   localFd                           |
     +                               |
     | file descriptor               |
     +---------------->  rpcFd       |
                   +-----------------+
                   |Determine stream||
                   |direction       ||
                   +-----------------|
                   +--------v--------|
                   |Create File     ||
                   |Stream instance ||
                   +-----------------|
                   +--------v--------|
                   |Create fdMonitor||
                   |on rpcFd        ||
                   +-----------------|
                   +--------v--------|
                   |Enable fdMonitor||
                   |if INCOMING     ||
                   |look for POLLOUT||
                   +-----------------|
                   +--------v--------|
                   |Set rpcProxyMsg ||
                   |StreamId & flags||
                   +--------+--------+
                            |        |
                            +--------------v StreamID & Flags
                                     |--------------------+
                                     ||Determine direction|
                                     ||from flags         |
                                     |--------------------+
                                     |----------v---------+
                                     ||Create matching    |
                                     ||FileStream instance|
                                     |--------------------+
                                     |----------v---------+
                                     || Create fifo       |
                                     |--------------------+
                                     |----------v---------+
                                     ||Open fifo twice:   |
                                     ||localFd and rpcFd  |
                                     |--------------------+
                                     |----------v---------+
                                     ||Create fdMonitor   |
                                     ||on rpcFd           |
                                     |--------------------+
                                     |----------v---------+
                                     ||Enable fdMonitor   |
                                     ||if INCOMING        |
                                     ||look for POLLOUT   |
                                     |--------------------+
                                     |----------v---------+
                                     ||set localFd in ipc |
                                     ||message reference  |
                                     +----------+---------+
                                     |          |                   localFd
                                     |          +------------------------>
                                     |
                        From here we assume stream
                        direction is from client
                        to Server, so OUTGOING for
                        client and INCOMING for server
                                     |
                                     |
                                     +---------------------------+
                                     || fdMonitor handle called  |
                                     || for rpcFd with POLLOUT   |<---+
                                     |---------------------------+    |
                                     |-------------v-------------+    |
                                     ||Find current capacity of  |    |
                                     ||rpcFd fifo:               |    |
                                     ||reqSize=PIPE_SIZE-FIONREAD|    |
                                     |---------------------------+    |
                                     |-------------v-------------+    |
                                     ||Send FSTREAM_REQUEST_DATA |    |
                                     ||message and request       |    |
                                     ||reqSize bytes             |    |
                                     |---------------------------+    |
                                     |-------------v-------------+    |
                                     ||Disable this fdMonitor    |    |
                                     +-------------+-------------+    |
                  requesting reqSize |             |                  |
                           +-----------------------+                  |
               +-----------v---------|                                |
               | Store reqSize in   ||                                |
               | fileStream instance||                                |
               +---------------------|                                |
               +----------v----------|                                |
               |Enable fdMonitor of ||                                |
               |rpcFd for POLLIN    ||                                |
               +---------------------+                                |
                                     |                                |
   Writes some                       |                                |
  data to localFd                    |                                |
                                     |                                |
               +---------------------+                                |
               |fdMon handle called ||                                |
               |for rpcFd w/ POLLIN ||                                |
               +---------------------+                                |
               +----------v----------|                                |
               |read minimum of     ||                                |
               |MAX_RPC_MSG_SIZE and||                                |
               |reqSize bytes       ||                                |
               +---------------------|                                |
               +----------v----------|                                |
               |reqSize -= number of||                                |
               |bytes read          ||                                |
               +---------------------|                                |
               +----------v----------|                                |
               |Create msg with:    ||                                |
               |FSTREAM_DATA_PACKET ||                                |
               +---------------------|                                |
               +----------v----------|                                |
               |if reqSize==0:      ||                                |
               |disable this monitor||                                |
               +---------------------|                                |
               +----------v----------|                                |
               |if reached EOF set  ||                                |
               |FSTREAM_EOF flag    ||                                |
               |and clean fileStream||                                |
               +----------+----------+                                |
                          |          |                                |
                          +------------------v                        |
                                     |-------------------------+      |
                                     ||Write data into rpcFd   |      |
                                     |-------------------------+      |
                                     |-------------v-----------+      |
                                     ||successful and no EOF:  |      |
                                     ||enable fdMon again and  |------+
                                     ||ask for more            |
                                     |-------------------------+
                                     |-------------v-----------+
                                     ||successful and EOF:     |
                                     ||close rpcFd and clean   |
                                     ||file Stream             |
                                     |-------------------------+
                                     |-------------v-----------+
                                     ||if failed:              |
                                     || close rpcFd            |
                                     || clean fileStream       |
                                     || send rpc fileStream msg|
                                     ||with FSTREAM_FORCE_CLOSE|
                                     +------------+------------+
              FSTREAM_FORCE_CLOSE    |            |
                           +----------------------+
            +--------------v----+    |
            | close rpcFd       |    |
            | clean fileStream  |    |
            +-------------------+    |
@endverbatim
 */
//--------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------
/**
 * @section rpcfstream_flags RPC file stream flags
 *
 * RPC file stream flag format as it is sent:
@verbatim
    +----------+-----+--------+-------+------+------+------+----------+----------+------+
    |   9-15   |  8  |   7    |   6   |  5   |  4   |  3   |    2     |    1     |   0  |
    ------------------------------------------------------------------------------------+
    | Reserved | I/O |NonBlock|Request| Data |Force |EOF on|Initialize|Initialize| Owner|
    |          |Error|Local Fd| Data  |Packet|Close |origin| Outgoing | Incoming |  Bit |
    |          |     |        | Packet|      |Stream|      |  Stream  |  Stream  |      |
    +----------+-----+--------+----------------------------+----------+----------+------+
@endverbatim

 * @subsection rpcfstream_initflag Initialization flags
 *   "F[1] , F[2], and F[7] are the flags that are present in the rpc proxy message that carries the
 *   file stream. F[1] and F[2] are from the perspective of the sender.
 *
 * @subsection rpcfstream_ownerflag Owner flag(F[0])
 *   The owner of a file stream is the system that created the file stream instance first. This
 *   happens in rpcFStream_HandleFileDescriptor. This flag will be stored with the file stream and
 *   must be valid in all future communications regarding this file stream instance. In following
 *   messages, each system must set the owner flag from their own perspective mean that the owner
 *   always sets the owner bit and the other side always sends its message with owner bit cleared.
 *
 *   Please note that ownership of a file stream is independent form its direction. A file stream's
 *   direction is set according to the flow of data. A system may create and send an incoming file
 *   stream to receive some data from the other side or it may create and send an outgoing file
 *   stream to send some data to the other side. In both of these cases the first system would be
 *   owner of said file stream.
 *
 *   There are two functions that create new file stream instances,
 *   @ref rpcFStream_HandleFileDescriptor and @ref rpcFStream_HandleStreamId. All file streams
 *   created by @ref rpcFStream_HandleFileDescriptor are owned by us(the system creating the file
 *   stream) and all file streams created by @ref rpcFStream_HandleStreamId are not owned by us.
 *
 *   The primary use of the owner flag is to prevent collision when two systems create two different
 *   file streams with the same Id at the same time.
 *
 * @subsection rpcfstream_eofflag EOF on origin(F[3])
 *   The outgoing side seen EOF it's rpcFd.
 *
 * @subsection rpcfstream_forceflag Force Close Stream(F[4])
 *   We must close this stream due to some error on the other side.
 *
 * @subsection rpcfstream_dataflag Data Packet(F[5])
 *   This file stream message contains data. Data is in the payload and packet as byte array.
 *
 * @subsection rpcfstream_requestflag Request Data Packet(F[6])
 *   This file stream message contains a size value which represents that maximum that the other
 *   side can receive right now. Requested size is packed as 32 bit unsigned integer.
 *
 * @subsection rpcfstream_ioerrorflag I/O Error(F[8])
 *   An error has happened during read or write to rpcFd.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_FSTREAM_OWNER                           0x1
#define RPC_FSTREAM_INIT_INCOMING                   0x2
#define RPC_FSTREAM_INIT_OUTGOING                   0x4
#define RPC_FSTREAM_EOF                             0x8
#define RPC_FSTREAM_FORCE_CLOSE                     0x10
#define RPC_FSTREAM_DATA_PACKET                     0x20
#define RPC_FSTREAM_REQUEST_DATA                    0x40
#define RPC_FSTREAM_NONBLOCK                        0x80
#define RPC_FSTREAM_IOERROR                         0x100


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of digits for file streamid. used in created fdmon names and fifo paths.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_FSTREAM_ID_MAX_DIGITS  5

//--------------------------------------------------------------------------------------------------
/**
 * RPC enum for direction of file stream.
 */
//--------------------------------------------------------------------------------------------------
typedef enum FStream_direction
{
    INCOMING_FILESTREAM = 1,
    OUTGOING_FILESTREAM = 2,
    BIDIRECTIONAL_FILESTREAM = 3 // not supported yet.
} fStream_direction_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Stream element.
 *
 * @note: A unique file stream is identified by its streamId, the owner flag, and the remote system
 * name.
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_FileStream
{
    uint16_t streamId;                         ///< stream id only used by rpcProxy
    bool owner;                                ///< true If this stream was created in the current
                                               /// system, false otherwise
    int32_t rpcFd;                             ///< Fd associated with this stream on the rpc side
    const char* remoteSystemName;              ///< name of remote system
    uint32_t serviceId;                        ///< service Id
    le_fdMonitor_Ref_t fdMonitorRef;           ///< reference to fd monitor for rpcfd
    size_t requestedSize;                      ///< free buffer of the other side.
    fStream_direction_t direction;             ///< stream direction.

    le_dls_Link_t  link;
} rpcProxy_FileStream_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy File Stream Function prototypes
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Handle the embedded file descriptor of an le_msg_MessageRef_t
 *
 * @return
 *      - LE_OK if successfully handled file descriptor.
 *      - LE_NO_MEMORY a memory pool was full.
 *      - LE_FAULT for any other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_HandleFileDescriptor
(
    le_msg_MessageRef_t msgRef,               ///< [IN] ipc message reference
    rpcProxy_MessageMetadata_t* metaDataPtr,  ///< [IN] pointer to proxy message meta data
    uint32_t serviceId,                       ///< [IN] service id
    const char* systemName                    ///< [IN] name of system message is being prepared for
);

//--------------------------------------------------------------------------------------------------
/**
 * Handle the embedded stream in an rpc proxy message
 *
 * @return
 *      - LE_OK if handled properly and without error.
 *      - LE_NO_MEMORY ran into memory issue while handing stream id
 *      - LE_FAULT any other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_HandleStreamId
(
    le_msg_MessageRef_t msgRef,               ///< [IN] ipc message reference
    rpcProxy_MessageMetadata_t* metaDataPtr,  ///< [IN] pointer to proxy message meta data
    uint32_t serviceId,                       ///< [IN] service id
    const char* systemName                    ///< [IN] name of system message was received on
);

//--------------------------------------------------------------------------------------------------
/**
 * Process a file Stream message
 *
 * @return
 *      - LE_OK if message was processed without error
 *      - LE_FAULT an error happened during processing message
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_ProcessFileStreamMessage
(
    void* handle,                     ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,           ///< [IN] name of system this message was received from
    StreamState_t* streamStatePtr,    ///< [IN] Pointer to the Stream State-Machine data
    void* proxyMessagePtr             ///< [IN] pointer to rpc message received
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a local channel with two file descriptors, one for rpcFd one for localFd
 * Depending on the platform, channel may be a fifo or a pipe.
 *
 * @return
 *      - LE_OK if channel was created successfully.
 *      - LE_FAULT error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_CreateChannel
(
    rpcProxy_FileStream_t* fileStreamRef,  ///< [IN] Pointer to file stream instance
    int32_t* rpcFdPtr,                     ///< [OUT] rpcFd
    int32_t* localFdPtr,                   ///< [OUT] localFd
    bool isLocalFdNonBlocking              ///< [IN] boolean determining whether localfd should be
                                           /// non blocking
);

//--------------------------------------------------------------------------------------------------
/**
 * Get space available on the channel.
 *
 * This will represent the number of bytes that can be written to rpcFd without being blocked
 *
 * @return
 *      - LE_OK if available space was read successfully.
 *      - LE_FAULT any error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_GetAvailableSpace
(
    rpcProxy_FileStream_t* fileStreamRef,  ///< [IN] Pointer to file stream instance
    uint32_t* spaceAvailablePtr            ///< [OUT] Pointer to available space
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete a specific streamid id that is owned by us if it's valid
 */
//--------------------------------------------------------------------------------------------------
void rpcFStream_DeleteOurStream
(
    uint16_t fStreamId,              ///< [IN] id of the file stream instance to be deleted
    const char* systemName           ///< [IN] The system that of it's file stream is to be deleted
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete all file streams associated with a system
 */
//--------------------------------------------------------------------------------------------------
void rpcFStream_DeleteStreamsBySystemName
(
    const char* systemName          ///< [IN] The system that of it's file stream are to be deleted
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete all file streams associated with a service Id:
 */
//--------------------------------------------------------------------------------------------------
void rpcFStream_DeleteStreamsByServiceId
(
    uint32_t serviceId              ///< [IN] Id of service being closed.
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize file stream pool:
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_InitFileStreamPool
(
    void
);
