#!/bin/sh
# Script to start the test of SignalShowStack on several signals.
# Like the test crashes due to these signals, check the returned
# code according to the signal raised.

exe=$1
test -z "$1" && exe=`dirname $0`/`basename $0 .sh`

# Check if /proc/cpu/alignment exists to run the BUS test
[ -e /proc/cpu/alignment ] && BUS="BUS"

# CRUSH does not raise SIGSEGV on x86_64/i686 platforms
(uname -m | grep -q '^arm') && CRUSH="CRUSH"

# Disable core dump generation
ulimit -c 0

# Sure to enable the signal show info handler
unset SIGNAL_SHOW_INFO

for sig in ILL $BUS FPE SEGV $CRUSH ABRT
do
    ok=0
    $exe $sig
    rc=$?
    case $rc in
       132) test "$sig" = "ILL" && ok=1 ;;
       134) test "$sig" = "ABRT" && ok=1 ;;
       135) test "$sig" = "BUS" && ok=1 ;;
       136) test "$sig" = "FPE" && ok=1 ;;
       139) test "$sig" = "SEGV" -o "$sig" = "CRUSH" && ok=1 ;;
    esac
    if [ 1 -ne $ok ]
    then
        echo "Test failed for signal $sig: rc=$rc"
        exit 1
    fi
    echo "OK on signal $sig"...
    echo ""
done

echo "Success"
exit 0
