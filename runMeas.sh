#!/bin/bash

if [ $# -ge 1 ] 
then 
	OUTPUT_FILE="meas/${1}"
else
	OUTPUT_FILE="meas/meas_out.txt"
fi

if [ $# -eq 3 ]
then
    HEIGHT=${2}
    WIDTH=${3}
else
    HEIGHT=3
    WIDTH=3
fi

echo measuring $HEIGHT X $WIDTH network

cd examples/ 


#./memory_probe "deepthings" 1 $OUTPUT_FILE &
./memory_probe_multi.sh "deepthings" 1 $OUTPUT_FILE & 

#currently redirecting all output from deepthings. 
./deepthings -n $HEIGHT -m $WIDTH &> dt.out

# to keep deepthings output, comment this out
#rm dt.out

# both deepthings and the measurement script should exit on their own.

# this is just a second of sleep to allow for the memory probe to finish
# outputting before a command prompt appears
sleep 1

