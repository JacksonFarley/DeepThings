#!/bin/bash

if [ $# -ge 1 ] 
then 
	MODE=${1}
else
	MODE=1
fi


if [ $# -ge 2 ] 
then 
	OUTPUT_FILE="meas/${2}"
else
	OUTPUT_FILE="meas/meas_out.txt"
fi

if [ $# -ge 4 ]
then
    HEIGHT=${3}
    WIDTH=${4}
else
    HEIGHT=3
    WIDTH=3
fi

if [ $# -eq 6 ]
then
    SEC_HEIGHT=${3}
    SEC_WIDTH=${4}
else
    SEC_HEIGHT=3
    SEC_WIDTH=3
fi


echo measuring $HEIGHT X $WIDTH network

cd examples/ 


#./memory_probe "deepthings" 1 $OUTPUT_FILE &
./memory_probe_multi.sh "deepthings" 1 $OUTPUT_FILE & 

#currently redirecting all output from deepthings. 
./deepthings -mode $MODE -n $HEIGHT -m $WIDTH -sec_n $SEC_HEIGHT -sec_m $SEC_WIDTH &> dt.out

# to keep deepthings output, comment this out
#rm dt.out

# both deepthings and the measurement script should exit on their own.

# this is just a second of sleep to allow for the memory probe to finish
# outputting before a command prompt appears
sleep 1

