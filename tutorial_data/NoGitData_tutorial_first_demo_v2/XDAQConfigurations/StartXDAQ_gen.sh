#!/bin/sh

echo
echo

echo "Launching XDAQ contexts..."

#for each XDAQ context
echo "xdaq.exe -p 1983 -e ${XDAQ_ARGS} &"
xdaq.exe -p 1983 -e ${XDAQ_ARGS} & #mainContext


