#!/bin/sh
echo -e "StartOTS.sh [${LINENO}]  \t ========================================================"
echo -e "StartOTS.sh [${LINENO}]  \t Launching StartOTS.sh otsdaq script... on {${HOSTNAME}}."
echo

SCRIPT_DIR="$( 
  cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
  pwd -P 
)"
		
echo -e "StartOTS.sh [${LINENO}]  \t Script directory found as: $SCRIPT_DIR/$0"

ISCONFIG=0
QUIET=1
CHROME=0
DONOTKILL=0

#############################
#############################
# function to kill all things ots
function killprocs 
{
	killall -9 mpirun &>/dev/null 2>&1 #hide output
	killall -9 xdaq.exe &>/dev/null 2>&1 #hide output
	killall -9 mf_rcv_n_fwd &>/dev/null 2>&1 #hide output #message viewer display without decoration
	
	#if [[ "x$1" == "x" ]]; then
	#		PIDS=`ps --no-headers axk comm o pid,args|grep mpirun|grep $USER_DATA|awk '{print $1}'`
	#		PIDS+=" "
	#		PIDS+=`ps --no-headers axk comm o pid,args|grep xdaq.exe|grep $USER_DATA|awk '{print $1}'`
	#		PIDS+=" "
	#		PIDS+=`ps --no-headers axk comm o pid,args|grep mf_rcv_n_fwd|grep $USER_DATA|awk '{print $1}'`
	#		
	#		#echo Killing PIDs: $PIDS
	#		kill $PIDS >/dev/null 2>&1
	#		kill -9 $PIDS >/dev/null 2>&1
	#    else
	#		PIDS=`ps --no-headers axk comm o pid,args|grep $1|grep $USER_DATA|awk '{print $1}'`
	#		#echo Killing PIDs: $PIDS
	#		kill $PIDS >/dev/null 2>&1
	#		kill -9 $PIDS >/dev/null 2>&1
	#    fi
}
#end killprocs

#check for wiz mode
if [[ "$1"  == "--config" || "$1"  == "--configure" || "$1"  == "--wizard" || "$1"  == "--wiz" || "$1"  == "-w" ]]; then
	echo -e "StartOTS.sh [${LINENO}]  \t *****************************************************"
	echo -e "StartOTS.sh [${LINENO}]  \t *************      WIZ MODE ENABLED!    *************"
	echo -e "StartOTS.sh [${LINENO}]  \t *****************************************************"
    ISCONFIG=1
fi


if [[ "$1"  == "--verbose" || "$2"  == "--verbose" || "$1"  == "-v" || "$2"  == "-v"  ]]; then
	echo -e "StartOTS.sh [${LINENO}]  \t *************   VERBOSE MODE ENABLED!    ************"
	QUIET=0
fi

if [[ "$1"  == "--chrome" || "$2"  == "--chrome" || "$1"  == "-c" || "$2"  == "-c"  ]]; then
	echo -e "StartOTS.sh [${LINENO}]  \t *************   GOOGLE-CHROME LAUNCH ENABLED!    ************"
	CHROME=1
fi

if [[ "$1"  == "--donotkill" || "$2"  == "--donotkill" || "$1"  == "-d" || "$2"  == "-d"  ]]; then
	echo -e "StartOTS.sh [${LINENO}]  \t *************   DO-NOT-KILL ENABLED!    ************"
	DONOTKILL=1
fi

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
OTSDAQ_STARTOTS_LOCAL_QUIT_FILE="${SCRIPT_DIR}/.StartOTS_action_quit.cmd"
echo -e "StartOTS.sh [${LINENO}]  \t StartOTS_action path     = ${OTSDAQ_STARTOTS_ACTION_FILE}"
echo -e "StartOTS.sh [${LINENO}]  \t StartOTS_quit path       = ${OTSDAQ_STARTOTS_QUIT_FILE}"
echo -e "StartOTS.sh [${LINENO}]  \t StartOTS_local_quit path = ${OTSDAQ_STARTOTS_LOCAL_QUIT_FILE}"
echo
echo

SAP_ARR=$(echo "${USER_DATA}/ServiceData" | tr '/' "\n")
SAP_PATH=""
for SAP_EL in ${SAP_ARR[@]}
do
	#echo $SAP_EL
	SAP_PATH="$SAP_PATH/$SAP_EL"
	#echo $SAP_PATH
	mkdir -p $SAP_PATH &>/dev/null #hide output
done

#exit any old action loops
echo "EXIT_LOOP" > $OTSDAQ_STARTOTS_ACTION_FILE

#done initializing StartOTS action file
#############################

if [[ "$1"  == "--killall" || "$1"  == "--kill" || "$1"  == "--kx" || "$1"  == "-k" ]]; then
	echo -e "StartOTS.sh [${LINENO}]  \t ******************************************************"
	echo -e "StartOTS.sh [${LINENO}]  \t *************    KILLING otsdaq!        **************"
    echo -e "StartOTS.sh [${LINENO}]  \t ******************************************************"

    killprocs
	
	
	#try to force kill other StartOTS scripts
	echo "EXIT_LOOP" > $OTSDAQ_STARTOTS_QUIT_FILE
	echo "EXIT_LOOP" > $OTSDAQ_STARTOTS_LOCAL_QUIT_FILE
	killall -9 StartOTS.sh &>/dev/null #hide output
	
	exit
fi

if [[ $ISCONFIG == 0 && $QUIET == 1 && $CHROME == 0 && $DONOTKILL == 0 && "$1x" != "x" ]]; then
	echo 
	echo -e "StartOTS.sh [${LINENO}]  \t Unrecognized paramater $1"
	echo
    echo -e "StartOTS.sh [${LINENO}]  \t ******************************************************"
	echo -e "StartOTS.sh [${LINENO}]  \t *************    StartOTS.sh Usage      **************"
    echo -e "StartOTS.sh [${LINENO}]  \t ******************************************************"
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t To start otsdaq in 'wiz mode' please use any of these options:"
	echo -e "StartOTS.sh [${LINENO}]  \t 	--configure  --config  --wizard  --wiz  -w"
	echo -e "StartOTS.sh [${LINENO}]  \t 	e.g.: StartOTS.sh --wiz"
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t To start otsdaq with 'verbose mode' enabled, add one of these options:"
	echo -e "StartOTS.sh [${LINENO}]  \t 	--verbose  -v"
	echo -e "StartOTS.sh [${LINENO}]  \t 	e.g.: StartOTS.sh --wiz -v     or    StartOTS.sh --verbose"
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t To start otsdaq and launch google-chrome, add one of these options:"
	echo -e "StartOTS.sh [${LINENO}]  \t 	--chrome  -c"
	echo -e "StartOTS.sh [${LINENO}]  \t 	e.g.: StartOTS.sh --wiz -c     or    StartOTS.sh --chrome"
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t To start otsdaq and launch google-chrome, add one of these options:"
	echo -e "StartOTS.sh [${LINENO}]  \t 	--donotkill  -d"
	echo -e "StartOTS.sh [${LINENO}]  \t 	e.g.: StartOTS.sh --wiz -d     or    StartOTS.sh --donotkill"
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t To kill all otsdaq running processes, please use any of these options:"
	echo -e "StartOTS.sh [${LINENO}]  \t 	--killall  --kill  --kx  -k"
	echo -e "StartOTS.sh [${LINENO}]  \t 	e.g.: StartOTS.sh --kx"
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t aborting StartOTS.sh..."
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
#echo -e "StartOTS.sh [${LINENO}]  \t WEB_PATH=$WEB_PATH"
#echo -e "StartOTS.sh [${LINENO}]  \t USER_WEB_PATH=$USER_WEB_PATH"
#echo -e "StartOTS.sh [${LINENO}]  \t Making symbolic link to USER_WEB_PATH"
#echo -e "StartOTS.sh [${LINENO}]  \t ln -s $USER_WEB_PATH $WEB_PATH/UserWebPath"
ln -s $USER_WEB_PATH $WEB_PATH/UserWebPath &>/dev/null #hide output


if [ "x$USER_DATA" == "x" ]; then
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t Error."
	echo -e "StartOTS.sh [${LINENO}]  \t Environment variable USER_DATA not setup!"
	echo -e "StartOTS.sh [${LINENO}]  \t To setup, use 'export USER_DATA=<path to user data>'"
	echo 
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t (If you do not have a user data folder copy '<path to ots source>/otsdaq-demo/Data' as your starting point.)"
	echo
	exit    
fi

if [ ! -d $USER_DATA ]; then
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t Error."
	echo -e "StartOTS.sh [${LINENO}]  \t USER_DATA=$USER_DATA"
	echo -e "StartOTS.sh [${LINENO}]  \t Environment variable USER_DATA does not point to a valid directory!"
	echo -e "StartOTS.sh [${LINENO}]  \t To setup, use 'export USER_DATA=<path to user data>'"
	echo 
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t (If you do not have a user data folder copy '<path to ots source>/otsdaq-demo/Data' as your starting point.)"
	echo
	exit   
fi

echo
echo -e "StartOTS.sh [${LINENO}]  \t User Data environment variable USER_DATA                 \t = ${USER_DATA}"

ARTDAQ_DATABASE_DIR=`echo ${ARTDAQ_DATABASE_URI}|sed 's|.*//|/|'`
	
echo
echo -e "StartOTS.sh [${LINENO}]  \t Database environment variable ARTDAQ_DATABASE_URI        \t = ${ARTDAQ_DATABASE_URI}"

if [ ! -e ${ARTDAQ_DATABASE_DIR}/fromIndexRebuild ]; then
	# Rebuild ARTDAQ_DATABASE indicies
	echo -e "StartOTS.sh [${LINENO}]  \t Rebuilding database indices..."
	rebuild_database_index >/dev/null 2>&1; rebuild_database_index --uri=${ARTDAQ_DATABASE_URI} >/dev/null 2>&1
	
	mv ${ARTDAQ_DATABASE_DIR} ${ARTDAQ_DATABASE_DIR}.bak.$$		
	mv ${ARTDAQ_DATABASE_DIR}_new ${ARTDAQ_DATABASE_DIR}
	echo "rebuilt" > ${ARTDAQ_DATABASE_DIR}/fromIndexRebuild
else
	echo -e "StartOTS.sh [${LINENO}]  \t ${ARTDAQ_DATABASE_DIR}/fromIndexRebuild file exists, so not rebuilding indices."
fi

echo
echo -e "StartOTS.sh [${LINENO}]  \t Primary Output Datapath environment variable OTSDAQ_DATA \t = ${OTSDAQ_DATA}"

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

##############################################################################
export XDAQ_CONFIGURATION_XML=otsConfigurationNoRU_CMake #-> 
##############################################################################

if [ $DONOTKILL == 0 ]; then
	#kill all things otsdaq, before launching new things
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t Killing all existing otsdaq Applications..."
	killprocs
	#killall -9 mpirun &>/dev/null #hide output
	#killall -9 xdaq.exe &>/dev/null #hide output
	#killall -9 mf_rcv_n_fwd &>/dev/null #hide output #message viewer display without decoration

	#give time for killall
	sleep 1
	#echo -e "StartOTS.sh [${LINENO}]  \t ...Applications killed!"
fi



#echo -e "StartOTS.sh [${LINENO}]  \t ARTDAQ_MFEXTENSIONS_DIR=" ${ARTDAQ_MFEXTENSIONS_DIR}

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
	
	echo -e "StartOTS.sh [${LINENO}]  \t *****************************************************"
	echo -e "StartOTS.sh [${LINENO}]  \t *************    Launching WIZ MODE!    *************"
	echo -e "StartOTS.sh [${LINENO}]  \t *****************************************************"

	####################################################################
	########### start console & message facility handling ##############
	####################################################################
	#decide which MessageFacility console viewer to run
	# and configure otsdaq MF library with MessageFacility*.fcl to use
	
	export OTSDAQ_LOG_FHICL=${USER_DATA}/MessageFacilityConfigurations/MessageFacilityGen.fcl
	#this fcl tells the MF library used by ots source how to behave
	#echo -e "StartOTS.sh [${LINENO}]  \t OTSDAQ_LOG_FHICL=" ${OTSDAQ_LOG_FHICL}
	
	
	USE_WEB_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseWebConsole.bool)"
	USE_QT_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseQTViewer.bool)"
			
	
	#echo -e "StartOTS.sh [${LINENO}]  \t USE_WEB_VIEWER" ${USE_WEB_VIEWER}
	#echo -e "StartOTS.sh [${LINENO}]  \t USE_QT_VIEWER" ${USE_QT_VIEWER}
	
	
	if [[ $USE_WEB_VIEWER == "1" ]]; then
		#echo -e "StartOTS.sh [${LINENO}]  \t CONSOLE: Using web console viewer"
		
		#start quiet forwarder with receiving port and destination port parameter file
	
		if [ $QUIET == 1 ]; then
			echo -e "StartOTS.sh [${LINENO}]  \t ===> Quiet mode  redirecting output to *** .otsdaq_quiet_run-mf-${HOSTNAME}.txt ***"
			mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarderGen.cfg  &> .otsdaq_quiet_run-mf-${HOSTNAME}.txt &
		else
			mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarderGen.cfg  &
		fi		 	
	fi
	
	if [[ $USE_QT_VIEWER == "1" ]]; then
		#echo -e "StartOTS.sh [${LINENO}]  \t CONSOLE: Using QT console viewer"
		if [ "x$ARTDAQ_MFEXTENSIONS_DIR" == "x" ]; then #qtviewer library missing!
			echo
			echo -e "StartOTS.sh [${LINENO}]  \t Error: ARTDAQ_MFEXTENSIONS_DIR missing for qtviewer!"
			echo
			exit
		fi
		
		#start the QT Viewer (only if it is not already started)
		if [ $( ps aux|egrep -c $USER.*msgviewer ) -eq 1 ]; then				
			msgviewer -c ${USER_DATA}/MessageFacilityConfigurations/QTMessageViewerGen.fcl  &
			sleep 2		
		fi		
	fi
	
	####################################################################
	########### end console & message facility handling ################
	####################################################################
	
	
	
	
	#setup wiz mode environment variables
	export CONSOLE_SUPERVISOR_ID=260
	export CONFIGURATION_GUI_SUPERVISOR_ID=280
	export WIZARD_SUPERVISOR_ID=290	
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
	
	echo -e "StartOTS.sh [${LINENO}]  \t Wiz mode xdaq config is ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake_Run.xml"
			
	if [ $QUIET == 1 ]; then
		echo
		echo -e "StartOTS.sh [${LINENO}]  \t ===> Quiet mode  redirecting output to *** .otsdaq_quiet_run-wiz-${HOSTNAME}.txt ***"
		echo
		xdaq.exe -p ${PORT} -h ${HOSTNAME} -e ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfiguration_CMake.xml -c ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake_Run.xml &> .otsdaq_quiet_run-wiz-${HOSTNAME}.txt &
	else
		xdaq.exe -p ${PORT} -h ${HOSTNAME} -e ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfiguration_CMake.xml -c ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake_Run.xml &
	fi

	################
	# start node db server
	
	#echo -e "StartOTS.sh [${LINENO}]  \t ARTDAQ_UTILITIES_DIR=" ${ARTDAQ_UTILITIES_DIR}
	#cd $ARTDAQ_UTILITIES_DIR/node.js
	#as root, once...
	# chmod +x setupNodeServer.sh 
	# ./setupNodeServer.sh 
	# chown -R products:products *
	
	#uncomment to use artdaq db nodejs web gui
	#node serverbase.js > /tmp/${USER}_serverbase.log &
	
	sleep 2
	MAIN_URL="http://${HOSTNAME}:${PORT}/urn:xdaq-application:lid=$WIZARD_SUPERVISOR_ID/Verify?code=$(cat ${SERVICE_DATA_PATH}//OtsWizardData/sequence.out)"
	#echo $MAIN_URL
			

	printMainURL &
	sleep 1 #so that the terminal comes back after the printouts in quiet mode

}

####################################################################
####################################################################
################## Normal Mode OTS Launch ##########################
####################################################################
####################################################################
#make URL print out a function so that & syntax can be used to run in background (user has immediate terminal access)
launchOTS() {

	ISGATEWAYLAUNCH=1
	if [ "x$1" == "x" ]; then
		ISGATEWAYLAUNCH=0
	fi
	
	echo
	echo
	
	echo -e "StartOTS.sh [${LINENO}]  \t *****************************************************"
	#if [ $ISGATEWAYLAUNCH == 1 ]; then
	#		echo -e "StartOTS.sh [${LINENO}]  \t **********       Launching OTS Gateway!    **********"
	#	else
		echo -e "StartOTS.sh [${LINENO}]  \t ***********       Launching OTS Apps!    ************"
	#	fi
	echo -e "StartOTS.sh [${LINENO}]  \t *****************************************************"
	echo -e "StartOTS.sh [${LINENO}]  \t XDAQ Configuration XML: ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml"	
	echo
	echo
	
	sed -i s/ots::Supervisor/ots::GatewaySupervisor/g ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml
	sed -i s/libSupervisor\.so/libGatewaySupervisor\.so/g ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml
	
	####################################################################
	########### start console & message facility handling ##############
	####################################################################
	#decide which MessageFacility console viewer to run
	# and configure otsdaq MF library with MessageFacility*.fcl to use
	
	export OTSDAQ_LOG_FHICL=${USER_DATA}/MessageFacilityConfigurations/MessageFacilityGen.fcl
	#this fcl tells the MF library used by ots source how to behave
	#echo -e "StartOTS.sh [${LINENO}]  \t OTSDAQ_LOG_FHICL=" ${OTSDAQ_LOG_FHICL}
	
	
	USE_WEB_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseWebConsole.bool)"
	USE_QT_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseQTViewer.bool)"
			
	
	#echo -e "StartOTS.sh [${LINENO}]  \t USE_WEB_VIEWER" ${USE_WEB_VIEWER}
	#echo -e "StartOTS.sh [${LINENO}]  \t USE_QT_VIEWER" ${USE_QT_VIEWER}
	
	
	if [[ $USE_WEB_VIEWER == "1" ]]; then
		echo -e "StartOTS.sh [${LINENO}]  \t Launching message facility web console assistant..."
		
		#start quiet forwarder with receiving port and destination port parameter file
	
		if [ $QUIET == 1 ]; then
			echo -e "StartOTS.sh [${LINENO}]  \t ===> Quiet mode  redirecting output to *** .otsdaq_quiet_run-mf-${HOSTNAME}.txt ***  (hidden file)"
			mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarderGen.cfg  &> .otsdaq_quiet_run-mf-${HOSTNAME}.txt &
		else
			mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarderGen.cfg  &
		fi		 	
		echo
		echo
	fi
	
	if [[ $USE_QT_VIEWER == "1" ]]; then
		echo -e "StartOTS.sh [${LINENO}]  \t Launching QT console viewer..."
		if [ "x$ARTDAQ_MFEXTENSIONS_DIR" == "x" ]; then #qtviewer library missing!
			echo
			echo -e "StartOTS.sh [${LINENO}]  \t Error: ARTDAQ_MFEXTENSIONS_DIR missing for qtviewer!"
			echo
			exit
		fi
		
		#start the QT Viewer (only if it is not already started)
		if [ $( ps aux|egrep -c $USER.*msgviewer ) -eq 1 ]; then				
			msgviewer -c ${USER_DATA}/MessageFacilityConfigurations/QTMessageViewerGen.fcl  &
			sleep 2		
		fi		
	fi
	
	####################################################################
	########### end console & message facility handling ################
	####################################################################
	
	
	# kill node db server (only allowed in wiz mode)
	# search assuming port 8080
	# netstat -apn | grep node | grep 8080 | grep LISTEN | rev | cut -d'.' -f1 | cut -c 16-22 | rev
	# kill result
	#NODESERVERPS="$(netstat -apn | grep node | grep 8080 | grep LISTEN | rev | cut -d'.' -f1 | cut -c 16-22 | rev)"
	#killprocs $NODESERVERPS
        #kill -9 $NODESERVERPS
			
	envString="-genv OTSDAQ_LOG_ROOT ${OTSDAQ_LOG_DIR} -genv ARTDAQ_OUTPUT_DIR ${ARTDAQ_OUTPUT_DIR}"
		
	export XDAQ_ARGS="${XDAQ_CONFIGURATION_DATA_PATH}/otsConfiguration_CMake.xml -c ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml"
	
	#echo
	#echo -e "StartOTS.sh [${LINENO}]  \t XDAQ ARGS PASSED TO xdaq.exe:"
	#echo ${XDAQ_ARGS}
	#echo
	#echo
	
	value=`cat ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml`
	
	#echo -e "StartOTS.sh [${LINENO}]  \t $value"
	#re="http://(${HOSTNAME}):([0-9]+)"
	
	re="http(s*)://(.+):([0-9]+)"
	superRe="id=\"([0-9]+)\""		
	
	#echo -e "StartOTS.sh [${LINENO}]  \t MATCHING REGEX"
	
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
				
		if [[ ($line == *"xc:Context"*) && ($line == *"url"*) ]]; then
			if [[ ($line =~ $re) ]]; then
				#if https && hostname matches
				#   convert hostname to localhost
				#   create node config files with https:port forwarding to localhost:madeupport
				#   run nodejs
				#   run xdaq
				#if [[ BASH_REMATCH[1] == "s" ]]; then
				#
				#cp ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml
				#
						#fi
				#echo ${BASH_REMATCH[1]}
				#echo ${BASH_REMATCH[2]}
					port=${BASH_REMATCH[3]}
					host=${BASH_REMATCH[2]}
					insideContext=true
					#echo $port
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
			#echo -e "StartOTS.sh [${LINENO}]  \t ------------------------------------------>>>>>>>>"
	
		fi
		if [[ $line == *"/xc:Context"* ]]; then
			insideContext=false
			haveXDAQContextPort=false
			#echo -e "StartOTS.sh [${LINENO}]  \t <<<<<<<------------------------------------------"
		fi
		if [[ ($insideContext == true) ]]; then 
#if [[ ($line == *"ots::ARTDAQDataManagerSupervisor"*) ]]; then
#			  boardReaderPort+=($port)
#			  boardReaderHost+=($host)
#			elif [[ ($line == *"ots::EventBuilderApp"*) ]]; then
#			  builderPort+=($port)
#			  builderHost+=($host)
#			elif [[ ($line == *"ots::AggregatorApp"*) ]]; then 
#			  aggregatorPort+=($port)
#			  aggregatorHost+=($host)
#			elif [[ ($line == *"ots::AggregatorApp"*) ]]; then 
#			  aggregatorPort+=($port)
#			  aggregatorHost+=($host)
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
								echo -e "StartOTS.sh [${LINENO}]  \t Launching the Gateway Application on host {${HOSTNAME}}..."
								echo -e "StartOTS.sh [${LINENO}]  \t ===> Quiet mode  redirecting output to *** .otsdaq_quiet_run-${HOSTNAME}-${port}.txt *** (hidden file)"
								echo
								echo
								xdaq.exe -h ${host} -p ${port} -e ${XDAQ_ARGS} &> .otsdaq_quiet_run-${HOSTNAME}-${port}.txt &
							else
								xdaq.exe -h ${host} -p ${port} -e ${XDAQ_ARGS} &
							fi
							#return #FIXME .. could not launch others.. and only launch at MPI restart
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
	
	echo -e "StartOTS.sh [${LINENO}]  \t Launching all otsdaq Applications for host {${HOSTNAME}}..."
	i=0	
	for port in "${xdaqPort[@]}"
	do
	  : 
	  #echo -e "StartOTS.sh [${LINENO}]  \t xdaq.exe -h ${xdaqHost[$i]} -p ${port} -e ${XDAQ_ARGS} &"
	  #echo
		  

		if [[ ${xdaqHost[$i]} != ${HOSTNAME} ]]; then
			continue
		fi
	
		if [ $QUIET == 1 ]; then
		  echo -e "StartOTS.sh [${LINENO}]  \t ===> Quiet mode  redirecting output to *** .otsdaq_quiet_run-${HOSTNAME}-${port}.txt ***  (hidden file)"	
		  xdaq.exe -h ${xdaqHost[$i]} -p ${port} -e ${XDAQ_ARGS} &> .otsdaq_quiet_run-${HOSTNAME}-${port}.txt &
	  else
		  xdaq.exe -h ${xdaqHost[$i]} -p ${port} -e ${XDAQ_ARGS} &
	  fi
	  
	  i=$(( $i + 1 ))
	done
		
	if [[ (${#boardReaderPort[@]} != 0) || (${#builderPort[@]} != 0) || (${#aggregatorPort[@]} != 0) ]] && [[ "${HOSTNAME}" == "${gatewayHostname}" ]]; then
	    cmdstart="mpirun -launcher ssh ${envString}"
	    cmd=""
	    mpiHosts=""
	    i=0

	    for port in "${boardReaderPort[@]}"
	    do
		: 
		#echo -e "StartOTS.sh [${LINENO}]  \t  -np xdaq.exe -h ${boardReaderHost[$i]} -p ${port} -e ${XDAQ_ARGS} :\"
		cmd=$cmd" -np 1 xdaq.exe -h ${boardReaderHost[$i]} -p ${port} -e ${XDAQ_ARGS} :"
		if [[ "x$mpiHosts" == "x" ]]; then
		    mpiHosts="${boardReaderHost[$i]}"
		else
		    mpiHosts=$mpiHosts",${boardReaderHost[$i]}"
		fi
		i=$(( $i + 1 ))
	    done
	    i=0	
	    for port in "${builderPort[@]}"
	    do
		: 
		#echo -e "StartOTS.sh [${LINENO}]  \t  -np xdaq.exe -h ${builderHost[$i]} -p ${port} -e ${XDAQ_ARGS} :\"
		if [ $i -eq 0 ];then
		    cmd=$cmd" -np 1 xdaq.exe -h ${builderHost[$i]} -p ${port} -e ${XDAQ_ARGS} "
		else	
		    cmd=$cmd": -np 1 xdaq.exe -h ${builderHost[$i]} -p ${port} -e ${XDAQ_ARGS} "
		fi
		if [[ "x$mpiHosts" == "x" ]]; then
		    mpiHosts="${builderHost[$i]}"
		else
		    mpiHosts=$mpiHosts",${builderHost[$i]}"
		fi
		i=$(( $i + 1 ))
	    done	
	    i=0	
	    for port in "${aggregatorPort[@]}"
	    do
		: 
		#echo -e "StartOTS.sh [${LINENO}]  \t  -np xdaq.exe -h ${aggregatorHost[$i]} -p ${port} -e ${XDAQ_ARGS}\n"
		cmd=$cmd": -np 1 xdaq.exe -h ${aggregatorHost[$i]} -p ${port} -e ${XDAQ_ARGS} "
		if [[ "x$mpiHosts" == "x" ]]; then
		    mpiHosts="${aggregatorHost[$i]}"
		else
		    mpiHosts=$mpiHosts",${aggregatorHost[$i]}"
		fi
		i=$(( $i + 1 ))
	    done
	    echo
	    cmd=$cmdstart" -host "$mpiHosts$cmd
	    echo Command used to start MPI: $cmd &
	    MPI_RUN_CMD=$cmd
		#if [ $QUIET == 1 ]; then
		#echo -e "StartOTS.sh [${LINENO}]  \t ===> Quiet mode  redirecting output to *** .otsdaq_quiet_run-mpi-${HOSTNAME}.txt ***"		  
		#$cmd &> .otsdaq_quiet_run-mpi-${HOSTNAME}.txt &
		#else
		#$cmd &
		#fi	  
	fi
	
	echo
	echo
	echo
	echo
	
	if [[ ${#contextHostname[@]} == 1 ]]; then 
		echo -e "StartOTS.sh [${LINENO}]  \t This is the ONLY host configured to run xdaq applications: ${contextHostname[@]}" 	    
	  else
		echo -e "StartOTS.sh [${LINENO}]  \t These are the hosts configured to run xdaq applications: ${contextHostname[@]}"
	fi
	echo
	  
	if [[ (${#boardReaderPort[@]} == 0) && (${#builderPort[@]} == 0) && (${#aggregatorPort[@]} == 0) && (${#xdaqPort[@]} == 0) ]]; then
	  echo -e "StartOTS.sh [${LINENO}]  \t ********************************************************************************************************************************"
	  echo -e "StartOTS.sh [${LINENO}]  \t ********************************************************************************************************************************"
	  echo -e "StartOTS.sh [${LINENO}]  \t WARNING: There are no configured processes for hostname ${HOSTNAME}." 
	  echo -e "StartOTS.sh [${LINENO}]  \t Are you sure your configuration is written for ${HOSTNAME}?" 
	 
	  echo -e "StartOTS.sh [${LINENO}]  \t ********************************************************************************************************************************"
	  echo -e "StartOTS.sh [${LINENO}]  \t ********************************************************************************************************************************"
	fi


	printMainURL &
	sleep 1 #so that the terminal comes back after the printouts in quiet mode
}   

export -f launchOTS

#########################################################
#########################################################
#make URL print out a function so that & syntax can be used to run in background (user has immediate terminal access)
printMainURL() {	
	
	#echo -e "StartOTS.sh [${LINENO}]  \t printMainURL()"
	
	#check if StartOTS.sh was aborted
	#OTSDAQ_STARTOTS_ACTION="$(cat ${OTSDAQ_STARTOTS_ACTION_FILE})"
	#	if [ "$OTSDAQ_STARTOTS_ACTION" == "EXIT_LOOP" ]; then
	#		exit
	#	fi
	
	if [ $QUIET == 0 ]; then
		sleep 3
	else
		sleep 2
	fi
	
	echo
	echo
	echo -e "StartOTS.sh [${LINENO}]  \t Open the URL below in your Google Chrome or Mozilla Firefox web browser:"	
	
	
	if [ $MAIN_URL == "unknown_url" ]; then
		echo -e "StartOTS.sh [${LINENO}]  \t INFO: No gateway supervisor found for node {${HOSTNAME}}."
		exit
	fi
	
	for i in {1..5}
	do
		#check if StartOTS.sh was aborted
		#OTSDAQ_STARTOTS_ACTION="$(cat ${OTSDAQ_STARTOTS_ACTION_FILE})"
		#if [ "$OTSDAQ_STARTOTS_ACTION" == "EXIT_LOOP" ]; then
		#exit
		#fi
		
		
		echo -e "StartOTS.sh [${LINENO}]  \t *********************************************************************"
		echo -e "StartOTS.sh [${LINENO}]  \t otsdaq URL = $MAIN_URL"
		echo -e "StartOTS.sh [${LINENO}]  \t *********************************************************************"
		echo
		
		if [ $QUIET == 1 ]; then
			exit
		fi
		sleep 1
	done
}

export -f printMainURL
	

#functions have been declared


if [ $ISCONFIG == 1 ]; then
	launchOTSWiz
else
	launchOTS " Gateway" #only launch gateway once.. otherwise relaunch everything else
fi




#FIXME  -- temporary solution for keeping artdaq mpi alive through reconfiguring
otsActionHandler() {
		
	echo -e "StartOTS.sh [${LINENO}]  \t Starting action handler..."

	#clear file initially
	echo "0" > $OTSDAQ_STARTOTS_ACTION_FILE
	

	if [[ ($ISCONFIG == 1) || ("${HOSTNAME}" == "${gatewayHostname}") ]]; then
		echo -e "StartOTS.sh [${LINENO}]  \t The script, on ${HOSTNAME}, is the gateway StartOTS.sh script, so it will drive exit of other scripts."
		

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
			echo -e "StartOTS.sh [${LINENO}]  \t  "
			echo -e "StartOTS.sh [${LINENO}]  \t Rebuilding. . ."
			echo -e "StartOTS.sh [${LINENO}]  \t  "			
			#echo "1" > mrbresult.num; mrb b > otsdaq_startots_mrbreport.txt && echo "0" > mrbresult.num			
			echo -e "StartOTS.sh [${LINENO}]  \t  "
			#grep -A 1 -B 1 "INFO: Stage build successful." otsdaq_startots_mrbreport.txt
			echo -e "StartOTS.sh [${LINENO}]  \t  "
			sleep 5
		
		elif [ "$OTSDAQ_STARTOTS_ACTION" == "OTS_APP_SHUTDOWN" ]; then
			echo -e "StartOTS.sh [${LINENO}]  \t  "
			echo -e "StartOTS.sh [${LINENO}]  \t Shutting down non-gateway supervisors (TODO). . ."
			echo -e "StartOTS.sh [${LINENO}]  \t  "	
			#TODO
			
		elif [ "$OTSDAQ_STARTOTS_ACTION" == "OTS_APP_STARTUP" ]; then
			echo -e "StartOTS.sh [${LINENO}]  \t  "
			echo -e "StartOTS.sh [${LINENO}]  \t Starting up non-gateway supervisors (TODO). . ."
			echo -e "StartOTS.sh [${LINENO}]  \t  "	
			#TODO
			
		elif [ "$OTSDAQ_STARTOTS_ACTION" == "RESET_MPI" ]; then
		
			#only print first time
			if [ $FIRST_TIME == 1 ]; then
				echo -e "StartOTS.sh [${LINENO}]  \t  "
				echo -e "StartOTS.sh [${LINENO}]  \t Restarting MPI (future restarts will be silent) . . ."
				#echo $MPI_RUN_CMD
				echo -e "StartOTS.sh [${LINENO}]  \t  "
			fi
			
			#killprocs mpirun
			killall -9 mpirun &>/dev/null 2>&1 #hide output
			sleep 1

			export MPIEXEC_PORT_RANGE=8300:8349
			export MPIR_CVAR_CH3_PORT_RANGE=8450:8700

			if [ $QUIET == 1 ]; then
				
				#only print first time
				if [ $FIRST_TIME == 1 ]; then
					echo -e "StartOTS.sh [${LINENO}]  \t ===> Quiet mode  redirecting output to *** .otsdaq_quiet_run-mpi-${HOSTNAME}.txt ***"
					FIRST_TIME=0
				fi
				$MPI_RUN_CMD &> .otsdaq_quiet_run-mpi-${HOSTNAME}.txt &
			else
				$MPI_RUN_CMD &
			fi
			#sleep 5
		elif [ "$OTSDAQ_STARTOTS_ACTION" == "LAUNCH_WIZ" ]; then
			
			echo
			echo -e "StartOTS.sh [${LINENO}]  \t Starting otsdaq Wiz mode for host {${HOSTNAME}}..."
			echo
			killprocs
			#killall -9 xdaq.exe &>/dev/null 2>&1 #hide output
			#killall -9 mf_rcv_n_fwd &>/dev/null 2>&1 #hide output #message viewer display without decoration
			#killall -9 mpirun &>/dev/null 2>&1 #hide output
			sleep 1
			
			launchOTSWiz

		elif [ "$OTSDAQ_STARTOTS_ACTION" == "LAUNCH_OTS" ]; then
				
			echo
			echo -e "StartOTS.sh [${LINENO}]  \t Starting otsdaq in normal mode for host {${HOSTNAME}}..."
			echo
			killprocs
			#killall -9 xdaq.exe &>/dev/null 2>&1 #hide output
			#killall -9 mf_rcv_n_fwd #message viewer display without decoration
			#killall -9 mpirun &>/dev/null 2>&1 #hide output
			sleep 1
			
			launchOTS

		elif [ "$OTSDAQ_STARTOTS_ACTION" == "FLATTEN_TO_SYSTEM_ALIASES" ]; then

			echo
			echo -e "StartOTS.sh [${LINENO}]  \t Removing unused tables and groups based on active System Aliases..."
			echo -e "StartOTS.sh [${LINENO}]  \t otsdaq_flatten_system_aliases 0"
			echo	
			echo 
			if [ $QUIET == 1 ]; then
				echo -e "StartOTS.sh [${LINENO}]  \t ===> Quiet mode  redirecting output to *** .otsdaq_quiet_run-flatten-${HOSTNAME}.txt ***"	
				otsdaq_flatten_system_aliases 0 &> .otsdaq_quiet_run-flatten-${HOSTNAME}.txt &
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
			echo -e "StartOTS.sh [${LINENO}]  \t Exiting StartOTS.sh.. Unrecognized command !=0 in ${OTSDAQ_STARTOTS_ACTION}-${OTSDAQ_STARTOTS_QUIT}-${OTSDAQ_STARTOTS_LOCAL_QUIT}"			
			exit
		fi
		
		echo "0" > $OTSDAQ_STARTOTS_ACTION_FILE
		sleep 1
	done

		
}
export -f otsActionHandler

otsActionHandler &

#moved to launch functions
#printMainURL &

sleep 1 #so that the terminal comes back after the printouts are done ( in quiet mode )

#launch chrome here if enabled
if [ $CHROME == 1 ]; then
	sleep 3 #give time for server to be live
	google-chrome $MAIN_URL &
fi













