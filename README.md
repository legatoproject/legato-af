![Legato](https://legato.io/resources/img/legato_logo.png)

---

![Build Status](https://travis-ci.org/legatoproject/legato-af.svg)

Legato Open Source Project is an initiative by Sierra Wireless Inc. which provides an open, secure
and easy to use Application Framework for embedded devices. The project enables developers who are
not experienced in traditional embedded programming to participate in the exponential growth of
the "Internet of Things".
Visit [legato.io](https://legato.io) to learn more or visit the [Legato forum](https://forum.legato.io).

### Prerequisites

  - A [maintained](https://wiki.ubuntu.com/Releases) Long Term Support (LTS) version of Ubuntu
  - Install required packages:

```bash
$ sudo apt-get install -y   \
    autoconf                \
    automake                \
    bash                    \
    bc                      \
    bison                   \
    bsdiff                  \
    build-essential         \
    chrpath                 \
    cmake                   \
    cpio                    \
    diffstat                \
    flex                    \
    gawk                    \
    gcovr                   \
    git                     \
    gperf                   \
    iputils-ping            \
    libbz2-dev              \
    libcurl4-gnutls-dev     \
    libncurses5-dev         \
    libncursesw5-dev        \
    libsdl-dev              \
    libssl-dev              \
    libtool                 \
    libxml2-utils           \
    ninja-build             \
    python                  \
    python-git              \
    python-jinja2           \
    python-pkg-resources    \
    python3                 \
    texinfo                 \
    unzip                   \
    wget                    \
    zlib1g-dev
```

Optional packages: ```openjdk-8-jdk``` (for Java support, at least Java 8 is required),
                   ```doxygen graphviz``` (for doc generation),
                   ```xsltproc``` (for running tests)

  - Cross-build toolchain(s)<br/>
    For Sierra Wireless platforms, toolchains are available at https://source.sierrawireless.com/resources/legato/downloads/

### Installation

#### Clone from GitHub

Legato uses [git-repo](https://code.google.com/p/git-repo/) as it is distributed as multiple repositories.

1. Install [repo](https://code.google.com/p/git-repo/):

  (on Ubuntu >= 16.04)
  ```bash
  $ sudo apt-get install -y repo
  ```
  OR
  (on Ubuntu < 16.04)
  ```bash
  $ sudo apt-get install -y phablet-tools
  ```
  OR
  ```bash
  $ wget -O ~/bin/repo https://storage.googleapis.com/git-repo-downloads/repo
  $ chmod a+x ~/bin/repo
  ```

2. Clone the environment:
  ```bash
  $ mkdir workspace
  $ cd workspace
  $ repo init -u git://github.com/legatoproject/manifest
  $ repo sync
  ```

  You can also clone a specific release:
  ```bash
  $ repo init -u git://github.com/legatoproject/manifest -m legato/releases/16.07.0.xml
  $ repo sync
  ```


#### Install the Legato framework on your development PC

  1. Clone it from GitHub or untar a release archive into a directory

  2. ```cd``` into that directory

  3. Run ```make```

#### Configure your bash shell's environment for the Legato application build tools

Source ```bin/configlegatoenv```:
```bash
$ . bin/configlegatoenv
```
OR, run the interactive bash shell ```bin/legs```:
```
$ bin/legs
```

### Run on Target Devices

#### Build support for cross-build targets, run [```make <target>```](https://docs.legato.io/latest/basicBuildLegato_make.html).

For example, to enable support for the Sierra Wireless WP85xx devices, run ```make wp85```.<br/>
Of course, each of these depends on the cross-build toolchain for that target,
so ensure that you have the appropriate toolchain installed first.

The path to your toolchain and the prefix of the name of the tools in the toolchain
are specified using the ```xxxxxx_TOOLCHAIN_DIR``` and ```xxxxxx_TOOLCHAIN_PREFIX``` environment variables
(where ```xxxxxx``` is replaced with the target platform's ID, such as WP85).

If your toolchain is installed somewhere other than the default location under ```/opt/swi```,
ensure that the appropriate environment variables are set to tell the build tools where to find
your toolchain and what its prefix is.<br/>
For example, for Sierra Wireless WP85xx devices, ```WP85_TOOLCHAIN_DIR``` must be set to the
path of the directory that contains the file ```arm-poky-linux-gnueabi-gcc```, and
```WP85_TOOLCHAIN_PREFIX``` must be set to ```arm-poky-linux-gnueabi--```.

Following is a list of supported cross-build targets:

Target  |  Description                    | Environment variables
:-------|---------------------------------|:-------------------------------------------------------
 ar7    | Sierra Wireless AR755x module   | ```AR7_TOOLCHAIN_DIR```,```AR7_TOOLCHAIN_PREFIX```
 ar758x | Sierra Wireless AR758x module   | ```AR758X_TOOLCHAIN_DIR```,```AR758X_TOOLCHAIN_PREFIX```
 ar759x | Sierra Wireless AR759x module   | ```AR759X_TOOLCHAIN_DIR```,```AR759X_TOOLCHAIN_PREFIX```
 ar86   | Sierra Wireless AR86xx module   | ```AR86_TOOLCHAIN_DIR```,```AR86_TOOLCHAIN_PREFIX```
 wp85   | Sierra Wireless WP85xx module   | ```WP85_TOOLCHAIN_DIR```,```WP85_TOOLCHAIN_PREFIX```
 wp750x | Sierra Wireless WP750x module   | ```WP750X_TOOLCHAIN_DIR```,```WP750X_TOOLCHAIN_PREFIX```
 wp76xx | Sierra Wireless WP76xx module   | ```WP76XX_TOOLCHAIN_DIR```,```WP76XX_TOOLCHAIN_PREFIX```
 wp77xx | Sierra Wireless WP77xx module   | ```WP77XX_TOOLCHAIN_DIR```,```WP77XX_TOOLCHAIN_PREFIX```
 raspi  | Raspberry Pi                    | ```RASPI_TOOLCHAIN_DIR```,```RASPI_TOOLCHAIN_PREFIX```

### Directory Structure

 The top level directory structure is as follows:

   ```./apps``` - contains source code for apps.

   ```./bin``` - created by build system and populated with executable files that run on the development
           host (the host that ran the build).

   ```./build``` - contains the results of the build.  Will be created by the build system.

   ```./build/tools``` - contains build tools that are built and then used by the build system on
                    the build host to build other things.

   ```./build/<target>``` - contains the output of a build for a specific target (e.g., ./build/wp85).

   ```./cmake``` - contains CMake scripts used by the build system to build samples and unit tests.

   ```./components``` - contains platform-independent components that are used in apps.

   ```./framework``` - contains the source code for the Legato application framework itself.

   ```./targetFiles``` - contains some files for installation on target devices that don't need
                    to be built.

   ```./platformAdaptor``` - contains components that are specific to certain platforms.

   ```./modules``` - contains other repositories that are extending Legato.

### Documentation

Once you have completed the first three installation steps above, you will find a set of
HTML documentation under the "Documentation" directory.<br/>
Point your web browser at ```Documentation/index.html``` to view it:
```
xdg-open Documentatation/index.html
```

The latest release documentation is available at: https://docs.legato.io/latest/

### Uninstallation

To uninstall Legato from your development PC:

  - Delete the directory you unzipped Legato under
  - Revert any changes you may have made to your ```.bashrc```, etc. to set up ```xxxxxx_TOOLCHAIN_DIR```
    environment variables.

* * *
_Copyright (C) Sierra Wireless Inc. _
