#!/bin/sh


echo "Starting mpi run..."
echo "$1"
echo
echo

echo mpirun $1 \
	   -np 1 xdaq.exe -p ${ARTDAQ_BOARDREADER_PORT1} -e ${XDAQ_ARGS} : \
	   -np 1 xdaq.exe -p ${ARTDAQ_BOARDREADER_PORT2} -e ${XDAQ_ARGS} : \
	   -np 1 xdaq.exe -p ${ARTDAQ_BUILDER_PORT}      -e ${XDAQ_ARGS} : \
	   -np 1 xdaq.exe -p ${ARTDAQ_AGGREGATOR_PORT}   -e ${XDAQ_ARGS} &

echo
echo
	   
mpirun $1 \
	   -np 1 xdaq.exe -p ${ARTDAQ_BOARDREADER_PORT1} -e ${XDAQ_ARGS} : \
	   -np 1 xdaq.exe -p ${ARTDAQ_BOARDREADER_PORT2} -e ${XDAQ_ARGS} : \
	   -np 1 xdaq.exe -p ${ARTDAQ_BUILDER_PORT}      -e ${XDAQ_ARGS} : \
	   -np 1 xdaq.exe -p ${ARTDAQ_AGGREGATOR_PORT}   -e ${XDAQ_ARGS} &
