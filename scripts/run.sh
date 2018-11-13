#!/bin/sh
#
# This script will start Scouting Data-Taking Software
#

cd ../src

echo "Clearing caches..."
sync
echo 3 > /proc/sys/vm/drop_caches

while true 
do
    echo "Starting scdaq..."
    ./scdaq 2>&1 | logger --stderr --tag scdaq --id
    echo "Resetting the board..."
    ../scripts/reset-firmware.sh
    echo "Clearing caches..."
    sync
    echo 3 > /proc/sys/vm/drop_caches
    echo "Waiting 30 seconds..."
    for ((i=30; i>0; i--)) 
    do
	echo "$i"
	sleep 1
    done
done
