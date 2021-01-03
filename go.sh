#!/bin/bash
#./socketcan-raw-demo -f -1 can0 -2 can1
#./socketcan-raw-demo -f -1 can0 -2 can1
./socketcan-raw-demo -f -1 vcan0 -2 vcan1 &

while (:)
do
/home/smooker/src/can-utils/build/./cansend vcan0 5A1\#11.2233.44556677.88
sleep 1
done
