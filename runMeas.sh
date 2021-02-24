#!/bin/bash

if [ $# -eq 1 ] 
then 
	OUTPUT_FILE="meas/${1}"
else 
	OUTPUT_FILE="meas/meas_out.txt"
fi


cd examples/ 


./memory_probe.sh "deepthings" 1 $OUTPUT_FILE & 

#currently redirecting all output from deepthings. 
./deepthings &> dt.out

# to keep deepthings output, comment this out
#rm dt.out

# both deepthings and the measurement script should exit on their own.


