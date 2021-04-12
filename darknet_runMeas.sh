#!/bin/bash

# This file runs standard darknet instead of the deepthings variante

# NOTE: This requires the standard darknet to have been downloaded and made. 
# this can be gotten from https://github.com/pjreddie/darknet

if [ $# -eq 1 ] 
then 
	OUTPUT_FILE="meas/darknet_${1}"
else 
	OUTPUT_FILE="meas/darknet_meas_out.txt"
fi

cd examples

./memory_probe.sh "darknet_dist" 1 $OUTPUT_FILE & 

cd ../darknet-nnpack
#currently redirecting all output from darknet

#yolo_cut only performs the first 16 layers of the model. This gives a more direct comparison to the FTP
./darknet_dist detect ../models/yolo_cut.cfg ../models/yolo.weights ../examples/data/input/0.jpg &> dt.out

#./darknet_dist detect ../models/yolo.cfg ../models/yolo.weights ../examples/data/input/0.jpg &> dt.out

# to keep deepthings output, comment this out
#rm dt.out

# both darknet and the measurement script should exit on their own.

# this is just a second of sleep to allow the memory script to finish
# outputting before the next command prompt
sleep 1

