#!/usr/bin/env zsh

my_dir="$(dirname "$0")"
source "$my_dir/common.sh"

# complete the 1st argument with commands
# complete the 2nd argument with running/stopped/all apps (depending on command)
# don't complete command "list"

compctl \
    -x 'p[1,1]' -k "(start stop restart remove status version info list runProc)" -- \
    + -x 'p[2,2] W[1,start]' -K list_stopped_apps -- \
    + -x 'p[2,2] W[1,stop]' -K list_running_apps -- \
    + -x 'p[2,2] W[1,restart]' -K list_running_apps -- \
    + -x 'p[2,2] W[1,^list]' -K list_apps -- \
    app

# complete with files and $LEGATO_ROOT/build targets
compctl -x 'p[1,1]' -f -K list_instlegato_targets -- \
    instlegato

# complete the 1st argument with commands
compctl \
    -x 'p[1,1]' -k "(download query help)" -- \
    fwupdate
