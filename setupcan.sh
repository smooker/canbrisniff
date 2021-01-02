#!/bin/bash

#set -e 

ip link set can0 type can help

ip link set can0 down
ip link set can0 type can bitrate 500000 triple-sampling off listen-only off loopback off restart-ms 500 sample-point 0.875 sjw 2 phase-seg2 4 phase-seg1 4 prop-seg 3
# fd-non-iso off
# fd off 
#one-shot off  loopback off
ip link set can0 up

ip link set can1 down
ip link set can1 type can bitrate 500000 triple-sampling off listen-only off loopback off restart-ms 500 sample-point 0.875 sjw 2 phase-seg2 4 phase-seg1 4 prop-seg 3
# fd-non-iso off
# fd off
# one-shot off loopback off
##ip link set can1 type can bitrate 125000 triple-sampling on
ip link set can1 up


#/usr/src/can-utils/candump can0
#sleep 1
echo sniffing....
./sniff.sh
exit
