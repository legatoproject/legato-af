Overview
========
RPC is an EXPERIMENTAL Alpha-quality feature that is actively under development,
and may be subject to future changes, which may not be backwards compatible.

As an Alpha-quality feature, the RPC framework may be unstable or incomplete,
and may contain bugs or other related issues.

Please use at your own discretion.

Feedback can be provided through our Legato Forum, forum.legato.io.



HelloRpc Sample Applications
============================

Contains sample applications for demonstrating Legato RPC bindings.
Consists of:
(1) two Client applications (printClient and printClientExtended),
(2) two Server applications (printServer and printServerExtended),
(3) two corresponding definition API files (printer.api and printerExtended.api), and
(4) two corresponding System Definition files (helloRpc-Alice.sdef and helloRpc-Bob.sdef).

The sample applications allow two RPC bindings to be created,
one using a combination of printClient/printServer/printer.api, and
the other using a combination of printClientExtended/printServerExtended/printerExtended.api.

RPC Bindings
------------
The helloRpc definition files will result in the following RPC bindings
being created:

    Bob.bobClient -> Alice.bobServer
    Alice.aliceClient -> Bob.aliceServer


Network Topology
----------------
To create RPC bindings, you will require:

(1) Two Legato systems that are networked together using TCP/IP.
(2) The two systems must be reachable by one another.
(3) TCP Port # 54323 (or equivalent) to be open and accessible.
    NOTE: This is configurable (see Configuration section).


Definition Files
----------------
Two System Definition files have been provided in the helloRpc directory:

helloRpc-Alice.sdef - for building System "Alice", and
helloRpc-Bob.sdef - for building System "Bob".

The RPC framework is built using extensions available in the System Definition
(.sdef) file.  Two new sections, "extern:" and "links:", are provided to define
the RPC configuration.

(1) "extern:" - Used to export the interface from the system.
    This enables an interface to partake in a RPC binding.

    extern:
    {
        bobServer = printServer.printer
        aliceClient = printClientExtended.printerExtended
    }


(2) "links:" - Used to define the implementation of the RPC
    Communication API (commonly referred to as le_comm).
    The le_comm API plugin provides a specific networking implementation.
    A default implementation, called Network Socket
    ($LEGATO_ROOT/components/networkSocket) has been provided for your
    convenience.

    links:
    {
        LinkToBob = (networkSocket)
    }


KConfig
-------
The RPC feature is currently an EXPERIMENTAL Alpha-quality feature.
To enable RPC, please enable the EXPERIMENTAL KConfig flag,
as well as, the RPC feature KConfig flags located under the
Framework configuration menu.

            [*] Enable RPC Proxy infrastructure (EXPERIMENTAL)
            [ ]   Enable RPC Proxy as a library component


Once enabled, a clean rebuild of your base Legato framework will be required.
That is:

    cd legato
    make clean
    make <target>


Please note:
(1) Library component support is currently not supported (WIP).


Build Systems
-------------
The helloRpc Makefile will generate two system update images,
one for "Alice", the other for "Bob".
Each system image will include the default Legato framework,
the RPC Proxy application,
and the helloRpc client and server sample applications.
To build, use the following commands:

    cd legato/apps/sample/helloRpc
    make clean
    make <target>

To download the system update images:

    update helloRpc-Alice.<target>.update <IP Address of "Alice">
    update helloRpc-Bob.<target>.update <IP Address of "Bob">


Run-time Configuration
----------------------
To activate a RPC binding, it must be added to the system run-time
configuration, using the RPC Configuration tool (rpctool).

On System "Alice",

    // Define the RPC bindings
    rpctool set binding bobServer Bob bobClient
    rpctool set binding aliceClient Bob aliceServer

    // Define the System network-link configuration
    // Assuming System "Bob" has an IP address of 192.168.3.2
    // with a TCP Port # 54323
    rpctool set link Bob LinkToBob argc 2
    rpctool set link Bob LinkToBob argv "192.168.3.2 54323"


On System "Bob" (assuming an IP Address of 192.168.3.2),

    // Define the RPC bindings
    rpctool set binding bobClient Alice bobServer
    rpctool set binding aliceServer Alice aliceClient

    // Define the System network-link configuration
    // Assuming System "Alice" has an IP address of 192.168.2.2
    // with a TCP Port # 54323
    rpctool set link Alice LinkToAlice argc 2
    rpctool set link Alice LinkToAlice argv "192.168.2.2 54323"


To list all configured bindings and system links,
use the following command:

    rpctool list bindings
    rpctool list links


NOTE:  Will require the rpcProxy application to be restart in order for it
       take effect.
