Minimum requirements
====================

In order to build properly the C source code, you will need the
following components to be installed:

* cmake (\>= 2.8.3)
* gcc
* libc development files
* readline library (optional)

#### Ubuntu/Debian
You can install those by installing packages: cmake
build-essential libreadline5-dev

```bash
$ apt-get install cmake build-essential libreadline5-dev
```

#### Fedora
You have to install the "Development Tools" package
group and the cmake and readline-devel packages

```bash
$ yum groupinstall "Development Tools"
$ yum install cmake readline-devel
```

#### Arch
You can install those by installing all packages except the libc6 which is already installed
in the base system (development files included)

```bash
$ pacman -S gcc cmake readline
```

Configure build system
======================

From now on, we'll use MihiniAgentSources as the
root directory of Mihini agent sources. It must contains folders like
"bin", "cmake", "luafmk", "agent" etc.

MihiniAgentSources/cmake folder contain toolchain files (named
toolchain.\*) along with other files (but don't pay attention at other
files in cmake folder).
Each toolchain file describes a set of tools to compile sources for a given target:

- (cross-)compiler configuration (most important ones)

```cmake
SET(CMAKE_C_COMPILER /path/to/gcc)
SET(CMAKE_CXX_COMPILER /path/to/g++)
# where is the target environment
SET(CMAKE_FIND_ROOT_PATH /path/to/root/env)
```
- other build options related to Mihini Agent integration (those are more likely to be used for fine tuning a Mihini Agent integration, after it is already compiled and running)

you may want to write your own toolchain file, to handle the specifics of your environment.

> **WARNING**
>
> * We strongly recommend that you use a the toolchain that was generated
>   while creating the kernel/rootfs of the Linux system running on your
>   device !
> * When not cross compiling (the executable is intended to run on the compiling PC),
>   the "build-essential" package or its equivalent provides most of what's needed;
>   as a result, toolchain.default.cmake is almost empty.


Build
=====

#### Build the software

You are strongly advised to run next line outside from MihiniAgentSources folder
Output files are produced on working\_dir/build.$target folder, with
$target equals to target specified in build.sh -t argument.

```bash
MihiniAgentSources/bin/build.sh [-d] [-m] [-t <target>]
```

* Use `-d` to toggle debugging on
* Use `-m` to compress build artifacts once compilation is completed (see the corresponding section below)
* Use `-t <target>` to specify the build target. When no target is
  specified, the default target (named "default") is used: it uses the
  compiler for the host computer running the build.sh script, using
  gcc/g++.

Available targets are the ones corresponding to the toolchain files in
`cmake` directory. Using non default targets will require to install
additional cross compiling toolchains.

#### Build artifacts details

- build.$target/runtime : the runtime folder contains all Mihini Agent artifacts that need to be integrated in the target device.
- build.$target/runtime/lib contains the libs you may need to link
  with when developing an application that works with the Mihini Agent, like:
   - libSwi\_AirVantage.so
   - libSwi\_Sms.so
   - libSwi\_System.so
   - ...
- build.$target/runtime/lua contains the lua packages and the lua native libs used by the agent
- other files and folders in build.$target folder are CMake artifacts,
  don't modify them, you shouldn't need to look at them.

Run Mihini Agent
==============

Just start the agent in a command line terminal:

```bash
$ cd build.default/runtime
$ ./start.sh
```

You should see the logs of the Mihini Agent, something similar to:

```bash
2013-03-12 16:14:48 GENERAL-INFO: ************************************************************
2013-03-12 16:14:48 GENERAL-INFO: Starting Agent ...
2013-03-12 16:14:48 GENERAL-INFO:      Agent: 0.9-DEV - Build: b660318
2013-03-12 16:14:48 GENERAL-INFO:      Lua VM: Lua 5.1.4 (+meta pairs/ipairs) (+patch-lua-5.1.4-3)
2013-03-12 16:14:48 GENERAL-INFO:      System: Linux arch 3.7.10-1-ARCH #1 SMP PREEMPT Thu Feb 28 09:50:17 CET 2013 x86_64
2013-03-12 16:14:48 GENERAL-INFO: ************************************************************
2013-03-12 16:14:48 GENERAL-INFO: Migration executed successfully
2013-03-12 16:14:48 GENERAL-INFO: Module [AssetConnector] initialized
2013-03-12 16:14:48 SHELL-INFO: Binding a shell server at address ?localhost, port 2000
2013-03-12 16:14:48 GENERAL-INFO: Module [Lua Shell] initialized
2013-03-12 16:14:48 GENERAL-INFO: Module [DummyNetman] initialized
2013-03-12 16:14:48 GENERAL-INFO: Module [Lua RPC] initialized
2013-03-12 16:14:48 GENERAL-INFO: Module [ServerConnector] initialized
2013-03-12 16:14:48 GENERAL-INFO: Module [DataManagement] initialized
2013-03-12 16:14:48 ASSCON-INFO: Connection received from asset [table: 0xf0a2e0] at '<local ipc=table: 0xed25c0>:0'
2013-03-12 16:14:48 ASSCON-INFO: Asset registered, name="@sys", id=table: 0xf0a2e0.
2013-03-12 16:14:48 GENERAL-INFO: Module [DeviceManagement] initialized
2013-03-12 16:14:48 GENERAL-INFO: Module [ApplicationContainer] initialized
2013-03-12 16:14:48 GENERAL-INFO: Module [Update] initialized
2013-03-12 16:14:48 GENERAL-INFO: Agent successfully initialized
```

> **INFO**
>
> According to the version you downloaded and you built, the header of the Agent might change (it includes version, git revision)


#### Mihini Agent shell

You can interact with the Mihini Agent by connecting to the Lua shell.
 When default settings are used, this is how to do it:

```bash
$ telnet localhost
```

Once connected to the shell, you can execute Lua instructions.
For instance, you can change a Mihini Agent config parameter:

```bash
$ telnet localhost
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
Lua interactive shell
> agent.config.network.activate = false
>
```

#### Mihini Agent configuration

To modify some of the Mihini Agent configuration parameters (like
communication ports, ...) you'll have to change the Mihini Agent Config.\
 This is explained in ConfigStore page.

Update build artifacts after source modifications
======================================================

This also applies when one wants to build artifacts that are not built
by default.

```bash
#go to the build folder
cd build.default
# this one rebuilds all default targets if their dependencies have change.
# Also it rebuilds cmake rules if those have changed.
make
# this one builds a specific target
make some-target
```

Compressing build artifacts
===========================

According to the device used, it might be useful to save space for the build artifacts.
In order to turn on runtime compression, you just need to invoke the script bin/build.sh
with `-m` as argument.

```bash
$ bin/build.sh -m
>>> Set Minimum Size release option to TRUE
[...]
```

For the time being, activating this option does the following things:

* Build the native code with CFLAGS optimized for size (-Os -s)
* Minify lua code

Depending on your toolchain and the target used, the produced build artifacts
might be different in size.

