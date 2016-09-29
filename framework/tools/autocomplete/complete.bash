#!/usr/bin/env bash


DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi
. "$DIR/common.sh"


function _app()
{
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD-1]}"

    # complete the 1st argument with commands
    if [[ $COMP_CWORD == 1 ]]; then
        COMPREPLY=( $(compgen -W "start stop restart remove status version info list runProc" -- "$cur") )
        return 0
    # complete the 2nd argument with running/stopped/all apps (depending on command)
    elif [[ $COMP_CWORD == 2 ]]; then
        case $prev in
            list )
                return 0 # don't complete command "list"
            ;;
            stop | restart )
                list_running_apps
            ;;
            start )
                list_stopped_apps
            ;;
            * )
                list_apps
            ;;
        esac
        COMPREPLY=( $(compgen -W '"${reply[@]}"' -- $cur) )
        return 0
    fi
}

complete -F _app app

# complete with files and $LEGATO_ROOT/build targets
function _instlegato()
{
    local cur="${COMP_WORDS[COMP_CWORD]}"
    list_instlegato_targets
    COMPREPLY=( $(compgen -f -W '"${reply[@]}"' -- $cur) )
}

complete -F _instlegato instlegato


function _fwupdate()
{
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD-1]}"

    # complete the 1st argument with commands
    if [[ $COMP_CWORD == 1 ]]; then
        COMPREPLY=( $(compgen -W "download query help" -- "$cur") )
    elif [[ $COMP_CWORD == 2 ]] && [[ $prev == "download" ]]; then
        _filedir
    fi

}

complete -F _fwupdate fwupdate
