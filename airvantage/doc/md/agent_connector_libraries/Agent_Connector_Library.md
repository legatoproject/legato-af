AirVantage Agent Connector library
======================


#### 1. Presentation

The AirVantage Agent Connector library provides access to the AirVantage M2M
platform through the Agent. It is a library which dialogs with the Agent process
through a dedicated IPC channel, most commonly a socket.

Communications with the server include buffering and consolidating data
on the embedded side, flushing them to the server according to
customizable policies, subscribing to server-side or hardware events and
value changes, acknowledge management.

The Agent naturally reasons by assets: connections to it are
associated with a given asset when established, events and notifications
are dispatched by asset, etc. The Agent's logical assets might map
one-to-one with physical assets connected to the physical device, but
they don't have to. Within an asset, data are organized into tree paths,
and it is possible to subscribe to data or event limited to any
arbitrary sub-path.

As a general principle, the Agent takes responsibility for the data
that have been passed to it: it is in charge of optimizing their
encoding and regrouping, of handling acknowledgements, of securing them
in non-volatile memory if asked to, etc.


#### 2. Create a New Asset Instance, Initialize a New Asset

Creating a new asset is as simple as calling `swi_av_asset_Create`
with a mandatory asset id string.\
 It returns an AirVantage asset object, which acts on behalf of the
asset.\
 An asset does not necessarily represent a physical piece of hardware;
it is merely a representation of the global application architecture.

At this point, the asset is created but is not ready to communicate with
the Agent. To make it active, the `swi_av_asset_Start` method must be called
first, after any required event subscription has been performed.

~~~~{.c}
#include "swi_airvantage.h"

int airvantage_sample(){
    swi_av_Asset_t *asset = NULL;    
    // configure the library, establish connection with the agent
    swi_av_Init();
    // create asset
    swi_av_asset_Create(&asset, "house");
    // register asset
    swi_av_asset_Start(asset);
}
~~~~

The asset identifier is the root element of the message path. For
instance, if the variable "foo" is written under path "bar" in the asset
"myAsset", the complete path as seen by the server is "myAsset.bar.foo".

#### 3. Data Sending

##### 3.1. Generalities

Data can be sent with two different APIs:

-   Through pre-declared tables, where the user can specify the table's
    persistence mode, columns, serialization options, etc.
-   Directly, by passing data in a C parameter. This is the most flexible
    way in terms of what can be sent, but it does not allow you to
    customize storage and emission settings.
    
##### 3.2. Event Sending

Noteworthy events are notified between the assets and the server by
sending data. When designing an application, some conventions are taken
about which paths represent "normal" data, and which ones\
 will be used to represent events. The embedded application will
subscribe to these paths and hook a proper handling function on them;
symmetrically, the server will be configured to react to "event\
 paths" by causing the appropriate notifications.

Both data sending APIs can be used (raw data or pre-declared tables).
However, given the generally simple and immediate kind of data exchange
suitable to event notifications, the raw data API will most often be the
best choice.

##### 3.3. Policies

Data is not normally sent to the server as soon as the user requests it.
It is accumulated by the Agent to limit the number of connections,
to optimize their encoding, and optionally to perform some data
consolidation operations on them. Data emission is performed according
to a policy, which can be:

- `cron="cron_config"`: The operation is performed at predefined
  dates, specified using the standard Unix cron syntax.
- `latency=n`: The operation will be performed in `n` seconds at most.
- `manual=true`: The connection will be triggered by the application
  through an explicit call to `swi_av_TriggerPolicy()`.
- `onboot=n`: The operation will be performed `n` seconds after the
  agent has been initialized.

There can be multiple policies set up on a single device, and AirVantage
APIs allow to choose according to which policy when each data is sent.

###### 3.3.1. Policy Examples

If `cron="*"`, the device will connect at the beginning of every hour if
there is something to send. Even if a connection is forced at 8:59, the
cron connection will still be forced at 9:00.

If `latency=60*60`, the device will connect every one hour: the first
connection will be done one hour after the device booted. However, if a
periodic connection is due at 9:00, and a connection is forced at 8:59,
the next connection will occur at 9:59 rather than at 9:00. This policy
guarantees that data will not be buffered for more than 60\*60 seconds
before being sent to the server, and that no more than 60\*60 seconds
will pass between two connections. (Connections not only allow data to
be sent to the server, but also to receive data sent by it).

`onboot=n` policy rules are mostly useful if some unsent, persisted data
might have been left unsent before a reboot. With such a policy, data
unsent because of a shutdown will be sent when the device reboots.

###### 3.3.2. Policy Configurations

To allow the handling of several policies, policy queues can be
declared, in unlimited number, in the agent's configuration. They are
indexed by name in `"data.policy"`, and contain a table with one key and
one value. The key is one of `"cron", "latency", "manual"`. More than
one pair can be given for a single policy.

-   cron policies are triggered at predefined dates, which can be
    described with the same formalism as POSIX' cron task scheduler;
-   latency policies are guaranteed to be triggered before a given delay
    in seconds elapses. The delay, when non-zero, leaves a chance to
    group several triggerings together and to save some bandwidth;
-   manual policies are only triggered when the user application
    explicitly triggers them.

Some policies must exist and have certain properties:

-   `"default"` is the policy chosen when a data sending command is
    given with no explicit policy. Users can change the way this policy
    is triggered.
-   `"never"` cannot be triggered: data associated with this policy are
    never sent.
-   `"now"` is intended for data which are to be sent (almost)
    immediately. It must be configured to be sent with a latency, and in
    normal situations, this latency should be short, no more than a
    couple of seconds.

####### 3.3.2.1. Policy configuration examples

Please note that even though basic knowledge on data policies is advised to correctly use 
AirVantage API, the policy configuration (creation/modification) demonstrated in this very section is
optional.
In basic use cases, using the default policies as provided by the agent will fulfill the needs.

There are several ways to access the Agent configuration: you can change the provisioning configuration,
connect to the agent telnet console, use an API or even do it remotely using AirVantage platform.

In the following example, the programmatic approach is taken, using the dedicated API from swi_devicetree.h 
to access Agent parameters.

(You can have an overview of the swi_devicetree API by reading the section `5. Setting and Getting Agent Internal Variables`)

~~~~{.c}
#include "swi_devicetree.h"

int set_policies(){
    //init agent parameter access library
    swi_dt_Init();
    //configure some policies
    swi_dt_SetInteger("config.data.policy.hourly.latency", 60*60);
    swi_dt_SetBool("config.data.policy.manually.manual", true);
    swi_dt_SetString("config.data.policy.midnight.cron", "0 0 * * \*");
}
~~~~



####### 3.3.2.2. Why policies?

Policies introduce an indirection in the way data are flushed from the
agent to the server. This indirection is extremely precious over the
lifetime of an M2M application: policies can be changed remotely, by
asking the server to update the agent's configuration. If data
triggering were done explicitly by the application, adapting the
reporting policy to changing conditions (bandwidth congestion and
billing, change in usage patterns...) would require a full software
update.

There are a couple of legitimate, very advanced cases where an
application needs direct control over data sending policies, and this
can be done by forcing the triggers on a manual-only policy. But it
should be avoided in most reasonable use cases, and represents a
liability for long term maintenance.

##### 3.4. Sending Undeclared Data


~~~~{.c}
rc_ReturnCode_t swi_av_asset_PushFloat(swi_av_Asset_t* asset, const char *pathPtr,  const char* policyPtr, uint32_t timestamp, double value);
rc_ReturnCode_t swi_av_asset_PushInteger(swi_av_Asset_t* asset, const char *pathPtr,  const char* policyPtr, uint32_t timestamp, int value);
rc_ReturnCode_t swi_av_asset_PushString(swi_av_Asset_t* asset, const char *pathPtr,  const char* policyPtr, uint32_t timestamp, const char* value);
~~~~


The path parameter specifies the datastore path under which data will be stored relative to the asset node,
the last path segment will be used as a datastore key.

These functions send some data according to the specified policy (or the
default policy if the policy is not specified).

Notice that for the server, all data is timestamped. If specific value `SWI_AV_TSTAMP_NO` is used, 
no timestamp is put in the record sent to the server, it will be timestamped at
the date of its reception by the server.

###### 3.4.1. Examples

~~~~{.c}
swi_av_Asset_t *asset = NULL;

swi_av_Init();  // configure the library, establish connection with the agent

// create asset
swi_av_asset_Create(&asset, "house");

//Push integer 1 on path house.foo.x, timestamp is automatically generated at execution time on the device side by the
//library, data is linked to the default policy (Cf. NULL parameter)
swi_av_asset_PushInteger(asset, "foo.x", NULL, SWI_AV_TSTAMP_AUTO, 1);

//Push string "bar" on path house.foo.y, timestamp will be generated by AirVantage Server at reception time,
//data is linked to the `midnight` policy
swi_av_asset_PushString(asset, "foo.y",  "midnight", SWI_AV_TSTAMP_NO, "bar");

~~~~


##### 3.5. Predeclared Data Tables

Data tables can be declared as follows:

~~~~{.c}
rc_ReturnCode_t swi_av_table_Create(swi_av_Asset_t* asset, swi_av_Table_t** table, const char* pathPtr, size_t numColumns,
    const char** columnNamesPtr, const char* policyPtr, swi_av_Table_Storage_t persisted, int purge)
~~~~

A table is associated with a given path (relative to the asset's root)
and policy (default policy if unspecified). It can also have either a
`STORAGE_RAM` or `STORAGE_RAM"` mode. Finally, it predeclares the columns
which it handles; all records later pushed in the table must conform to
this set of columns. The parameter  `columnNamesPtr` is a list of column names as
strings.

Using this API, if the data is meant to be timestamped on the device ( i.e. at sending/acquisition time)
, then the table declaration *must* contain a column named `"timestamp"`, later on filed with integers. 

In the future, columns will support more advanced settings, including
serialization policies which affect their bandwidth usage vs. data
precision compromise.

The result of this function is a table proxy object `table`, which
supports methods `swi_av_table_PushFloat`, `swi_av_table_PushInteger`, and `swi_av_table_PushString`, 
to fill a row with each column value, and finally `swi_av_table_PushRow` to push the whole data row to the Agent.

###### 3.5.1. Example

~~~~{.c}
const char *tableColumns[] = {
    "latitude",
    "longitude",
    "altitude",
    "timestamp"
  };

swi_av_table_Create(asset, &table, "position", 4, tableColumns, "hourly", STORAGE_RAM, 0);
~~~~

###### 3.5.2. Pushing a Row in a Predeclared Table

~~~~{.c}
//Push one value, pick-up the appropriate function
rc_ReturnCode_t swi_av_table_PushFloat( swi_av_Table_t* table, double value);
rc_ReturnCode_t swi_av_table_PushInteger( swi_av_Table_t* table, int value);
rc_ReturnCode_t swi_av_table_PushString( swi_av_Table_t* table, const char* value);
//Once the row is complete, push it to the Agent
rc_ReturnCode_t swi_av_table_PushRow( swi_av_Table_t* table);
~~~~

Push a row of data in a predeclared table. Keys must match the
predeclared column names.

###### 3.5.3. Example

~~~~{.c}
swi_av_table_PushInteger(table, 49.171699); //latitude
swi_av_table_PushInteger(table, -123.070741); //longitude
swi_av_table_PushInteger(table, 5); //altitude
swi_av_table_PushInteger(table, time(NULL)); //timestamp
  
swi_av_table_PushRow(table);
~~~~

###### 3.5.4. Forcing the Emission of a Table Content

TODO: Not available yet.

###### 3.5.5. Forcing a Policy Flush

All the data attached to a given policy can be flushed to the server
immediately with function:

~~~~{.c}
rc_ReturnCode_t swi_av_TriggerPolicy( const char* policyPtr);
~~~~

If `policyPtr` is passed as NULL, the default policy is used. If the
special name `"*"` is passed, all policies are flushed.

#### 4. Data Reception


##### 4.1. Data sent by AirVantage server (Data Writing)

AirVantage server is able to send data to a device in order to request some action on it: changing some configuration
parameter, trigger some action etc.
The data can be addressed to the AirVantage Agent or to another application, in any case the AirVantage Agent will receive 
the data first and then forward it to the appropriate application.

(FYI, AirVantage server can only modify data defined in the application data model as a setting, and
not a variable.)

AirVantage Agent library provides API to enable your application to handle the reception of data coming from the AirVantage server.

##### 4.2. Registering a Function to handle data reception

~~~~{.c}
typedef void (*swi_av_DataWriteCB)(
    swi_av_Asset_t *asset,
    const char *pathPtr,
    swi_dset_Iterator_t* data,
    int ack_id,
    void *userDataPtr
);

rc_ReturnCode_t swi_av_RegisterDataWrite( swi_av_Asset_t *asset, swi_av_DataWriteCB cb, void * userDataPtr);
~~~~

Once a callback has been registered, it will be called each time the server sends some data to the application, alongside with customizable userdata.

The `pathPtr` specifies the root path of the data, this way the application can determine what is the point the data being received. 
(E.g. the path can be "action1" or "action2" to provoke two different behavior of the application.)

The `data` object is meant to iterate over the values received.
As received data are completely dynamic from the library point of view, this object makes it possible to abstract any kind of data the application can receive (string, integer, float, ...).

The application have to iterate over the object to get all received values.
Let's take one example: acting on a garage door of a house.
The message from the server could trigger this action with two parameters : the direction and the speed. 
In that example, the `pathPtr` could be "garageDoor", then the `data` object could contain 2 items: the first one named "direction" and value "up", 
the second named "speed" with value "max".

Finally the `ack_id` param is used to acknowledge the data, see the next section for that topic.

##### 4.3. Acknowledging received data

When the AirVantage server sends data, it can request to receive an acknowledge to confirm the correct processing of the data by the application.
This acknowledgment takes place at an applicative level, meaning that it is up to the application to acknowledge the data, once the
appropriate actions have been taken.

AirVantage Agent library provides this function to fulfill this need.

~~~~{.c}
rc_ReturnCode_t swi_av_Acknowledge( int ackId, int status, const char* errMsgPtr, const char* policyPtr, int persisted);
~~~~

##### 4.4. Data reception example

~~~~{.c}
//will be called when the device received some data addressed to this application (asset). 
my_datacallback(swi_av_Asset *asset, const char *path, swi_dset_Iterator* data, int ack_id, void *userData){
  if(! strncmp(path, "commands.setvalue", strlen("command.setvalue")) ){ //known command
   int64_t cmd_value;
   assert(RC_OK == swi_dset_GetIntegerByName(data, "cmd_value", &cmd_value) );
   setvalue(cmd_value);
  }
  else{ //unknown command/writing, just print the request
     printf("received data on path[%s]:\n", path);
     while(RC_NOT_FOUND != swi_dset_Next(data) ){
       printf("data name: [%s]\n", swi_dset_GetName(data));
       //...
     }
  }
  //server requested acknowledgement
  if(ack_id)
    swi_av_Acknowledge(ack_id, 0, NULL, "now", 1);
}

main(...){
  swi_av_Asset_t *asset = NULL;  
  // configure the library, establish connection with the agent
  swi_av_Init();
  // create asset
  swi_av_asset_Create(&asset, "house");
  // register callback for data reception
  swi_av_RegisterDataWrite(asset, my_datacallback, NULL)
  // start asset (ready to send/receive data).
  swi_av_asset_Start(asset);
}

~~~~



#### 5. Setting and Getting Agent Internal Variables

The Agent has a list of internal variables that can be manipulated
by a user application. For instance, these variables allow to interact
with the Agent configuration.


The provided "Get" and "Set" API allow to access
those Agent internal variables.\

 The `swi_dt_Get` API allows you to retrieve the value of a terminal variable as
well as to retrieve and discover branches of the tree representation of
variables.\
The `swi_dt_SetInteger`, `swi_dt_SetFloat`, `swi_dt_SetString`, `swi_dt_SetNull`, `swi_dt_SetBool`
API allow you to set the value of a terminal variable and to create a 
new variable when it does not exist.


The `swi_dt_Register` API allows you to subscribe to change on individual
variables, on whole sub-trees, or on a mix thereof. It also handles
passive variables: normally, when a callback is triggered because some
registered variables changed, it only receives the variables whose
values changed as parameters. If some passive variables are listed
during the registration, their content is always passed to the callback,
whether it changed or not. A change in a passive variable won't trigger
the callback, though--unless it is also listed as an active one.


