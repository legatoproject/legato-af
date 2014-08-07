cd /tmp/testPos
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.
./serviceDirectory &
sleep 1
./logCtrlDaemon &
sleep 1
./testPaQmiPos
