unset IFS

# functions for completions

function list_apps()
{
    if [ -n "$DEST_IP" ]; then
        IFS=$'\n' reply=(`app list 2> /dev/null`)
    fi
}

function list_stopped_apps()
{
    if [ -n "$DEST_IP" ]; then
        # get app statuses that start with [stopped]
        # and extract the 2nd field (app name)
        IFS=$'\n' reply=(`app status 2> /dev/null | grep "^\[stopped]" | cut -d " " -f 2`)
    fi
}

function list_running_apps()
{
    if [ -n "$DEST_IP" ]; then
        # get app statuses that start with [running]
        # and extract the 2nd field (app name)
        IFS=$'\n' reply=(`app status 2> /dev/null | grep "^\[running]" | cut -d " " -f 2 2> /dev/null`)
    fi
}

function list_instlegato_targets()
{
    reply=()
    # get directories in $LEGATO_ROOT/build/ that contain a .update file
    if [ -n "$LEGATO_ROOT" ] && [ -d "$LEGATO_ROOT/build" ]; then
        for D in `IFS=$'\n' find $LEGATO_ROOT/build/ -maxdepth 1 -mindepth 1 -type d -printf "%f\n"`; do
            if [ -e "$LEGATO_ROOT/build/$D/system.$D.update" ]; then
                reply+=("$D")
            fi
        done
    fi
}
