#!/bin/bash


#argument parsing

if [ $# -ge 1 ] 
then 
	echo command $1 shall be probed
	COMMAND_PROBE=$1
else
	COMMAND_PROBE="bash"
fi

if [ $# -ge 2 ]
then
	echo measurement interval of $2 specified
	MEASUREMENT_INTERVAL=$2
else
	MEASUREMENT_INTERVAL=1

fi


if [ $# -ge 3 ]
then
	echo output file $3 specified
	OUTPUT_FILE=$3
else
	OUTPUT_FILE="meas_out.txt"
fi

VMSTAT_FILE=$(echo $OUTPUT_FILE | cut -f1 -d'.').memstat

#VMSTAT_FILE="${OUTPUT_FILE}.memstat"

echo memstat file $VMSTAT_FILE specified


# vmstat originally prints an average, then the difference
#SWAPPED_BLOCKS=$SWAPPED_BLOCKS+$(vmstat 1 | sed '3q;d' | awk '{print $7+$8}')

vmstat -n 1 > $VMSTAT_FILE &

# wait for a correct process to be identified
# the header is included in the line count, so we wait until an entry
# creates a number of lines greater than 1
while [ $(ps -C "${COMMAND_PROBE}" | wc -l) -le 1 ]
do
	sleep $MEASUREMENT_INTERVAL
done

# do an initial measurement with headers
ps --format pid,pcpu,pmem,bsdtime,etime,size,rss,drs,trs,sz,vsz,comm -C $COMMAND_PROBE > $OUTPUT_FILE
sleep $MEASUREMENT_INTERVAL


#continue to measure the process until there is no process left to measure. 
#All subsequent measurements don't need to have headers, thus the equals signs. 
while [ $(ps -C "${COMMAND_PROBE}" | wc -l) -gt 1 ]
do
	ps --format pid=,pcpu=,pmem=,bsdtime=,etime=,size=,rss=,drs=,trs=,sz=,vsz=,comm= -C $COMMAND_PROBE >> $OUTPUT_FILE
	sleep $MEASUREMENT_INTERVAL
done

#kill vmstat command from earlier
kill $(ps | grep '[v]mstat' | awk '{print $1}')

# retrieve swapped block value
SWAPPED_BLOCKS=$(cat $VMSTAT_FILE | awk 'NR > 3'  | head -n -1 | awk '{s+=$7+$8} END {print s}')
ELAPSED_TIME=$(cat $OUTPUT_FILE | tail -n -1 | awk '{print $5}' | awk -F: '{print ($1*60)+$2}')
CPU_TIME=$(cat $OUTPUT_FILE | tail -n -1 | awk '{print $4}' | awk -F: '{print ($1*60)+$2}')

echo swapped blocks $SWAPPED_BLOCKS calculated. 
echo elapsed time   $ELAPSED_TIME s
echo cpu time       $CPU_TIME s
echo efficiency     $((($CPU_TIME*100)/$ELAPSED_TIME))%
echo Swapped blocks: $SWAPPED_BLOCKS >> $OUTPUT_FILE

#rm $VMSTAT_FILE
