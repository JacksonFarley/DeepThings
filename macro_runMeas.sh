#!/bin/bash

if [ $# -eq 1 ]
then
    MEM_SIZE=${1}
else
    MEM_SIZE="1G"
fi

SQUARE_DIM=(1 2 3 4 5)

for i in "${SQUARE_DIM[@]}"
do 
    
    echo "starting ${i}x${i}"

    FILENAME="dt_1cpu_${MEM_SIZE}_${i}x${i}.txt"
     ./runMeas.sh $FILENAME $i $i &  
    wait 
    echo "finished ${i}x${i}"
    sleep 10
done
