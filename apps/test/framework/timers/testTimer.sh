cd /mnt/userrw/timerTest
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.
./testTimer 2> out_`date +%y%m%d_%H%M%S`.txt
