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

#!/bin/sh

if [ \( ! -d "$1"  \) -o  \( ! -x "$2" \) ]
then
    echo "Usage:\n\t $0 directorytocompile compilerpath"
    exit 1
fi

echo "Compiles lua files in $1 with compiler $2"

files=$(find "$1" -name "*.lua")

for file in $files
do
    echo "Compiling $file"
    "$2" -s -o "$file" "$file"
done
