#!/bin/sh


# do one round of sampleMemPools
# move log files to a separate dir
# run v on all log files and generate a summary file
# grep the summary file - execptation is have some race condition msgs only.
# clean up (delete log files)

logDir=__InspectMemoryPool_testRace_log_deleteme
summaryFile=__InspectMemoryPool_testRace_summary_deleteme

./sampleMemPools

mkdir -p $logDir

mv sampleMemPools_log_* $logDir

./v 100000 $logDir > $summaryFile


if grep -q "Race Condition: declared false complete list" $summaryFile
then
    echo "[FAILED] Inspect declared a flase complete list"
    exit 1
fi


if grep -q "No declaration of list completion - something is wrong" $summaryFile
then
    echo "[FAILED] Inspect didn't declare any completion status msg"
    exit 1
fi


if ! grep -q "Race Condition: declared incomplete list and the last item reported is wrong" $summaryFile
then
    echo "[FAILED] No occurances of race condition - it's so wonderful that it's wrong."
    exit 1
fi


# clean up
rm -rf $logDir
rm $summaryFile

exit 0
