#!/bin/bash
#!/bin/bash
trap ctrl_c INT

function ctrl_c() {
        echo "** Trapped CTRL-C"
		exit
}

while(:)
do
#stdbuf -i0 -o0 -e0 /usr/src/can-utils/candump -t A -b can1 -caexd can0,0:0,#FFFFFFFF
#stdbuf -i0 -o0 -e0 /usr/src/can-utils/candump -cae can0,0:0,#FFFFFFFF
#stdbuf -i0 -o0 -e0 /usr/src/can-utils/candump -t A -caexd can1,072:7FF,#FFFFFFFF
stdbuf -i0 -o0 -e0 /usr/src/can-utils/candump -t A -b can1 -caexd can0,0:0,#FFFFFFFF
done