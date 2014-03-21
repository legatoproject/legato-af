#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

#! /bin/sh

BINDIR=$(dirname $(readlink -f $0))
RELDIR=$(pwd)/release
TEMPDIR=$(pwd)/temp
STDOUT=$TEMPDIR/cmdout


while getopts 'u:p:r:n' OPTION
do
    case $OPTION in
    u)    USER="$OPTARG"
                ;;
    p)    PASSWORD="$OPTARG"
                ;;
    r)    REV="$OPTARG"
                ;;
    n)    DRYRUN="yes"
                ;;
    *)    printf "Usage: %s: [-u user] [-p password] [-r releaseversion]\n" $(basename $0) >&2
            exit 2
                ;;
    esac
done

# Check input params
if  [ ! $USER ]
then
    USER=$(whoami)
fi

if  [ ! $PASSWORD ]
then
    printf "Enter $USER password: "
    stty -echo
    read -r PASSWORD
    stty echo
    echo ""         # force a carriage return to be output
fi

while  [ ! $REV ]
do
    printf "Enter release version: "
    read REV
done

MAJOR_VERSION=$(echo $REV | sed -n 's/^\([^\."]*\)\.[^\.\"]*$/\1/p')
MINOR_VERSION=$(echo $REV | sed -n 's/^[^\.\"]*\.\([^\.\"]*\)$/\1/p')

if  [ ! $MAJOR_VERSION ] || [ ! $MINOR_VERSION ]
then
    echo "Version must be in the form of MAJOR.MINOR, where MAJOR and MINOR do not contain chars '.\"'"
    exit 1
fi

# Check necessary tools to be present on the system
REQEXEC="doxygen ant java javadoc gcc cmake"
for f in $REQEXEC
do
    if ! which "$f" > /dev/null
    then
        echo "Progam $f is missing, please install it and relaunch this script"
        exit 1
    fi
done



# Utilitary functions
require(){
    if ! $@ > $STDOUT
    then
        echo "Command Failed, see $STDOUT for more detail"
        exit 1
    fi
}

printstep() {
    printf "$1 ... "
}

printok() {
    echo "ok"
}





# Start the actual work !
mkdir -p $TEMPDIR
printstep "Make Agent $MAJOR_VERSION.$MINOR_VERSION Release packages"
rm -fr $RELDIR
require mkdir -p $RELDIR
require cd $RELDIR
printok


# Prepare the source package
SRCDIR=$RELDIR/sources

# compute SVN pasword option
SVNPASS=${PASSWORD:+"--password $PASSWORD"}


# Lock SVN to make sure nobody is going to spoil around while setting the new revision
#printstep "Locking SVN repo"
#require svn --username $USER $SVNPASS lock https://svn.anyware/platform-embedded/trunk
#printok

echo $TEMPDIR

printstep "Setting Release version into the source tree"
require svn --username $USER $SVNPASS co -q https://svn.anyware/platform-embedded/trunk/ReadyAgent/c/common $TEMPDIR
cat $TEMPDIR/version.h | sed  "s;\(.*MAJOR_VERSION.*\)\"\(.*\)\";\1\"$MAJOR_VERSION\";" | sed  "s;\(.*MINOR_VERSION.*\)\"\(.*\)\";\1\"$MINOR_VERSION\";" > $TEMPDIR/tempversion
require mv $TEMPDIR/tempversion $TEMPDIR/version.h
echo "" # add a new line
svn diff $TEMPDIR/version.h
printf "Confirm diff and commit the new version? (N): "
read CONTINUE
CONTINUE=$(echo "$CONTINUE" | tr '[:upper:]' '[:lower:]')
if [ "$CONTINUE" != "y" ] && [ "$CONTINUE" != "yes" ]
then
    exit 0
fi
if [ ! $DRYRUN ]
then
    svn --username $USER $SVNPASS commit -m "Release of the Agent R$REV" -q $TEMPDIR/version.h
else
    printf "dryrun"
fi
printok

TAG="Agent-R$REV"
printstep "Branching trunk to create the tag $TAG"
if [ ! $DRYRUN ]
then
    svn --username $USER $SVNPASS copy -q -m "Create tag $TAG for Agent release" https://svn.anyware/platform-embedded/trunk https://svn.anyware/platform-embedded/tags/$TAG
else
    printf "dryrun"
fi
printok


printstep "Checking out sources and build the Agent"
require svn --username $USER $SVNPASS export -q https://svn.anyware/platform-embedded/trunk $SRCDIR
printok

# Unlock the SVN, commit can go on...
#printstep "Unlocking SVN repo"
#require svn --username $USER $SVNPASS unlock https://svn.anyware/platform-embedded/trunk
#printok


printstep "Clean source tree"
require rm -fr $SRCDIR/demos/schneider-firefox
require rm -fr $SRCDIR/demos/fsu
require rm -fr $SRCDIR/demos/M2MBoxPro_End2End_tests
require rm -fr $SRCDIR/hudson
require rm -fr $SRCDIR/tools/AwtDaTestU
require rm -fr $SRCDIR/tools/BandwithEstimation
require rm -fr $SRCDIR/tools/DataModelParser
require rm -fr $SRCDIR/tools/DeviceSimulator
require rm -fr $SRCDIR/tools/M2MBoxPro
require rm -fr $SRCDIR/luafwk/serialframework/roadrunner
require rm -fr $SRCDIR/luafwk/serialframework/elga
require rm -fr $SRCDIR/libs/c/SerialFramework/Elga
require rm -fr $SRCDIR/libs/c/SerialFramework/Atlas
require rm -fr $SRCDIR/libs/c/SerialFramework/Roadrunner
printok

printstep "Build the documentation"
require cd $RELDIR
require $SRCDIR/bin/build.sh
require cd build.default
require make ldoc_gen javadoc_gen doxygen_gen
DOCDIR=$RELDIR/UserGuide/APIDoc
require mkdir -p $DOCDIR
require mkdir -p $DOCDIR/lua
require cp -r $SRCDIR/doc/ldoc/Lua_User_API_doc/* $DOCDIR/lua
require rm -rf $SRCDIR/doc/ldoc/Lua_User_API_doc
require mkdir -p $DOCDIR/java
require cp -r $SRCDIR/doc/javadoc/Java_User_API_doc/* $DOCDIR/java
require rm -rf $SRCDIR/doc/ldoc/Java_User_API_doc
require mkdir -p $DOCDIR/c
require cp -r $SRCDIR/doc/doxygen/C_User_API_doc/html/* $DOCDIR/c
require rm -rf $SRCDIR/doc/ldoc/C_User_API_doc
require cd $RELDIR
require mkdir $DOCDIR/../CodeSamples
require cp -fr $SRCDIR/demos/samples/* $DOCDIR/../CodeSamples
printok


printstep "Build the Agent"
require cd $RELDIR
require $SRCDIR/bin/build.sh
require cd build.default
require make lua luac
require cd $RELDIR
require mv build.default/runtime runtime
require rm -fr build.default
require "$BINDIR/precompile.sh" runtime/lua runtime/bin/luac
printok


printstep "Make source and binary tarballs"
require tar -cjf "Agent-R$REV-Linux-x86.tar.bz2" runtime
require tar -cjf "Agent-R$REV-sources.tar.bz2" sources
require rm -fr $SRCDIR
require rm -fr runtime
printok


#printstep "Fetch docs from Confluence"
#require wget --no-check-certificate https://confluence.anyware-tech.com/download/attachments/2621752/ReadyAgent+API+User+Guide.pdf
#require wget --no-check-certificate https://confluence.anyware-tech.com/download/attachments/13140115/ReadyAgent+Features.pdf


