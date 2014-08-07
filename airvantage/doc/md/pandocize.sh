#!/bin/sh

#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# and Eclipse Distribution License v1.0 which accompany this distribution.
#
# The Eclipse Public License is available at
#   http://www.eclipse.org/legal/epl-v10.html
# The Eclipse Distribution License is available at
#   http://www.eclipse.org/org/documents/edl-v10.php
#
# Contributors:
#     Romain Perier for Sierra Wireless - initial API and implementation
#     Benjamin Cabé for Sierra Wireless - bug 407919
#*******************************************************************************

BASEDIR=$(cd $(dirname $0) && pwd)
OUTPUT_DIR=$1/specs
mkdir -p $1

rm -fr $OUTPUT_DIR

markdown_list="
    agent/ConfigStore.md
    agent/Appmon_Daemon.md
    agent/security.md
    agent/Logging_framework.md
    agent/Migration.md
    agent/Migration_MigrationScript_c.md
    agent/Migration_MigrationScript_h.md
    agent/Migration_CMakeLists_txt.md
    agent/Network_Manager.md
    agent/Software_Update_Framework.md
    agent/Software_Update_Module.md
    agent/Software_Update_Package.md
    agent/TCP_Remote_Connection.md
    agent/Time_services.md
    agent/Application_Container.md
    agent/Device_Management.md
    agent/Tree_Manager.md
    agent/Using_treemgr_handlers_for_asset_management.md
    agent/Remote_Script.md
    agent/Rest.md
    agent/Monitoring.md
    agent_connector_libraries/Racon_Lua_library.md
    agent_connector_libraries/Agent_Connector_Library.md
    agent_connector_libraries/Embedded_Micro_Protocol_EMP.md
    agent_connector_libraries/Agent_UserGuide.md
    utilitary_libraries/Lua_RPC.md
    utilitary_libraries/Modbus.md
    utilitary_libraries/Serialize_Deserialize_Lua_objects.md
    ./index.md
"

source_dir=$BASEDIR


for md in $markdown_list; do
    output=$(echo $md | sed 's:\.md::')
    category_dir=$(echo $md | sed 's:/.*::')
    test -d $category_dir || mkdir $category_dir

    if [ -d ${source_dir}/${category_dir}/images ]  && [ ! -e ${category_dir}/images ]; then
	    ln -s ${source_dir}/${category_dir}/images ${category_dir}/images
    fi

    if [ ! -e ${category_dir}/default.css ]; then
        ln -s ${source_dir}/default.css ${category_dir}/default.css
    fi

    echo "${source_dir}/$md"
    pandoc --standalone --css="default.css" --highlight-style=tango ${source_dir}/$md -o ${output}.html || exit 1 
done

cp -fr . $OUTPUT_DIR
