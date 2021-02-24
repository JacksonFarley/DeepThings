#!/bin/bash

echo $#

if [ $# -ge 1 ] 
then
	echo measurement interval of $1 specified
	MEASUREMENT_INTERVAL=$1
else
	MEASUREMENT_INTERVAL=1

fi


if [ $# -ge 2 ]
then
	echo output file $2 specified
	OUTPUT_FILE=$2
else
	OUTPUT_FILE="meas_out.txt"
fi


# wait for a correct process to be identified
# the header is included in the line count, so we wait until an entry
# creates a number of lines greater than 1
while [ $(ps -C "deepthings" | wc -l) -le 1 ]
do
	sleep $MEASUREMENT_INTERVAL
done

# do an initial measurement with headers
ps --format pid,pcpu,pmem,bsdtime,etime,size,rss,drs,trs,sz,vsz,comm -C deepthings > $OUTPUT_FILE
sleep $MEASUREMENT_INTERVAL


#continue to measure the process until there is no process left to measure. 
#All subsequent measurements don't need to have headers, thus the equals signs. 
while [ $(ps -C "deepthings" | wc -l) -gt 1 ]
do
	ps --format pid=,pcpu=,pmem=,bsdtime=,etime=,size=,rss=,drs=,trs=,sz=,vsz=,comm= -C deepthings >> $OUTPUT_FILE
	sleep $MEASUREMENT_INTERVAL
done


