BEGIN {
    failure=0
}

{
    if ($0 ~ /\[START\]List of Failure/) {
        failure=1
    }
    if ($0 ~ /\[STOP\]List of Failure/) {
        print
        failure=0
    }
    if (failure==1) {
        print
    }
}
