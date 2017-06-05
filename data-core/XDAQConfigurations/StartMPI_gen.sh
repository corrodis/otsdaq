#!/bin/sh

	echo
	echo

	echo "Cleaning up old MPI instance..."
	echo
	echo

	echo "Starting mpi run..."
	echo "$1"
	echo
	echo

	echo "	mpirun $1 \
     -np 1 xdaq.exe -p 2015 -e ${XDAQ_ARGS} : \
     -np 1 xdaq.exe -p 2015 -e ${XDAQ_ARGS} "	
	echo
	echo


	#context.port_ : MainContext
	#context.port_ : MAPSAContext

	mpirun $1 \
     -np 1 xdaq.exe -p 2015 -e ${XDAQ_ARGS} : \
     -np 1 xdaq.exe -p 2015 -e ${XDAQ_ARGS} 

