Software Update Package
=======================

This type of update package is:

-   supported only on **Linux devices with Agent installed**.
-   can be delivered by M3DA protocol, or directly copied on the device
    and then "locally" installed.

The purpose of this file format is to :

-   handle update of several components on embedded device.

Definitions:

-   **feature**: unit of software that provides one functionality, can
    be provided by different components, a feature can be provided
    several times on the same device by different component.\
     A feature can not be installed or removed directly (i.e you have to
    use a component for that).
-   **component**: id of a group of files, once installed those files
    can provide one or several features.\
     A component can be installed or removed.
-   **update package**: archive with several components.

1. Software Update Package Format
=================================

#### 1.1. Archive Format

Software Update Package is basically an **archive file in tar format**
containing:

-   **Manifest**: a file describing the content of the update (see
    Manifest section for further description)
-   **one or more components**: those are the files and/or folders to be
    used to process the update of the given component.

> **WARNING**
>
> Avoid file names or sub folder names containing **'** (single quote)
> character within Software Update Package archive.\
> There is no other limitation related to components folders/files.


> **WARNING**
>
> Exact **archive file format** support (for both extraction and automatic
> format detection steps) totally **relies** on **tar command** available
> options on the **embedded target**.\
>  Once downloaded, the archive file is extracted using ***tar -xvf***
> command, with no specific compression format specified.\
>  See your embedded Linux distribution documentation to know which
> formats are supported for extraction with automatic format detection.\
>  Usually, gz and bz2 are correctly supported.

#### 1.2. Manifest content

The manifest file format is:

-   **file named "Manifest"** put **at the root** of the update package
-   the file must **declare a Lua table** containing the description of
    the update, the manifest must contain **only this table** (plus
    comments).

The manifest is divided in two parts:

-   The first one contains global update information
-   The second part describes each component included in the archive.

##### 1.2.1. Version declaration / checking

Versions are used in various places of the Manifest.\
 The versions will be put **as string** and the comparison between 2
versions will be done using **strcmp-like** function, using **ascii
order**.\
 The version string only allows the following characters: alphanumeric,
'-' (minus), '_' (underscore), '.' (dot), ':'(colon).\
 Examples: "2.1" > "1.3", "3.a" > "3.9", "1.a" > "1.Z", "1.10" <
"1.9" but "1.10" > "1.09" etc.

When defining a **dependency** on one or several version of a component,
the following format should be used:

-   embedded in a string
-   space as separator between part of dependency, earch part is and a
    **logical and** with the next part
-   special characters "=", "!=", ">=, ">", "<=", "<" have the regular
    meaning of comparison operators, using the ascii order.
-   special character "#" enables to specify a list (i.e **logical
    or**) of accepted exact match separated by a comma.
-   at least one special character must start the dependency definition

This format is referred as **[version_dep]** in the following
sections.\
 example: ">1.0 <=3.0" means any version strictly above "1.0" **and**
inferior or equal to "3.0".\
 example: "#a,b,c" means any version a **or** b **or** c

##### 1.2.2. Global package information

-   components: **mandatory**, this is a **table**, used as an **ordered
    list**, the list of components to be installed ,
-   force: optional, **boolean**, if set then no dependency checking is
    done on the update package. Default value is false, meaning
    dependency checking will be done,
-   version: optional, **string** (non empty), some kind of user data:
    this can be used to give a name to a combination of components
    available on the device after the update.\
     No check is performed on this field, and if the update is
    successful, then the old value is replaced by this one.

> **WARNING**
>
> force parameter is to be used with caution.\
> force parameter is likely to put the device in a software configuration
> that will **not** be valid, requiring other "forced" updates to put it
> back in valid state.

##### 1.2.3. Component update information

-   name: **mandatory**, it's the component name as a **string** (non
    empty), and it must be **unique** within one update package.\
     This is usually a path in dot notation, the first elements of the
    component path determine which software module is responsible for a
    component update. See below section.
-   version: **mandatory** (unless for component removal), **string**
    (non empty), version of this component
-   location: optional, **string** (non empty), it's the relative path
    to locate the file or folder within the update package to be used as
    component update. The same location can be reused by several
    components within one update package. When given, location must
    point to a file/folder in the update package, if location points to
    an absent file/folder, the update package will be rejected.
-   depends: optional, **table**, keys are the feature names or
    component names as **strings** (non empty), values are (non empty)
    **strings in [version_dep]** format.
-   provides: optional, **table**, keys are the feature names as
    **strings** (non empty), values are **strings** (non empty) defining
    versions of features provided by this component.
-   parameters: optional, **table**, keys are the parameters names as
    strings (non empty) or as integer to specify a list instead a map,
    values are parameters values (basic types only: string, number,
    boolean). Both parameters names and values are specific to the
    software module responsible for the update of this component.

##### 1.2.4. A little example:

~~~~{.json}
{
  -- Global information
  version = "1.1",
  
  -- Components information
  components = 
  {
    {   name = "assetLivingRoom.LivingRoom.update",
        location = "assetLivingRoom/update.zip",  --location is a file in this case      
        depends = { assetLivingRoom="=1.0" },
        provides = { assetLivingRoom="1.1"},
        version = "1.3"
    },
    
    {   name = "assetBedRoom.BedRoom.update",
        location = "assetBedRoom/update.tar", --location is a file in this case        
        depends = { assetBedRoom = "=1.0", assetLivingRoom=">0.1 <2.0" },
        provides = {  assetBedRoom = "1.1" },
        version = "1.2"
    },
    
    {   name = "@sys.appcon.my_app",
        location = "app/", --location is a folder in this case        
        depends = { Agent=">0.7", my_app = "=1.1" },
        provides =  { my_app = "1.2" },
        -- ApplicationContainer specifics attributes
        parameters = { purge=true, autostart=true },
        version = "1.2"
    },
    --cmp uninstall:
    {   name = "@sys.appcon.app2",
    },
    {   name = "@sys.update.GardenApp",
        location = "GardenApp/",        
        depends = {},
        version = "0.1"
    },
    --cmp uninstall:
    {   name = "@sys.appcon.app3",
    },
  }
}
~~~~

2. Components/Features Management Use Cases
===========================================

This section lists some kind of "applicative" behaviors built over the
update package defined above.

#### 2.1. Determine which software module is responsible for a component update

The software module responsible for an update is determined by the value
of name field of each component description.\
This field is interpreted as an M3DA path:

-   dot is the separator
-   @sys is special value targeting the device, i.e. the Agent.
-   otherwise the root is the id/name of the asset that will be
    responsible to do the update.

The value of this field is very important because it determines who will
receive the update notification.

If the component targets an asset, then you have to look at the asset
API to see how to deal with the update.\
Basically the asset will receive the path where to find files embedded
in update package, but file copy/update operation will have to be
handled by the application itself. To put it another way: when sending
an update to an asset, make sure the asset application will know what to
do with the files in the package.

If the component targets the Agent, it means that the update uses a
built-in update functionality of the Agent. Those built-in update
functionalities of the Agent are identified using special values of
component name, i.e. path shortcuts are defined to trigger them.

New shortcuts may be added afterwards, here is the **current built-in
updaters** of the Agent:

##### 2.1.1. Install script

Install script is to execute a simple Lua script during update process,
without having to write an Application to install in the
ApplicationContainer, or any asset program etc.\
The Lua script is embedded in the update package and is run by the
Agent.

More details on [Software Update
Module](Software_Update_Module.html) page.

Manifest Component specific fields:

-   name: @sys.update.[script\_name]

##### 2.1.2. ApplicationContainer update

To install/update/remove an application managed by the
ApplicationContainer, the module of the Agent responsible for
installing and monitoring applications.\
As this kind of update is managed by an Agent module, the
ApplicationContainer, the update actions are done by this module, please
note that:

-   the ApplicationContainer module does the files copies using the
    files present in the package without requiring any action of the
    application itself
-   the application needs to conform to ApplicationContainer application
    requirements to enable application monitoring
-   the install path is fixed : each application installed in
    ApplicationContainer is copied in specific folder, but all the
    applications folders have the same root folder.

More details on [Application Container](Application_Container.html)
page.

Manifest Component specific fields:

-   name: `@sys.appcon.[application_name]`\
     [application_name] can be any value, the ApplicationContainer will
    receive it as one string all characters after the @sys.appcon.
    shortcut.
-   location: it has to point to a **folder**, see [Application
    Container](Application_Container.html) page for application data
    specifications.
-   parameters: specific parameters: those parameters are used only for
    installing/reinstalling an application, those parameters are
    directly related to those of the install API of [Application
    Container](Application_Container.html).
    -   `autostart`: `boolean`, automatically start the app after
        install and on Agent reboot; default value is `false`
    -   `purge`: `boolean`, when reinstalling an application, the
        application runtime folder will be erased before reinstallation
        if this field is set to true; default value is `false`

> **INFO**
>
> The following use cases (add/remove) are also valid for
> ApplicationContainer components

#### 2.2. Add a new component

Nothing special to do, just give the new component name and choose the
good path to target the software component.

-   name:new component name (include updater prefix)
-   version:version of that component

#### 2.3. Update a component

-   name:updated component name (include updater prefix)
-   version:version of that component

#### 2.4. Remove a component

-   name: name of the component to remove
-   version: **nil**
-   provides: **nil**
-   depends: **nil**

> **WARNING**
>
>
> -   When removing a component it will also remove all features that were
>     provided by this component
> -   After uninstall, components are removed from the device software
>     list
> -   If one or several components depend on the components (and its
>     provided features) being removed by the uninstall operation, then
>     this is treated as a dependency failure: the update is aborted.

#### 2.5. Dependencies-related behavior on embedded side

When global option **force** is set, then no dependency check is done,
even if some dependencies are defined in the Manifest.\
 Otherwise, dependencies checking is done before applying any update.\
 If one or more dependencies fail to be satisfied, then the update is
**aborted** and the software of the device is not altered.

3. Transport Protocol specifics
===============================

The archive and its content as described above remain valid across
protocols but there is still some specifics to define.

#### 3.1. Status Codes

> **WARNING**
>
> this is to be defined this is first questions and propositions

First proposition:

-   use the same numerical codes for errors reporting across protocols
    (this avoids to maintain two/n mapping (error code <-\> error
    description) for the same errors)
-   use OMADM FUMO codes + vendor specified ranges to defined codes not
    supported by FUMO (FUMO doesn't deal with several software
    components, dependencies etc)
-   add error string description in AWTDA command acknowledge when a
    interesting text description of the error is available.
-   keep a value range to set project specifics code, those code would
    be (set / overwritten) in some model

#### 3.2. M3DA transport

With M3DA, the embedded will receive the archive file described above,
the MD5 being embedded as a parameter in the M3DA command used to get
the update package.\
 See M3DA [Device Management commands](Device_Management.html) to see
description of the M3DA SoftwareUpdate command.

4. Steps to use Software Update Package
=======================================

Those are the main steps.

#### 4.1. Embedded Side

1.  Get MD5 sum and archive file (this steps depends on transport
    protocol used to retrieve the update package)
2.  Check archive file integrity
3.  Extract the archive
4.  Parse the Manifest file
5.  Check Manifest content (including dependencies)
6.  Send update notification to software module responsible to do the
    updates.
7.  Collect update result and compute whole update result
8.  Report the update result to the server (this steps depends on
    transport protocol used to retrieve the update package)

#### 4.2. Server Side

1.  Get components to update with related dependencies
2.  Generate Manifest file according to previously gathered data
3.  Compress the archive
4.  Determine the transport protocol to use and protocol specific
    binaries if needed.
5.  Create the update job, ask the device to connect of wait the device
    to connect
6.  Wait for update status reported by device

> **WARNING**
>
> it may be useful to define a configurable timeout for the update job,
> when this timeout is elapsed, then the job is failed, a server could set
> up a 'synchronize' job to check device state at next connexion, and the
> user can re-create an update job afterward without having to clean the
> previous job.

#### 4.3. Data exchange

Those data will be accessible on the device (read only) so that the
server can use them : either to give info to the user, or to generate
update packages, ...

~~~~{.json}
@sys.update = {
    last_update = {}, -- TBD
    last_failed_update = {},  -- TBD

    components = { 
        {
            name = "assetBedRoom.BedRoom.update",
            depends = { assetBedRoom = "=1.0", assetLivingRoom = ">0.1 <2.0",  my_app = ">= 1.1" },
            provides = {  assetBedRoom = "1.1" },
            version = "1.4"
        },
        
        {
            name = "@sys.appcon.my_app",
            depends = { Agent = ">0.7", my_app = "=1.1" },
            provides =  { my_app = "1.2" } ,
            parameters = {purge=true, autostart=true },
            version = "1.2"
        }
    }

}
~~~~

