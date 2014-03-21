Embedded Micro Protocol (EMP)
========================================

#### 1. Presentation

On the embedded target, several processes (tasks/threads/modules/etc,
depending on the target) are running and providing simple services.

The Agent and the different clients (assets) need to exchange data.
This micro protocol specifies the way of exchanging those data.

The communication is done using a standard IPC, i.e. a socket on
localhost. However the protocol does not require that socket must be
used, any transport layer that provide the same level of features can be
used.\
 On a linux target, the Agent proxy opens a socket on localhost,
default port is 9999. The client can connect to it when it needs to send
data or be able to receive data from a remote server.

#### 2. Frame format

All Commands expect a Response. The Response has the same command id and
the same request id, but the bit0 of the status byte is set to 1.

Commands and Responses can be interleaved, and a maximum of **256** Commands
(same or different ones) can be running at the same time.

A frame (Command or Response) is a set of bytes, composed of a header
followed by a payload.

##### 2.1. Header format

 Header is composed of 8 bytes:
 
------------------------------------------------------------------------------------
Frame section                    Size and description
-------------                    ---------------------------------------------------
command id                       2 bytes: Command id (See #List of commands)

type                             1 byte:\
                                  - bit0: 0-command, 1-response\
                                  - bit1-7: reserved (set to 0)

requestid                        1 byte: unsigned integer to code command 
                                 request id

payloadsize                      4 bytes: BigEndian coded unsigned integer.\ 
                                 Size of the payload in bytes
------------------------------------------------------------------------------------

\ 

##### 2.2. Command payload format

Command payload is encoded in JSON:

------------------------------------------------------------------------------------
Frame section                    Size and description
-------------                    ---------------------------------------------------
payload                          payload of '`payloadsize`' bytes of JSON
------------------------------------------------------------------------------------

\ 

##### 2.3. Response payload format

Response Payload is composed of bytes which may be interpreted
according to the 'command' field from the header, however it contains at
least 2 bytes that are interpreted as the status of the response:

------------------------------------------------------------------------------------
Frame section                    Size and description
-------------                    ---------------------------------------------------
status                           2 bytes: BigEndian coded signed short integer. \
                                   - if status == 0 the command went OK, \ 
                                   - if status != 0 an error occurred. \
                                 The status value is to be interpreted as a \ 
                                 [rc_ReturnCode_t](../c/returncodes_8h.html) type.

payload                          payload '`payloadsize-2`' bytes of JSON
------------------------------------------------------------------------------------

\ 

#### 3.  List of commands and their respective payloads.


Vocabulary:

* App: stands for Application
* Agt: stands for (Mihini) Agent

--------------------------------------------------------------------------------------------------------------------------------------------------------------
ID            Command ID                   Request Direction        Description
--            ----------                   -----------------        ------------------------------------------------------------------------------------------
1             SendData                     Agt-\>App                Data coming from server.\
                                                                    The payload contains the message (containing metadata and regular data) 
                                                                    sent by the server.\
                                                                     \
                                                                    **Command payload**: \
                                                                    The message is a JSON `map` which format is:\
                                                                     - `Path`: a `string` identifying the root path of all variables contained in this message.\
                                                                     - `TicketId`: a `number` that serves as a acknowledge request for this message.
                                                                       The `TicketId` value must be given in `PAcknowledge` EMP command to acknowledge the message 
                                                                       to the server.\
                                                                     - `Body`: variables sent by the server, the data is in a `map` which format is:\
                                                                        -- `variable_name` = `variable_value` : `variable_name` is the variable sub path and name relative
                                                                          to `Path`.\
                                                                    \
                                                                    **Response payload**: \
                                                                     - `status`: 2 bytes acknowledging the command.
                                           
2             Register                     App-\>Agt                 Register client asset/service to the Agent \
                                                                     \
                                                                     **Command payload**: \
                                                                     - the path for which this client want to receive
                                                                       messages (a JSON string). \
                                                                     \
                                                                     **Response payload**: \
                                                                      - `status`: 2 bytes acknowledging the command. \
                                                                     \
                                                                     **Remarks**:\
                                                                     - It is possible to register several assets for the same IPC\
                                                                        connection by sending several Register commands from that IPC.\
                                                                     - When the IPC is closed, the asset is unregistered automatically.\
                                                                     - If several assets were registered, closing IPC will unregister all\
                                                                       the assets linked to that IPC.
                                           
3             Unregister                   App-\>Agt                  Unregisters a path previously registered with "Register".
                                                                      \
                                                                     **Command payload**: \
                                                                     - the path to unregister (a JSON string).\
                                                                     \
                                                                     **Response payload**: \
                                                                      - `status`: 2 bytes acknowledging the command. \
                                           
4             ConnectToServer              App-\>Agt                  Forces the agent to connect to the server \
                                                                      The connection is made synchronously if no latency is specified (i.e.
                                                                     the connection is finished when response is sent), otherwise when \
                                                                      a latency is specified the connection is likely to happen after the
                                                                     response is sent. \
                                                                      \
                                                                      **Command payload**: \ 
                                                                      - optional positive integer: the latency in seconds 
                                                                     before executing the connect to server action. If no latency specified,
                                                                     the connexion is made synchronously.\
                                                                     \
                                                                      **Response payload**: \
                                                                      - `status`: 2 bytes acknowledging the command.
                                                                     The response has a status == 0 when no latency was specified and the
                                                                     connection to the server was successful. \
                                                                      If `latency` was specified, then status==0 means that the connection
                                                                     request was successfully scheduled
                                           
7              RegisterSMSListener         App-\>Agt                  Register a client as a SMS listener to the Agent. \
                                                                      \
                                                                      **Command payload**: a `list` of two objects: \
                                                                      - `String`: Phone number pattern to listen to. \
                                                                      - `String`: Message content pattern to listen to. \
                                                                      \
                                                                      **Response payload**: \
                                                                      - `status`: 2 bytes acknowledging the command. \
                                                                      if the status is OK (=0) \
                                                                      - `id`: an unsigned integer, giving the registration id, identifying the
                                                                     sms listening registration for the combination: (sms module
                                                                     instance/phone pattern/message pattern). This is the id to be used to
                                                                     call UnregisterSMSListener command. \
                                                                      \
                                                                      **Note**: Patterns syntax should conformed to [Lua
                                                                     patterns](http://www.lua.org/manual/5.1/manual.html#5.4.1)

51             UnregisterSMSListener       App-\>Agt                  Unregister a client as a SMS listener to the Agent. \
                                                                      \
                                                                      **Command payload**: \
                                                                      - `id`: an unsigned integer, the registration id to unregister, as returned
                                                                     by RegisterSMSListener command \
                                                                      \
                                                                      **Response payload**: \
                                                                      - `status`: 2 bytes acknowledging the command.

8              NewSMS                      Agt-\>App                  Notify an application that a SMS is received \
                                                                     \
                                                                      **Command payload:** a `list` of 3 objects: \
                                                                       - `String`: Phone number of the sender (for incoming messages) / recipient
                                                                     (for outgoing messages) \
                                                                       - `String`: payload of the message \
                                                                       - `Integer`: the registration id (i.e. the id for the couple message
                                                                     pattern and sender pattern) that matched the incoming sms \
                                                                     \
                                                                      **Response:** 2 bytes acknowledgement.

52             SendSMS                     App-\>Agt                  Used by the application to send a SMS \
                                                                     \
                                                                      **Command payload**: a `list` of 3 objects: \
                                                                       - `String`: Phone number of the recipient of the outgoing message \
                                                                       - `String`: payload of the message \
                                                                       - `String`: format of the SMS, supported values are "8bits", "7bits", "ucs2" \
                                                                      \
                                                                      **Response payload**: \
                                                                       - `status`: 2 bytes acknowledging the command

9              GetVariable                 App-\>Agt                  Retrieve one or several variable from the Core Agent. \
                                                                      \
                                                                      **Command payload**: \
                                                                       - `String`: name of the variable to get (usually a string that is a path!)
                                                                       (it can be a list when getting several variables) \
                                                                     \
                                                                      **Response payload**: \
                                                                      - `status`: 2 bytes acknowledging the command. \
                                                                      if the status is OK (=0) \
                                                                      2 objects, can be null. The first object is the value of the variable,
                                                                     or null if the variable is a table. The second object is a list of all
                                                                     sub tree names

10             SetVariable                 App-\>Agt                  Set one or several variables on the Core Agent. \
                                                                      \
                                                                      **Command payload**: a `list` of 2 objects: \
                                                                       - 1 `String` representing the variable to set (usually a string that is a
                                                                     path!) \
                                                                       - 1 `object` representing the value of the variable 
                                                                       (it can be a hashtable when setting several variables)\
                                                                     \
                                                                      **Response payload**: \
                                                                       - `status`: 2 bytes acknowledging the command.

11             RegisterVariable            App-\>Agt                  Register one variable or path to get notification when it changes. \
                                                                      \
                                                                      **Command payload**: a `list` of 2 objects: \
                                                                       - `reg vars`: list of strings representing the variables to register to,
                                                                       each string is a path to register to \
                                                                       - `passive vars`: list of strings, each string identifying the path of a
                                                                     passive variable \
                                                                      \
                                                                      **Response payload**: \
                                                                      - `status`: 2 bytes acknowledging the command. \
                                                                      - `registration id`: string identifying this registration for variable
                                                                     change, this id is to be given to DeRegisterVariable command.

12             NotifyVariable              Agt-\>App                  Notify that a variable changed. \
                                                                      \
                                                                      **Command payload**: a `list` of 2 objects: \
                                                                      - 1 `string` representing the registration id that provoked this
                                                                     notification \
                                                                      - 1 `map` representing the variables data sent in the change notification,
                                                                     map keys are variables full path, map values are variable values, the
                                                                     map contains both registered variables that have changed and passive
                                                                     variables set in register request \
                                                                      \
                                                                      **Response payload**: \
                                                                       - `status`: 2 bytes acknowledging the command.

13             DeRegisterVariable          App-\>Agt                  Cancel previous registration for watching variable changes. \
                                                                      \
                                                                      **Command payload**: \
                                                                       - `String` representing the registration id (sent in RegisterVariable
                                                                     command Response payload) \
                                                                      \
                                                                      **Response payload**: \
                                                                       - `status`: 2 bytes acknowledging the command.

20             SoftwareUpdate              Agt-\>App                  Notify the Application that an update must be downloaded and installed
                                                                     from the url given into payload. \
                                                                      \
                                                                      **Command payload**: \
                                                                       - `componentname`: string that contains the application / bundle to update
                                                                     (the path **does contain** the assetid of the asset responsible for that
                                                                     update) \
                                                                       - `version`: string, empty string (but not null!) to specify
                                                                     de-installation request, non empty string for regular update/install of
                                                                     software component. \
                                                                       - `URL`: string; Can be empty when version is empty too, otherwise URL will
                                                                     be a non empty string defining the url (can be local) from which the
                                                                     application has to be updated and an optional username and userpassword
                                                                     ex *url:username@password* (username and/or password can be empty). \
                                                                       - `params`: a `map` whose content depends on the Application and the content
                                                                     of the Update package received by the Agent. Those params provide a
                                                                     way to give application specific update params. \
                                                                      \
                                                                      **Response payload**: \
                                                                       - `status`: 2 bytes acknowledging the command \
                                                                      \
                                                                      **Note**: There can be only one SoftwareUpdate request at a time.

21             SoftwareUpdateResult        App-\>Agt                  Return the result of the previous SoftwareUpdate request. \
                                                                      \
                                                                      **Command payload**: a `list` of 2 objects: \
                                                                       - `componentname`: a string holding the name of the component \
                                                                      update status code: integer (see [Software Update
                                                                     Module](../agent/Software_Update_Module.html) error code documentation) \
                                                                      \
                                                                      **Response payload**: \
                                                                       - `status`: 2 bytes acknowledging the command \
                                                                      \
                                                                      **Note**: There can be only one SoftwareUpdate request at a time.

22             SoftwareUpdateStatus        Agt-\>App                  Notify the application that the update's status has changed in agent update module.\
                                                                      \
                                                                      **Command payload**: a `list` of 2 objects: \
                                                                       - `status`: an integer representing the current update status. More details in [swi_update_Event enum](../c/swi__update_8h.html#a2f73926feabc8ef9800b642aa632cb57) in swi\_update.h\
                                                                       - `details`: a string containing additional informations for this status.\
                                                                       \
                                                                       **Response payload**: \
                                                                      - `status`: 2 bytes acknowledging the command.\

23             SoftwareUpdateRequest       App-\>Agt                  Request agent to change the current update status: can pause, resume , abort the update.\
                                                                      \
                                                                      **Command payload**: \
                                                                       - `status`: an integer representing the new required update status. More details in [swi_update_Request enum](../c/swi__update_8h.html#ad3de109c1a370fb8ae98ed2a83f8e6ca) in swi\_update.h\
                                                                       \
                                                                       **Response payload**: \
                                                                      - `status`: 2 bytes acknowledging the command.\

24             RegisterUpdateListener      App-\>Agt                  Enable an application to receive notifications to watch progress 
                                                                      (by the mean of EMP `SoftwareUpdateStatus` msg) of an update job in the agent .\
                                                                      \
                                                                      **Command payload**: empty! \
                                                                      \
                                                                      **Response payload**: \
                                                                      - `status`: 2 bytes acknowledging the command.\

25             UnregisterUpdateListener    App-\>Agt                  Disable software update notification sending to this application.\
                                                                      \
                                                                      **Command payload**: empty!\ 
                                                                      \  
                                                                      **Response payload**: \
                                                                      - `status`: 2 bytes acknowledging the command.\
                                                                      

30             PData                       App-\>Agt                  Push some unstructured data to the data manager \
                                                                      \
                                                                      **Command payload:** \
                                                                      an `hashmap` with fields:\
                                                                      - `asset`: name of the asset \
                                                                      - `queue`: triggering policy (nil refers to the default policy) \
                                                                      - `path`: common path prefix for sent data \
                                                                      - `data`: possibly nested table of path suffix / data pairs \
                                                                          \
                                                                      **Response:** 2 bytes acknowledgement.

32             PFlush                      App-\>Agt                  Force the immediate flush of a given trigger policy. \
                                                                     \
                                                                      **Command payload:** an `hashmap` with :field "policy" containing the
                                                                     name of the policy to flush. If no name is given, the "default" policy
                                                                     is flushed. \
                                                                     \
                                                                      **Response:** 2 bytes acknowledgement.

33             PAcknowledge                App-\>Agt                  Push an acknowledge to data manager, given trigger policy. \
                                                                      \
                                                                      **Command payload:** \
                                                                      an `hashmap` with a fields:\
                                                                      - `ticket`: the ticket identifying the message to acknowledge,\
                                                                      - `status`: status code (integer) for the acknowledge (0=OK, other value
                                                                        means error)\
                                                                      - `message`: error message to send along with status code in the
                                                                        acknowledge\
                                                                      - `policy`: triggering policy (nil refers to the 'now' policy)\
                                                                      - `persisted`: whether the acknowledge has to be stored in ram or flash.\
                                                                          \
                                                                      **Response:** 2 bytes acknowledgement.

40             TableNew                    App-\>Agt                  Create a new structured table \
                                                                      \
                                                                      **Command payload:** \
                                                                      an `hashmap` with fields:
                                                                      - `asset`: name of the asset\
                                                                      - `storage`: "ram" or "flash"\
                                                                      - `policy`: triggering policy (nil refers to the default policy)\
                                                                      - `path`: common path prefix for table data\
                                                                      - `columns`: columns configuration, as used by stagedb's constructor. \
                                                                      \
                                                                      **Response:** 2 bytes acknowledgement, plus a table identifier to
                                                                         be used in further operations on this table

41             TableRow                    App-\>Agt                  Push a row of data in an existing data table \
                                                                      \
                                                                      **Command payload:** \
                                                                      an `hashmap` with fields:\
                                                                       - `table`: table identifier\
                                                                       - `row`: map of data to push, which format is { columnname1 = value,
                                                                        columnname2 = value, ... }\
                                                                          \
                                                                      **Response**: 2 bytes acknowledgement.

43             TableSetMaxRows             App-\>Agt                  Set a maximum number of rows in the table. The table will auto-flush
                                                                     itself when it reaches that number \
                                                                      \
                                                                      **Command payload:** \
                                                                      an `hashmap` with fields:\
                                                                       - `table`: table identifier\
                                                                       - `maxrows`: max \# of row in the table \
                                                                     \
                                                                      **Response:** 2 bytes acknowledgement.

44             TableReset                  App-\>Agt                  Remove all content from a table \
                                                                      \
                                                                      **Command payload:** \
                                                                      an `hashmap` with fields:\
                                                                       - `table`: table identifier \
                                                                      \
                                                                     **Response:** 2 bytes acknowledgement.

45             ConsoNew                    App-\>Agt                  Setup a new consolidation table \
                                                                     \
                                                                     **Command payload:** \
                                                                     an `hashmap` with fields:
                                                                     - `src`: source table identifier\
                                                                     - `path`: common path prefix for table data\
                                                                     - `columns`: columns configuration, as used by stagedb's constructor\
                                                                     - `storage`: "ram" or "flash"\
                                                                     - `send_queue`: send triggering policy for destination table (nil
                                                                       refers to the default policy)\
                                                                     - `conso_queue`: consolidation policy for source table (nil refers to
                                                                       the default policy) \
                                                                     \
                                                                     **Response:** 2 bytes acknowledgement, plus destination table
                                                                       identifier to be used in further operations on this table

46             ConsoTrigger                App-\>Agt                 Consolidate content of table immediately \
                                                                     \
                                                                     **Command payload:** \
                                                                     an `hashmap` with fields:\
                                                                     - `table`: table identifier\
                                                                     - `dont_reset`: (Boolean) if true, the table's content isn't erased\
                                                                       after it's been consolidated. \
                                                                     \
                                                                     **Response:** 2 bytes acknowledgement.

47             SendTrigger                 App-\>Agt                  Send content of table to server immediately \
                                                                     \
                                                                     **Command payload:** \
                                                                     an `hashmap` with fields:\
                                                                     - `table`: table identifier\
                                                                     - `dont_reset`: (Boolean) if true, the table's content isn't erased
                                                                       after it's been sent. \
                                                                       \
                                                                     **Response:** 2 bytes acknowledgement.

50             Reboot                      App-\>Agt                  Requests reboot of the system running the Agent. The reboot will
                                                                     occur after a small delay. \
                                                                     \
                                                                     **Command payload:**\
                                                                      - `reason`: string describing the reason to request the reboot, the\
                                                                        reason will be displayed in the Agent logs.\
                                                                     \
                                                                     **Response:** 2 bytes acknowledgement.

--------------------------------------------------------------------------------------------------------------------------------------------------------------


