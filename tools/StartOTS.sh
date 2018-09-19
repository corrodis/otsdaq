#!/bin/sh

#for some reason, this function does not exist in this script.. recreating
function toffS () 
{ 
	${TRACE_BIN}/trace_cntl lvlclr 0 `bitN_to_mask "$@"` 0
}

function muteTrace() {
	#source /data/ups/setup
	#setup TRACE v3_13_04
	#ups active
	#which trace_cntl
	#type toffS
	
	#for muting trace
	export TRACE_NAME=OTSDAQ_TRACE
	 
	#type toffS
	
	toffS 0-63 -n CONF:OpBase_C
	toffS 0-63 -n CONF:OpLdStr_C
	toffS 0-63 -n CONF:CrtCfD_C
	toffS 0-63 -n COFS:DpFle_C
	toffS 0-63 -n PRVDR:FileDB_C
	toffS 0-63 -n PRVDR:FileDBIX_C
	toffS 0-63 -n JSNU:Document_C
	toffS 0-63 -n JSNU:DocUtils_C
	toffS 0-63 -n CONF:LdStrD_C
	toffS 0-63 -n FileDB:RDWRT_C
	
}
muteTrace


echo
echo "  |"
echo "  |"
echo "  |"
echo " _|_"
echo " \ /"
echo "  - "
echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ========================================================"
echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Launching StartOTS.sh otsdaq script... on {${HOSTNAME}}."
echo



SCRIPT_DIR="$( 
  cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
  pwd -P 
)"
		
echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Script directory found as: $SCRIPT_DIR/StartOTS.sh"

unalias ots.exe &>/dev/null 2>&1 #hide output
alias ots.exe='xdaq.exe' &>/dev/null #hide output



ISCONFIG=0
QUIET=1
CHROME=0
FIREFOX=0
BACKUPLOGS=0

#check for options
echo
echo
if [[ "$1"  == "--config" || "$1"  == "--configure" || "$1"  == "--wizard" || "$1"  == "--wiz" || "$1"  == "-w" || "$2"  == "--config" || "$2"  == "--configure" || "$2"  == "--wizard" || "$2"  == "--wiz" || "$2"  == "-w" ]]; then	
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t *************   WIZ MODE ENABLED!                *************"	
    ISCONFIG=1
fi

if [[ "$1"  == "--verbose" || "$2"  == "--verbose" || "$1"  == "-v" || "$2"  == "-v"  ]]; then
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t *************   VERBOSE MODE ENABLED!            ************"
	QUIET=0
fi

if [[ "$1"  == "--chrome" || "$2"  == "--chrome" || "$1"  == "-c" || "$2"  == "-c"  ]]; then
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t **************   GOOGLE-CHROME LAUNCH ENABLED!    ************"
	CHROME=1
fi

if [[ "$1"  == "--firefox" || "$2"  == "--firefox" || "$1"  == "-f" || "$2"  == "-f"  ]]; then
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t *************   FIREFOX LAUNCH ENABLED!          ************"
	FIREFOX=1
fi

if [[ "$1"  == "--backup" || "$2"  == "--backup" || "$1"  == "-b" || "$2"  == "-b"  ]]; then
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t *************   BACKUP LOGS ENABLED!              ************"
	BACKUPLOGS=1
fi
echo
#end check for options


#############################
#initializing StartOTS action file
#attempt to mkdir for full path so that it exists to move the database to
# assuming mkdir is non-destructive
#Note: quit file added to universally quit StartOTS scripts originating from same USER_DATA
#Note: local path quit file added to universally quit StartOTS scripts originating from same directory (regardless of USER_DATA)
# can not come from action file because individual StartOTS scripts need to respond to that one.
# The gateway supervisor StartOTS script drives the quit file.

OTSDAQ_STARTOTS_ACTION_FILE="${USER_DATA}/ServiceData/StartOTS_action_${HOSTNAME}.cmd"
OTSDAQ_STARTOTS_QUIT_FILE="${USER_DATA}/ServiceData/StartOTS_action_quit.cmd"
OTSDAQ_STARTOTS_LOCAL_QUIT_FILE=".StartOTS_action_quit.cmd"
echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t StartOTS_action path     = ${OTSDAQ_STARTOTS_ACTION_FILE}"
echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t StartOTS_quit path       = ${OTSDAQ_STARTOTS_QUIT_FILE}"
echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t StartOTS_local_quit path = ${OTSDAQ_STARTOTS_LOCAL_QUIT_FILE}"


SAP_ARR=$(echo "${USER_DATA}/ServiceData" | tr '/' "\n")
SAP_PATH=""
for SAP_EL in ${SAP_ARR[@]}
do
	#echo $SAP_EL
	SAP_PATH="$SAP_PATH/$SAP_EL"
	#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t $SAP_PATH"
			
	mkdir -p $SAP_PATH &>/dev/null #hide output
done

#exit any old action loops
echo "EXIT_LOOP" > $OTSDAQ_STARTOTS_ACTION_FILE

#done initializing StartOTS action file
#############################



#############################
#############################
# function to kill all things ots
function killprocs 
{	
	if [[ "x$1" == "x" ]]; then #kill all ots processes

		killall ots.exe 			&>/dev/null 2>&1 #hide output
		killall xdaq.exe 			&>/dev/null 2>&1 #hide output
		killall mf_rcv_n_fwd 		&>/dev/null 2>&1 #hide output #message viewer display without decoration		
		killall art 				&>/dev/null 2>&1 #hide output
		killall -9 ots.exe 			&>/dev/null 2>&1 #hide output
		killall -9 xdaq.exe			&>/dev/null 2>&1 #hide output
		killall -9 mf_rcv_n_fwd 	&>/dev/null 2>&1 #hide output #message viewer display without decoration		
		killall -9 art 				&>/dev/null 2>&1 #hide output
		
		usershort=`echo $USER|cut -c 1-10`
		for key in `ipcs|grep $usershort|grep ' 0 '|awk '{print $1}'`;do ipcrm -M $key;done

		
	else #then killing only non-gateway contexts
		
		PIDS=""
		for contextPID in "${ContextPIDArray[@]}"
		do
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Killing Nongateway-PID ${contextPID}"
			PIDS+=" ${contextPID}"
		done
		
		#echo Killing PIDs: $PIDS
		kill $PIDS 					&>/dev/null 2>&1 #hide output
		kill -9 $PIDS 				&>/dev/null 2>&1 #hide output

		unset ContextPIDArray #done with array of PIDs, so clear
	fi
	
	sleep 1 #give time for cleanup to occur
	
} #end killprocs
export -f killprocs

if [[ "$1"  == "--killall" || "$1"  == "--kill" || "$1"  == "--kx" || "$1"  == "-k" ]]; then

	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ******************************************************"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t *************    KILLING otsdaq!        **************"
    echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ******************************************************"
	echo
	
	#try to force kill other StartOTS scripts
	echo "EXIT_LOOP" > $OTSDAQ_STARTOTS_QUIT_FILE
	echo "EXIT_LOOP" > $OTSDAQ_STARTOTS_LOCAL_QUIT_FILE
	
    killprocs	
	killall -9 StartOTS.sh &>/dev/null 2>&1 #hide output
	
	exit
fi

if [[ $ISCONFIG == 0 && $QUIET == 1 && $CHROME == 0 && $FIREFOX == 0 && $BACKUPLOGS == 0 && "$1x" != "x" ]]; then
	echo 
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Unrecognized parameter(s) $1 $2 [Note: only two parameters are considered, others are ignored].. showing usage:"
	echo
    echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t ******************************************************"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t *************    StartOTS.sh Usage      **************"
    echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t ******************************************************"
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t To kill all otsdaq running processes, please use any of these options:"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	--killall  --kill  --kx  -k"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	e.g.: StartOTS.sh --kx"
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t To start otsdaq in 'wiz mode' please use any of these options:"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	--configure  --config  --wizard  --wiz  -w"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	e.g.: StartOTS.sh --wiz"
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t To start otsdaq with 'verbose mode' enabled, add one of these options:"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	--verbose  -v"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	e.g.: StartOTS.sh --wiz -v     or    StartOTS.sh --verbose"
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t To start otsdaq and launch google-chrome, add one of these options:"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	--chrome  -c"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	e.g.: StartOTS.sh --wiz -c     or    StartOTS.sh --chrome"
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t To start otsdaq and launch firefox, add one of these options:"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	--firefox  -f"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	e.g.: StartOTS.sh --wiz -f     or    StartOTS.sh --firefox"
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t To backup and not overwrite previous quiet log files, add add one of these options:"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	--backup  -b"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t 	e.g.: StartOTS.sh -b     or    StartOTS.sh --backup"
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Exiting StartOTS.sh. Please see usage tips above."
	exit
fi



#SERVER=`hostname -f || ifconfig eth0|grep "inet addr"|cut -d":" -f2|awk '{print $1}'`
export SUPERVISOR_SERVER=$HOSTNAME #$SERVER
if [ $ISCONFIG == 1 ]; then
    export OTS_CONFIGURATION_WIZARD_SUPERVISOR_SERVER=$SERVER
fi

 #Can be File, Database, DatabaseTest
export CONFIGURATION_TYPE=File

# Setup environment when building with MRB
if [ "x$MRB_BUILDDIR" != "x" ] && [ -e $OTSDAQ_DEMO_DIR/CMakeLists.txt ]; then
  export OTSDAQDEMO_BUILD=${MRB_BUILDDIR}/otsdaq_demo
  export OTSDAQ_DEMO_LIB=${MRB_BUILDDIR}/otsdaq_demo/lib
  export OTSDAQDEMO_REPO=$OTSDAQ_DEMO_DIR
  unset  OTSDAQ_DEMO_DIR
fi

if [ "x$MRB_BUILDDIR" != "x" ] && [ -e $OTSDAQ_DIR/CMakeLists.txt ]; then
  export OTSDAQ_BUILD=${MRB_BUILDDIR}/otsdaq
  export OTSDAQ_LIB=${MRB_BUILDDIR}/otsdaq/lib
  export OTSDAQ_REPO=$OTSDAQ_DIR
  export FHICL_FILE_PATH=.:$OTSDAQ_REPO/tools/fcl:$FHICL_FILE_PATH
fi
  
if [ "x$MRB_BUILDDIR" != "x" ] && [ -e $OTSDAQ_UTILITIES_DIR/CMakeLists.txt ]; then
  export OTSDAQUTILITIES_BUILD=${MRB_BUILDDIR}/otsdaq_utilities
  export OTSDAQ_UTILITIES_LIB=${MRB_BUILDDIR}/otsdaq_utilities/lib
  export OTSDAQUTILITIES_REPO=$OTSDAQ_UTILITIES_DIR
fi

if [ "x$OTSDAQ_DEMO_DIR" == "x" ]; then
  export OTSDAQ_DEMO_DIR=$OTSDAQDEMO_BUILD
fi

if [ "x$USER_WEB_PATH" == "x" ]; then  #setup the location for user web-apps
  export USER_WEB_PATH=$OTSDAQ_DEMO_DIR/UserWebGUI 
fi

#setup web path as XDAQ is setup.. 
#then make a link to user specified web path.
WEB_PATH=${OTSDAQ_UTILITIES_DIR}/WebGUI
#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t WEB_PATH=$WEB_PATH"
#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t USER_WEB_PATH=$USER_WEB_PATH"
#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Making symbolic link to USER_WEB_PATH"
#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ln -s $USER_WEB_PATH $WEB_PATH/UserWebPath"
ln -s $USER_WEB_PATH $WEB_PATH/UserWebPath &>/dev/null  #hide output


if [ "x$USER_DATA" == "x" ]; then
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Error."
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Environment variable USER_DATA not setup!"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t To setup, use 'export USER_DATA=<path to user data>'"
	echo 
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t (If you do not have a user data folder copy '<path to ots source>/otsdaq-demo/Data' as your starting point.)"
	echo
	exit    
fi

if [ ! -d $USER_DATA ]; then
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Error."
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t USER_DATA=$USER_DATA"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Environment variable USER_DATA does not point to a valid directory!"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t To setup, use 'export USER_DATA=<path to user data>'"
	echo 
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t (If you do not have a user data folder copy '<path to ots source>/otsdaq-demo/Data' as your starting point.)"
	echo
	exit   
fi

#print out important environment variables
echo
echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t User Data environment variable        USER_DATA           = ${USER_DATA}"
echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Database environment variable         ARTDAQ_DATABASE_URI = ${ARTDAQ_DATABASE_URI}"
echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Primary Datapath environment variable OTSDAQ_DATA         = ${OTSDAQ_DATA}"
echo
#end print out important environment variables

#check for antiquated artdaq databse
ARTDAQ_DATABASE_DIR=`echo ${ARTDAQ_DATABASE_URI}|sed 's|.*//|/|'`	
if [ ! -e ${ARTDAQ_DATABASE_DIR}/fromIndexRebuild ]; then
	# Rebuild ARTDAQ_DATABASE indicies
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Rebuilding database indices..."
	rebuild_database_index >/dev/null 2>&1; rebuild_database_index --uri=${ARTDAQ_DATABASE_URI} >/dev/null 2>&1
	
	mv ${ARTDAQ_DATABASE_DIR} ${ARTDAQ_DATABASE_DIR}.bak.$$		
	mv ${ARTDAQ_DATABASE_DIR}_new ${ARTDAQ_DATABASE_DIR}
	echo "rebuilt" > ${ARTDAQ_DATABASE_DIR}/fromIndexRebuild
#else
#	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ${ARTDAQ_DATABASE_DIR}/fromIndexRebuild file exists, so not rebuilding indices."
fi

export CONFIGURATION_DATA_PATH=${USER_DATA}/ConfigurationDataExamples
export CONFIGURATION_INFO_PATH=${USER_DATA}/ConfigurationInfo
export SERVICE_DATA_PATH=${USER_DATA}/ServiceData
export XDAQ_CONFIGURATION_DATA_PATH=${USER_DATA}/XDAQConfigurations
export LOGIN_DATA_PATH=${USER_DATA}/ServiceData/LoginData
export LOGBOOK_DATA_PATH=${USER_DATA}/ServiceData/LogbookData
export PROGRESS_BAR_DATA_PATH=${USER_DATA}/ServiceData/ProgressBarData
export ROOT_DISPLAY_CONFIG_PATH=${USER_DATA}/RootDisplayConfigData

if [ "x$OTSDAQ_DATA" == "x" ];then
	export OTSDAQ_DATA=/tmp
fi

#make directory if it does not exist
mkdir -p ${OTSDAQ_DATA} || echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Error: OTSDAQ_DATA path (${OTSDAQ_DATA}) does not exist and mkdir failed!"

if [ "x$ROOT_BROWSER_PATH" == "x" ];then
	export ROOT_BROWSER_PATH=${OTSDAQ_DEMO_DIR}
fi

if [ "x$OTSDAQ_LOG_DIR" == "x" ];then
    export OTSDAQ_LOG_DIR="${USER_DATA}/Logs"
fi

if [ "x${ARTDAQ_OUTPUT_DIR}" == "x" ]; then
    export ARTDAQ_OUTPUT_DIR="${USER_DATA}/ArtdaqData"
fi

if [ ! -d $ARTDAQ_OUTPUT_DIR ]; then
    mkdir -p $ARTDAQ_OUTPUT_DIR
fi

if [ ! -d $OTSDAQ_LOG_DIR ]; then
    mkdir -p $OTSDAQ_LOG_DIR
fi
export OTSDAQ_LOG_ROOT=$OTSDAQ_LOG_DIR

##############################################################################
export XDAQ_CONFIGURATION_XML=otsConfigurationNoRU_CMake #-> 
##############################################################################


#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ARTDAQ_MFEXTENSIONS_DIR=" ${ARTDAQ_MFEXTENSIONS_DIR}

#at end print out connection instructions using MAIN_URL
MAIN_URL="unknown_url"
MPI_RUN_CMD=""

	
#declare launch functions

####################################################################
####################################################################
################## Wiz Mode OTS Launch ###########################
####################################################################
####################################################################
#make URL print out a function so that & syntax can be used to run in background (user has immediate terminal access)
launchOTSWiz() {	
	
	#kill all things otsdaq, before launching new things	
	killprocs
	
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t *****************************************************"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t *************    Launching WIZ MODE!    *************"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t *****************************************************"
	echo
	
	####################################################################
	########### start console & message facility handling ##############
	####################################################################
	#decide which MessageFacility console viewer to run
	# and configure otsdaq MF library with MessageFacility*.fcl to use
	
	export OTSDAQ_LOG_FHICL=${USER_DATA}/MessageFacilityConfigurations/MessageFacilityGen.fcl
	#this fcl tells the MF library used by ots source how to behave
	#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t OTSDAQ_LOG_FHICL=" ${OTSDAQ_LOG_FHICL}
	
	
	USE_WEB_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseWebConsole.bool)"
	USE_QT_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseQTViewer.bool)"
			
	
	#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t USE_WEB_VIEWER" ${USE_WEB_VIEWER}
	#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t USE_QT_VIEWER" ${USE_QT_VIEWER}
	
	
	if [[ $USE_WEB_VIEWER == "1" ]]; then
		#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t CONSOLE: Using web console viewer"
		
		#start quiet forwarder with wiz receiving port and destination port parameter file
		cp ${USER_DATA}/MessageFacilityConfigurations/QuietForwarderWiz.cfg ${USER_DATA}/MessageFacilityConfigurations/QuietForwarder.cfg
		
		if [ $QUIET == 1 ]; then

			if [ $BACKUPLOGS == 1 ]; then
				DATESTRING=`date +'%s'`
				echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t      Backing up logfile to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-mf-${HOSTNAME}.${DATESTRING}.txt ***"
				mv ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-mf-${HOSTNAME}.txt ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-mf-${HOSTNAME}.${DATESTRING}.txt
			fi
			
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ===> Quiet mode redirecting output to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-mf-${HOSTNAME}.txt ***  "
			mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarder.cfg  &> ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-mf-${HOSTNAME}.txt &
		else
			mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarder.cfg  &
		fi		 	
	fi
	
	if [[ $USE_QT_VIEWER == "1" ]]; then
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t CONSOLE: Using QT console viewer"
		if [ "x$ARTDAQ_MFEXTENSIONS_DIR" == "x" ]; then #qtviewer library missing!
			echo
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Error: ARTDAQ_MFEXTENSIONS_DIR missing for qtviewer!"
			echo
			exit
		fi
		
		#start the QT Viewer (only if it is not already started)
		if [ $( ps aux|egrep -c $USER.*msgviewer ) -eq 1 ]; then				
			msgviewer -c ${USER_DATA}/MessageFacilityConfigurations/QTMessageViewerGen.fcl  &
			sleep 2	 #give time for msgviewer to be ready for messages	
		fi		
	fi
	
	####################################################################
	########### end console & message facility handling ################
	####################################################################
	
	
	
	
	#setup wiz mode environment variables
	export CONSOLE_SUPERVISOR_ID=260
	export CONFIGURATION_GUI_SUPERVISOR_ID=280
	export WIZARD_SUPERVISOR_ID=290	
	export OTS_CONFIGURATION_WIZARD_SUPERVISOR_ID=290	
	MAIN_PORT=2015

	if [ "x$OTS_WIZ_MODE_MAIN_PORT" != "x" ]; then
	  MAIN_PORT=${OTS_WIZ_MODE_MAIN_PORT}
	elif [ "x$OTS_MAIN_PORT" != "x" ]; then
	  MAIN_PORT=${OTS_MAIN_PORT}
	elif [ $USER == rrivera ]; then
	  MAIN_PORT=1983
	elif [ $USER == lukhanin ]; then
	  MAIN_PORT=2060
	elif [ $USER == uplegger ]; then
	  MAIN_PORT=1974
	elif [ $USER == parilla ]; then
	   MAIN_PORT=9000
	elif [ $USER == eflumerf ]; then
	   MAIN_PORT=1987
	elif [ $USER == swu ]; then
	   MAIN_PORT=1994
	elif [ $USER == rrivera ]; then
	   MAIN_PORT=1776
	elif [ $USER == naodell ]; then
	   MAIN_PORT=2030
	elif [ $USER == bschneid ]; then
	   MAIN_PORT=2050
	fi
	export PORT=${MAIN_PORT}	
	
		
	#substitute environment variables into template wiz-mode xdaq config xml
	envsubst <${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake.xml > ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake_Run.xml
	
	#use safe Message Facility fcl in config mode
	export OTSDAQ_LOG_FHICL=${USER_DATA}/MessageFacilityConfigurations/MessageFacility.fcl #MessageFacilityWithCout.fcl
	
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Starting wiz mode on port ${PORT}; to change, please setup environment variable OTS_WIZ_MODE_MAIN_PORT."
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Wiz mode xdaq config is ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake_Run.xml"
			
	if [ $QUIET == 1 ]; then
		echo

		if [ $BACKUPLOGS == 1 ]; then
			DATESTRING=`date +'%s'`
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t      Backing up logfile to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-wiz-${HOSTNAME}.${DATESTRING}.txt ***"
			mv ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-wiz-${HOSTNAME}.txt ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-wiz-${HOSTNAME}.${DATESTRING}.txt
		fi
		
		echo
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ===> Quiet mode redirecting output to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-wiz-${HOSTNAME}.txt ***  "
		echo
		
		ots.exe -p ${PORT} -h ${HOSTNAME} -e ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfiguration_CMake.xml -c ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake_Run.xml &> ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-wiz-${HOSTNAME}.txt &
	else
		ots.exe -p ${PORT} -h ${HOSTNAME} -e ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfiguration_CMake.xml -c ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake_Run.xml &
	fi

	################
	# start node db server
	
	#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ARTDAQ_UTILITIES_DIR=" ${ARTDAQ_UTILITIES_DIR}
	#cd $ARTDAQ_UTILITIES_DIR/node.js
	#as root, once...
	# chmod +x setupNodeServer.sh 
	# ./setupNodeServer.sh 
	# chown -R products:products *
	
	#uncomment to use artdaq db nodejs web gui
	#node serverbase.js > /tmp/${USER}_serverbase.log &
	
	MAIN_URL="http://${HOSTNAME}:${PORT}/urn:xdaq-application:lid=$WIZARD_SUPERVISOR_ID/Verify?code=$(cat ${SERVICE_DATA_PATH}//OtsWizardData/sequence.out)"
	
	printMainURL &
	
} #end launchOTSWiz
export -f launchOTSWiz
		
####################################################################
####################################################################
################## Normal Mode OTS Launch ##########################
####################################################################
####################################################################
#make URL print out a function so that & syntax can be used to run in background (user has immediate terminal access)
#ContextPIDArray is context PID array 
launchOTS() {
	
	ISGATEWAYLAUNCH=1
	if [ "x$1" != "x" ]; then #if parameter, then is nongateway launch
		ISGATEWAYLAUNCH=0
		killprocs nongateway
		
		unset ContextPIDArray 
		unset xdaqPort
		unset xdaqHost	
		
	else 					#else is full gateway and nongateway launch
		GATEWAY_PID=-1
		#kill all things otsdaq, before launching new things	
		killprocs
	fi
	
	echo
	echo	
	
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t *****************************************************"
	if [ $ISGATEWAYLAUNCH == 1 ]; then
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t ***********       Launching OTS!         ************"
	else
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t *******    Launching OTS Non-gateway Apps!    *******"
	fi
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t *****************************************************"
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t XDAQ Configuration XML: ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml"	
	echo
	echo
	
	
	####################################################################
	########### start console & message facility handling ##############
	####################################################################
	#decide which MessageFacility console viewer to run
	# and configure otsdaq MF library with MessageFacility*.fcl to use
	
	export OTSDAQ_LOG_FHICL=${USER_DATA}/MessageFacilityConfigurations/MessageFacilityGen.fcl
	#this fcl tells the MF library used by ots source how to behave
	#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t OTSDAQ_LOG_FHICL=" ${OTSDAQ_LOG_FHICL}
	
	
	if [[ $ISGATEWAYLAUNCH == 1 ]]; then
		USE_WEB_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseWebConsole.bool)"
		USE_QT_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseQTViewer.bool)"
				
		
		#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t USE_WEB_VIEWER" ${USE_WEB_VIEWER}
		#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t USE_QT_VIEWER" ${USE_QT_VIEWER}
		
		
		if [[ $USE_WEB_VIEWER == "1" ]]; then
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Launching message facility web console assistant..."
			
			#start quiet forwarder with receiving port and destination port parameter file
			cp ${USER_DATA}/MessageFacilityConfigurations/QuietForwarderGen.cfg ${USER_DATA}/MessageFacilityConfigurations/QuietForwarder.cfg
			
			if [[ $QUIET == 1 ]]; then

				if [ $BACKUPLOGS == 1 ]; then
					DATESTRING=`date +'%s'`
					echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t      Backing up logfile to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-mf-${HOSTNAME}.${DATESTRING}.txt ***"
					mv ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-mf-${HOSTNAME}.txt ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-mf-${HOSTNAME}.${DATESTRING}.txt
				fi
				
				echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ===> Quiet mode redirecting output to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-mf-${HOSTNAME}.txt ***  "
				mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarder.cfg  &> ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-mf-${HOSTNAME}.txt &
			else
				mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarder.cfg  &
			fi		 	
			echo
			echo
		fi
		
		if [[ $USE_QT_VIEWER == "1" ]]; then
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Launching QT console viewer..."
			if [ "x$ARTDAQ_MFEXTENSIONS_DIR" == "x" ]; then #qtviewer library missing!
				echo
				echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Error: ARTDAQ_MFEXTENSIONS_DIR missing for qtviewer!"
				echo
				exit
			fi
			
			#start the QT Viewer (only if it is not already started)
			if [ $( ps aux|egrep -c $USER.*msgviewer ) -eq 1 ]; then				
				msgviewer -c ${USER_DATA}/MessageFacilityConfigurations/QTMessageViewerGen.fcl  &
				sleep 2	 #give time for msgviewer to be ready for messages			
			fi		
		fi
	fi
	
	####################################################################
	########### end console & message facility handling ################
	####################################################################
	
			
	envString="-genv OTSDAQ_LOG_ROOT ${OTSDAQ_LOG_DIR} -genv ARTDAQ_OUTPUT_DIR ${ARTDAQ_OUTPUT_DIR}"

	#create argument to pass to xdaq executable
	export XDAQ_ARGS="${XDAQ_CONFIGURATION_DATA_PATH}/otsConfiguration_CMake.xml -c ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml"
	
	#echo
	#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t XDAQ ARGS PASSED TO ots.exe:"
	#echo ${XDAQ_ARGS}
	#echo
	#echo	

	#for Supervisor backwards compatibility, convert to GatewaySupervisor stealthily
	sed -i s/ots::Supervisor/ots::GatewaySupervisor/g ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml
	sed -i s/libSupervisor\.so/libGatewaySupervisor\.so/g ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml

	value=`cat ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml`	
	#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t $value"
	#re="http://(${HOSTNAME}):([0-9]+)"
	
	re="http(s*)://(.+):([0-9]+)"
	superRe="id=\"([0-9]+)\""		
	
	#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t MATCHING REGEX"
	
	haveXDAQContextPort=false
	insideContext=false
	ignore=false
	isLocal=false
	gatewayHostname=""
	gatewayPort=0
			
	while read line; do    
		if [[ ($line == *"<!--"*) ]]; then		
			ignore=true
		fi
		if [[ ($line == *"-->"*) ]]; then
			ignore=false
		fi
		if [[ ${ignore} == true ]]; then
			continue
		fi
		#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t $line"
				
		if [[ ($line == *"xc:Context"*) && ($line == *"url"*) ]]; then
			if [[ ($line =~ $re) ]]; then
				#if https && hostname matches
				#   convert hostname to localhost
				#   create node config files with https:port forwarding to localhost:madeupport
				#   run nodejs
				#   run xdaq
			
				#echo ${BASH_REMATCH[1]}
				#echo ${BASH_REMATCH[2]}
			
				port=${BASH_REMATCH[3]}
				host=${BASH_REMATCH[2]}
				insideContext=true
						
				#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t $host $port "				
						
				if [[ (${BASH_REMATCH[2]} == ${HOSTNAME}) || (${BASH_REMATCH[2]} == ${HOSTNAME}"."*) || (${BASH_REMATCH[2]} == "localhost") ]]; then
				    isLocal=true
				else
				    isLocal=false
				fi
				if [[ ${contextHostname[*]} != ${BASH_REMATCH[2]} ]]; then
					contextHostname+=(${BASH_REMATCH[2]})
					#echo ${BASH_REMATCH[1]}    
				fi
			fi
			#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ------------------------------------------ out"
	
		fi
		if [[ $line == *"/xc:Context"* ]]; then
			insideContext=false
			haveXDAQContextPort=false
			#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t in ------------------------------------------"
		fi
		if [[ ($insideContext == true) ]]; then 
			
			if [[ ($line == *"class"*) ]] && [[ "${isLocal}" == "true" ]]; then #IT'S A XDAQ SUPERVISOR		
				
				if [[ ($line == *"ots::GatewaySupervisor"*) ]]; then #IT's the SUPER supervisor, record LID 
					if [[ ($line =~ $superRe) ]]; then
					    gatewayHostname=${host}
						gatewayPort=${port}
						
						#echo ${BASH_REMATCH[1]}	#should be supervisor LID
						MAIN_URL="http://${host}:${port}/urn:xdaq-application:lid=${BASH_REMATCH[1]}/"
								
						#if gateway launch, do it
						if [[ $ISGATEWAYLAUNCH == 1 && ${host} == ${HOSTNAME} ]]; then	
							if [ $QUIET == 1 ]; then
								echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Launching the Gateway Application on host {${HOSTNAME}}..."								

								if [ $BACKUPLOGS == 1 ]; then
									DATESTRING=`date +'%s'`
									echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t      Backing up logfile to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-gateway-${HOSTNAME}-${port}.${DATESTRING}.txt ***"
									mv ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-gateway-${HOSTNAME}-${port}.txt ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-gateway-${HOSTNAME}-${port}.${DATESTRING}.txt
								fi
								
								echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ===> Quiet mode redirecting output to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-gateway-${HOSTNAME}-${port}.txt ***  "
								ots.exe -h ${host} -p ${port} -e ${XDAQ_ARGS} &> ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-gateway-${HOSTNAME}-${port}.txt &								
								
							else
								ots.exe -h ${host} -p ${port} -e ${XDAQ_ARGS} &
							fi
							
							GATEWAY_PID=$!
							echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Gateway-PID = ${GATEWAY_PID}"
							echo
							echo
							

							printMainURL &
															
						fi
					fi
				elif [[ ($haveXDAQContextPort == false) && ($gatewayHostname != $host || $gatewayPort != $port) ]]; then 
					xdaqPort+=($port)
					xdaqHost+=($host)					
					haveXDAQContextPort=true
				fi
				
			  #IF THERE IS AT LEAST ONE NOT ARTDAQ APP THEN I CAN GET OUT OF THIS CONTEXT AND RUN XDAQ ONCE JUST FOR THIS
			  #insideContext=false #RAR commented because need Super Supervisor connection LID for URL
			  #echo $line          
			fi
		fi   
	done < ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml
		
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Launching all otsdaq Applications for host {${HOSTNAME}}..."
	i=0	
	for port in "${xdaqPort[@]}"
	do
	  : 
	  	#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ots.exe -h ${xdaqHost[$i]} -p ${port} -e ${XDAQ_ARGS} &"
		#echo
		  

		if [[ ${xdaqHost[$i]} != ${HOSTNAME} ]]; then
			continue
		fi
	
		if [ $QUIET == 1 ]; then		  

			if [ $BACKUPLOGS == 1 ]; then
				DATESTRING=`date +'%s'`				
				
				echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t      Backing up logfile to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-${HOSTNAME}-${port}.${DATESTRING}.txt ***"				
				mv ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-${HOSTNAME}-${port}.txt ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-${HOSTNAME}-${port}.${DATESTRING}.txt
			fi
		  			
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ===> Quiet mode redirecting output to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-${HOSTNAME}-${port}.txt ***  "			
			ots.exe -h ${xdaqHost[$i]} -p ${port} -e ${XDAQ_ARGS} &> ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-${HOSTNAME}-${port}.txt &
		else
		  ots.exe -h ${xdaqHost[$i]} -p ${port} -e ${XDAQ_ARGS} &
		fi
		
		ContextPIDArray+=($!)
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Nongateway-PID = ${ContextPIDArray[$i]}"
		
		i=$(( $i + 1 ))
	done
		

	FIRST_TIME=0 #used to supress printouts
	
	echo
	echo
	echo
	echo
	
	if [[ ${#contextHostname[@]} == 1 ]]; then 
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t This is the ONLY host configured to run xdaq applications: ${contextHostname[@]}" 	    
	  else
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t These are the hosts configured to run xdaq applications: ${contextHostname[@]}"
	fi
	echo
	  
	if [[ (${#xdaqPort[@]} == 0) ]]; then
	  echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ********************************************************************************************************************************"
	  echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ********************************************************************************************************************************"

	  echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t WARNING: There are no configured processes for hostname ${HOSTNAME}." 
	  echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Are you sure your configuration is written for ${HOSTNAME}?" 
	 
	  echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t ********************************************************************************************************************************"
	  echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t ********************************************************************************************************************************"
	fi

}   #end launchOTS
export -f launchOTS


#########################################################
#########################################################
#make URL print out a function so that & syntax can be used to run in background (user has immediate terminal access)
printMainURL() {	
	
	#echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t printMainURL()"
	
	#check if StartOTS.sh was aborted
	#OTSDAQ_STARTOTS_ACTION="$(cat ${OTSDAQ_STARTOTS_ACTION_FILE})"
	#	if [ "$OTSDAQ_STARTOTS_ACTION" == "EXIT_LOOP" ]; then
	#		exit
	#	fi
	
	if [ $QUIET == 0 ]; then
		sleep 4 #give a little more time before injecting printouts in scrolling printouts
	else
		sleep 2 #give a little time for other StartOTS printouts to occur (so this one is last)  
	fi
	
	echo
	echo
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Open the URL below in your Google Chrome or Mozilla Firefox web browser:"	
	
	
	if [ $MAIN_URL == "unknown_url" ]; then
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t INFO: No gateway supervisor found for node {${HOSTNAME}}."
		exit
	fi
	
	for i in {1..5}
	do
		#check if StartOTS.sh was aborted
		#OTSDAQ_STARTOTS_ACTION="$(cat ${OTSDAQ_STARTOTS_ACTION_FILE})"
		#if [ "$OTSDAQ_STARTOTS_ACTION" == "EXIT_LOOP" ]; then
		#exit
		#fi
		
		
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t *********************************************************************"
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t otsdaq URL = $MAIN_URL"
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}] \t *********************************************************************"
		echo
		
		if [ $QUIET == 1 ]; then
			exit
		fi
		sleep 2 #for delay between each printout
	done
}  #end printMainURL
export -f printMainURL
	

#########################################################
#########################################################
otsActionHandler() {
		
	echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Starting action handler..."

	#clear file initially
	echo "0" > $OTSDAQ_STARTOTS_ACTION_FILE
	

	if [[ ($ISCONFIG == 1) || ("${HOSTNAME}" == "${gatewayHostname}") ]]; then
		echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t The script, on ${HOSTNAME}, is the gateway StartOTS.sh script, so it will drive exit of other scripts."
		

		echo "EXIT_LOOP" > $OTSDAQ_STARTOTS_QUIT_FILE
		echo "EXIT_LOOP" > $OTSDAQ_STARTOTS_LOCAL_QUIT_FILE
		
		#time for other stale StartOTS to quit
		sleep 5
		echo "0" > $OTSDAQ_STARTOTS_QUIT_FILE
		echo "0" > $OTSDAQ_STARTOTS_LOCAL_QUIT_FILE
	else
		sleep 10 #non masters sleep for a while, to give time to quit stale scripts
	fi	
		
	FIRST_TIME=1
	
	
	
	#listen for file commands
	while true; do
		#In OTSDAQ_STARTOTS_ACTION_FILE
		#0 is the default. No action is taken
		#REBUILD_OTS will rebuild otsdaq
		#RESET_MPI will reboot artdaq MPI runs
		#EXIT_LOOP will exit StartOTS loop
		#if cmd file is missing, exit StartOTS loop
		
		OTSDAQ_STARTOTS_ACTION="$(cat ${OTSDAQ_STARTOTS_ACTION_FILE})"
		OTSDAQ_STARTOTS_QUIT="$(cat ${OTSDAQ_STARTOTS_QUIT_FILE})"
		OTSDAQ_STARTOTS_LOCAL_QUIT="$(cat ${OTSDAQ_STARTOTS_LOCAL_QUIT_FILE})"
		
		if [ "$OTSDAQ_STARTOTS_ACTION" == "REBUILD_OTS" ]; then
		
			echo
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t  "
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Rebuilding..."
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t  "			
			#echo "1" > mrbresult.num; mrb b > otsdaq_startots_mrbreport.txt && echo "0" > mrbresult.num			
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t  "
			#grep -A 1 -B 1 "INFO: Stage build successful." otsdaq_startots_mrbreport.txt
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t  "
			sleep 5
		
		elif [ "$OTSDAQ_STARTOTS_ACTION" == "OTS_APP_SHUTDOWN" ]; then
		
			echo
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t  "
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Shutting down non-gateway contexts..."
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t  "	
			
			#kill all non-Gateway context processes
			killprocs nongateway
			
			
		elif [ "$OTSDAQ_STARTOTS_ACTION" == "OTS_APP_STARTUP" ]; then
		
			echo
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t  "
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Starting up non-gateway contexts..."
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t  "	
			
			launchOTS nongateway #launch all non-gateway apps			
	
		elif [ "$OTSDAQ_STARTOTS_ACTION" == "LAUNCH_WIZ" ]; then
			
			echo
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Starting otsdaq Wiz mode for host {${HOSTNAME}}..."
			echo
			killprocs
			
			launchOTSWiz
			
			sleep 3 #so that the terminal comes back after the printouts in quiet mode
			
			FIRST_TIME=1 #re-enable printouts for launch ots, in case of context changes

		elif [ "$OTSDAQ_STARTOTS_ACTION" == "LAUNCH_OTS" ]; then
				
			echo
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Starting otsdaq in normal mode for host {${HOSTNAME}}..."
			echo
			killprocs
			
			launchOTS

			sleep 3 #so that the terminal comes back after the printouts in quiet mode			

		elif [ "$OTSDAQ_STARTOTS_ACTION" == "FLATTEN_TO_SYSTEM_ALIASES" ]; then

			echo
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Removing unused tables and groups based on active System Aliases..."
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t otsdaq_flatten_system_aliases 0"
			echo	
			echo 
			if [ $QUIET == 1 ]; then			

				if [ $BACKUPLOGS == 1 ]; then
					DATESTRING=`date +'%s'`
					echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t      Backing up logfile to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-flatten-${HOSTNAME}.${DATESTRING}.txt ***"
					mv ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-flatten-${HOSTNAME}.txt ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-flatten-${HOSTNAME}.${DATESTRING}.txt
				fi
				
				echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t ===> Quiet mode redirecting output to *** ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-flatten-${HOSTNAME}.txt ***  "	
				otsdaq_flatten_system_aliases 0 &> ${OTSDAQ_LOG_DIR}/otsdaq_quiet_run-flatten-${HOSTNAME}.txt &
			else
				otsdaq_flatten_system_aliases 0 &
			fi		
						
		elif [[ "$OTSDAQ_STARTOTS_ACTION" == "EXIT_LOOP" || "$OTSDAQ_STARTOTS_QUIT" == "EXIT_LOOP" || "$OTSDAQ_STARTOTS_LOCAL_QUIT" == "EXIT_LOOP" ]]; then


			if [[ ($ISCONFIG == 1) || ("${HOSTNAME}" == "${gatewayHostname}") ]]; then
				echo "EXIT_LOOP" > $OTSDAQ_STARTOTS_QUIT_FILE
				echo "EXIT_LOOP" > $OTSDAQ_STARTOTS_LOCAL_QUIT_FILE
			fi
			
		    exit
			
		elif [[ "$OTSDAQ_STARTOTS_ACTION" != "0"  || "$OTSDAQ_STARTOTS_QUIT" != "0"  || "$OTSDAQ_STARTOTS_LOCAL_QUIT" != "0" ]]; then
		
			echo -e `date +"%h%y %T"` "StartOTS.sh [${LINENO}]  \t Exiting StartOTS.sh.. Unrecognized command !=0 in ${OTSDAQ_STARTOTS_ACTION}-${OTSDAQ_STARTOTS_QUIT}-${OTSDAQ_STARTOTS_LOCAL_QUIT}"			
			exit
			
		fi
		
		echo "0" > $OTSDAQ_STARTOTS_ACTION_FILE
		sleep 1
	done

		
} #end otsActionHandler
export -f otsActionHandler



#functions have been declared
#now launch things

if [ $ISCONFIG == 1 ]; then
	launchOTSWiz
else
	launchOTS #only launch gateway once.. on shutdown and startup others can relaunch
fi

otsActionHandler &

#launch chrome here if enabled
if [ $CHROME == 1 ]; then
	sleep 3 #give time for server to be live
	google-chrome $MAIN_URL &
fi

#launch firefox here if enabled
if [ $FIREFOX == 1 ]; then
	sleep 3 #give time for server to be live
	firefox $MAIN_URL &
fi

sleep 3 #so that the terminal comes back after the printouts are done ( in quiet mode )











