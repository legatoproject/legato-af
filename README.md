Welcome to Legato!
==================

#### Dependencies

  - Ubuntu 12.04 or newer.
  - Install the packages required:

        sudo apt-get install bison build-essential chrpath cifs-utils cmake \
        coreutils curl desktop-file-utils diffstat docbook-utils doxygen \
        fakeroot flex g++ gawk gcc git-core gitk graphviz help2man \
        libgmp3-dev libmpfr-dev libreadline6-dev libtool libxml2-dev \
        libxml-libxml-perl make m4 ninja-build python-pip python-jinja2 \
        python-pysqlite2 quilt samba scons sed subversion texi2html texinfo \
        unzip wget ninja-build

  - [Cross toolchain(s)](http://source.sierrawireless.com/resources/legato/downloads/) 

#### Installation

 To install the Legato framework on your development PC,

  1. unzip/untar it into a directory

  2. cd into that directory

  3. run ```make```

  4. To configure your bash shell's environment for the Legato application build tools,
     source ```bin/configlegatoenv```:

        $ . bin/configlegatoenv

    OR, run the interactive bash shell ```bin/legs```:

        $ bin/legs

  5. To build support for cross-build targets, run ```make <target>```.
    For example, to enable support for the Sierra Wireless AR7xxx devices, run ```make ar7```.
    Of course, each of these depends on the cross-build tool chain for that target,
    so ensure that you have the appropriate tool chain installed first.

    If your toolchain is installed somewhere other than under ```/opt/swi```,
    ensure that the appropriate environment variable is set to the path of
    the directory containing your toolchain.
    For example, for AR7 devices, ```AR7_TOOLCHAIN_DIR``` must be set to the path of the directory
    that contains the file ```arm-poky-linux-gnuabi-gcc```.

     Following is a list of supported cross-build targets:

Target |  Description                    | Environment variable
:------|---------------------------------|:-----------------------
  ar7  | Sierra Wireless AR7xxx module   | ```AR7_TOOLCHAIN_DIR```
  ar86 | Sierra Wireless AR86xx module   | ```AR86_TOOLCHAIN_DIR```
  wp85 | Sierra Wireless WP85xx module   | ```WP85_TOOLCHAIN_DIR```

#### Documentation

 Once you have completed the first three installation steps above, you will find a set of
 HTML documentation under the "Documentation" directory.  Point your web browser at
 ```Documentation/index.html``` to view it.

#### Uninstallation

 To uninstall Legato from your development PC:

  - Delete the directory you unzipped Legato under
  - Revert any changes you may have made to your ```.bashrc```, etc. to set up ```XXX_TOOLCHAIN_DIR```
    environment variables.


#### Directory Structure

 The top level directory structure is as follows:

   ```./apps``` - contains source code for apps.

   ```./bin``` - created by build system and populated with executable files that run on the development
           host (the host that ran the build).

   ```./build``` - contains the results of a framework build.  Will be created by the build system.

   ```./build/tools``` - contains tools that are built and then used by the build system to build
                   other things.

   ```./build/<target>``` - contains the output of a build for a specific target (e.g., ./build/wp85).

   ```./cmake``` - contains CMake scripts used by the build system.

   ```./framework``` - contains the source code for the Legato framework itself.

   ```./targetFiles``` - contains files that are for installation on target devices.

* * *
_Copyright (C) Sierra Wireless Inc. Use of this work is subject to license._
