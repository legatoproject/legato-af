Welcome to Legato!
==================

![Build Status](https://travis-ci.org/legatoproject/legato-af.svg)

#### Dependencies

  - A [maintained](https://wiki.ubuntu.com/Releases) Long Term Support (LTS) version of Ubuntu
  - Install required packages:

```bash
$ sudo apt-get install build-essential \
                       python python-jinja2 python-pkg-resources cmake \
                       git subversion libsdl-dev \
                       diffstat texinfo gawk chrpath wget cpio \
                       vim zsh bash ninja-build screen sshpass \
                       bc python-git unzip libxml2-utils gcovr \
                       libcurl4-gnutls-dev zlib1g-dev libbz2-dev \
                       bsdiff libssl-dev autoconf automake
```

Optional packages: ```vim``` (or any text editor),
                   ```zsh``` (command interpreter for shell scripting),
                   ```openjdk-8-jdk``` (for Java support),
                   ```doxygen graphviz``` (for doc generation)

  - Cross-build toolchain(s)<br/>
    For Sierra Wireless platforms, toolchains are available at http://source.sierrawireless.com/resources/legato/downloads/

#### Clone from GitHub

Legato uses [git-repo](https://code.google.com/p/git-repo/) as it is distributed as multiple repositories.

1. Install [repo](https://code.google.com/p/git-repo/):

  (on Ubuntu > 14.04)
  ```bash
  $ sudo apt-get install phablet-tools
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

#### Installation

##### To install the Legato framework on your development PC

  1. Clone it from GitHub or untar a release archive into a directory

  2. ```cd``` into that directory

  3. Run ```make```

##### To configure your bash shell's environment for the Legato application build tools

Source ```bin/configlegatoenv```:
```bash
$ . bin/configlegatoenv
```
OR, run the interactive bash shell ```bin/legs```:
```
$ bin/legs
```

##### To build support for cross-build targets, run [```make <target>```](http://legato.io/legato-docs/latest/basicBuildLegato_make.html).

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
path of the directory that contains the file ```arm-poky-linux-gnuabi-gcc```, and
```WP85_TOOLCHAIN_PREFIX``` must be set to ```arm-poky-linux-gnuabi-```.

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
 em75xx | Sierra Wireless EM75xx module   | ```EM75XX_TOOLCHAIN_DIR```,```EM75XX_TOOLCHAIN_PREFIX```
 raspi  | Raspberry Pi                    | ```RASPI_TOOLCHAIN_DIR```,```RASPI_TOOLCHAIN_PREFIX```

#### Documentation

Once you have completed the first three installation steps above, you will find a set of
HTML documentation under the "Documentation" directory.<br/>
Point your web browser at ```Documentation/index.html``` to view it:
```
xdg-open Documentatation/index.html
```

The latest release documentation is available at: http://legato.io/legato-docs/latest/

#### Uninstallation

To uninstall Legato from your development PC:

  - Delete the directory you unzipped Legato under
  - Revert any changes you may have made to your ```.bashrc```, etc. to set up ```xxxxxx_TOOLCHAIN_DIR```
    environment variables.

#### Directory Structure

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

* * *
_Copyright (C) Sierra Wireless Inc. _
