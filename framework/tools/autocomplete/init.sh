#!/bin/sh

if [ -n "$BASH_VERSION" ]; then
    root="$(dirname "${BASH_SOURCE[0]}")"
    source "$root/complete.bash"

elif [ -n "$ZSH_VERSION" ]; then
    root="$(dirname "$0")"
    source "$root/complete.zsh"
fi
