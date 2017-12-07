#!/bin/sh
echo
echo "Launching StartOTS.sh otsdaq script... on {${HOSTNAME}}."
echo

ISCONFIG=0
QUIET=1
CHROME=0
DONOTKILL=0

function killprocs {
    if [[ "x$1" == "x" ]]; then
	PIDS=`ps --no-headers axk comm o pid,args|grep mpirun|grep $USER_DATA|awk '{print $1}'`
	PIDS+=" "
	PIDS+=`ps --no-headers axk comm o pid,args|grep xdaq.exe|grep $USER_DATA|awk '{print $1}'`
	PIDS+=" "
	PIDS+=`ps --no-headers axk comm o pid,args|grep mf_rcv_n_fwd|grep $USER_DATA|awk '{print $1}'`
	#echo Killing PIDs: $PIDS
	kill $PIDS >/dev/null 2>&1
	kill -9 $PIDS >/dev/null 2>&1
    else
	PIDS=`ps --no-headers axk comm o pid,args|grep $1|grep $USER_DATA|awk '{print $1}'`
	#echo Killing PIDs: $PIDS
	kill $PIDS >/dev/null 2>&1
	kill -9 $PIDS >/dev/null 2>&1
    fi
}

#check for wiz mode
if [[ "$1"  == "--config" || "$1"  == "--configure" || "$1"  == "--wizard" || "$1"  == "--wiz" || "$1"  == "-w" ]]; then
	echo "*****************************************************"
	echo "*************      WIZ MODE ENABLED!    *************"
	echo "*****************************************************"
    ISCONFIG=1
fi


if [[ "$1"  == "--verbose" || "$2"  == "--verbose" || "$1"  == "-v" || "$2"  == "-v"  ]]; then
	echo "*************   VERBOSE MODE ENABLED!    ************"
	QUIET=0
fi

if [[ "$1"  == "--chrome" || "$2"  == "--chrome" || "$1"  == "-c" || "$2"  == "-c"  ]]; then
	echo "*************   GOOGLE-CHROME LAUNCH ENABLED!    ************"
	CHROME=1
fi

if [[ "$1"  == "--donotkill" || "$2"  == "--donotkill" || "$1"  == "-d" || "$2"  == "-d"  ]]; then
	echo "*************   DO-NOT-KILL ENABLED!    ************"
	DONOTKILL=1
fi

#############################
#initializing StartOTS action file
#attempt to mkdir for full path so that it exists to move the database to
# assuming mkdir is non-destructive

OTSDAQ_STARTOTS_ACTION_FILE="${USER_DATA}/ServiceData/StartOTS_action_${HOSTNAME}.cmd"
echo "StartOTS_action file path = ${OTSDAQ_STARTOTS_ACTION_FILE}"

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
    echo "******************************************************"
	echo "*************    KILLING otsdaq!        **************"
    echo "******************************************************"

    killprocs
	#killall -9 mpirun &>/dev/null #hide output
	#killall -9 xdaq.exe &>/dev/null #hide output
	#killall -9 mf_rcv_n_fwd &>/dev/null #hide output #message viewer display without decoration

	exit
fi

if [[ $ISCONFIG == 0 && $QUIET == 1 && $CHROME == 0 && $DONOTKILL == 0 && "$1x" != "x" ]]; then
	echo 
	echo "Unrecognized paramater $1"
	echo
    echo "******************************************************"
	echo "*************    StartOTS.sh Usage      **************"
    echo "******************************************************"
	echo
	echo "To start otsdaq in 'wiz mode' please use any of these options:"
	echo "	--configure  --config  --wizard  --wiz  -w"
	echo "	e.g.: StartOTS.sh --wiz"
	echo
	echo "To start otsdaq with 'verbose mode' enabled, add one of these options:"
	echo "	--verbose  -v"
	echo "	e.g.: StartOTS.sh --wiz -v     or    StartOTS.sh --verbose"
	echo
	echo "To start otsdaq and launch google-chrome, add one of these options:"
	echo "	--chrome  -c"
	echo "	e.g.: StartOTS.sh --wiz -c     or    StartOTS.sh --chrome"
	echo
	echo "To start otsdaq and launch google-chrome, add one of these options:"
	echo "	--donotkill  -d"
	echo "	e.g.: StartOTS.sh --wiz -d     or    StartOTS.sh --donotkill"
	echo
	echo "To kill all otsdaq running processes, please use any of these options:"
	echo "	--killall  --kill  --kx  -k"
	echo "	e.g.: StartOTS.sh --kx"
	echo
	echo "aborting StartOTS.sh..."
	exit
fi



SERVER=`hostname -f || ifconfig eth0|grep "inet addr"|cut -d":" -f2|awk '{print $1}'`
export SUPERVISOR_SERVER=$SERVER
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
#echo "WEB_PATH=$WEB_PATH"
#echo "USER_WEB_PATH=$USER_WEB_PATH"
#echo "Making symbolic link to USER_WEB_PATH"
#echo "ln -s $USER_WEB_PATH $WEB_PATH/UserWebPath"
ln -s $USER_WEB_PATH $WEB_PATH/UserWebPath &>/dev/null #hide output


if [ "x$USER_DATA" == "x" ]; then
	echo
	echo "Error."
	echo "Environment variable USER_DATA not setup!"
	echo "To setup, use 'export USER_DATA=<path to user data>'"
	echo 
	echo
	echo "(If you do not have a user data folder copy '<path to ots source>/otsdaq-demo/Data' as your starting point.)"
	echo
	exit    
fi

if [ ! -d $USER_DATA ]; then
	echo
	echo "Error."
	echo "USER_DATA=$USER_DATA"
	echo "Environment variable USER_DATA does not point to a valid directory!"
	echo "To setup, use 'export USER_DATA=<path to user data>'"
	echo 
	echo
	echo "(If you do not have a user data folder copy '<path to ots source>/otsdaq-demo/Data' as your starting point.)"
	echo
	exit   
fi

echo "Environment variable USER_DATA is setup and points to folder" ${USER_DATA}

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
    export OTSDAQ_LOG_DIR="${OTSDAQDEMO_BUILD}/Logs"
fi

if [ "x${ARTDAQ_OUTPUT_DIR}" == "x" ]; then
    export ARTDAQ_OUTPUT_DIR="${OTSDAQDEMO_BUILD}"
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
	echo "Killing all existing otsdaq Applications..."
	killprocs
	#killall -9 mpirun &>/dev/null #hide output
	#killall -9 xdaq.exe &>/dev/null #hide output
	#killall -9 mf_rcv_n_fwd &>/dev/null #hide output #message viewer display without decoration

	#give time for killall
	sleep 1
	echo "...Applications killed!"
fi



#echo "ARTDAQ_MFEXTENSIONS_DIR=" ${ARTDAQ_MFEXTENSIONS_DIR}

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
	
	####################################################################
	########### start console & message facility handling ##############
	####################################################################
	#decide which MessageFacility console viewer to run
	# and configure otsdaq MF library with MessageFacility*.fcl to use
	
	export OTSDAQ_LOG_FHICL=${USER_DATA}/MessageFacilityConfigurations/MessageFacilityGen.fcl
	#this fcl tells the MF library used by ots source how to behave
	#echo "OTSDAQ_LOG_FHICL=" ${OTSDAQ_LOG_FHICL}
	
	
	USE_WEB_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseWebConsole.bool)"
	USE_QT_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseQTViewer.bool)"
			
	
	#echo "USE_WEB_VIEWER" ${USE_WEB_VIEWER}
	#echo "USE_QT_VIEWER" ${USE_QT_VIEWER}
	
	
	if [[ $USE_WEB_VIEWER == "1" ]]; then
		echo "CONSOLE: Using web console viewer"
		
		#start quiet forwarder with receiving port and destination port parameter file
	
		if [ $QUIET == 1 ]; then
			echo "Quiet mode redirecting output to *** otsdaq_quiet_run-mf.txt ***"
			mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarderGen.cfg  &> otsdaq_quiet_run-mf.txt &
		else
			mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarderGen.cfg  &
		fi		 	
	fi
	
	if [[ $USE_QT_VIEWER == "1" ]]; then
		echo "CONSOLE: Using QT console viewer"
		if [ "x$ARTDAQ_MFEXTENSIONS_DIR" == "x" ]; then #qtviewer library missing!
			echo
			echo "Error: ARTDAQ_MFEXTENSIONS_DIR missing for qtviewer!"
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

	ARTDAQ_DATABASE_DIR=`echo ${ARTDAQ_DATABASE_URI}|sed 's|.*//|/|'`
		
	echo "Checking for database migration at ${ARTDAQ_DATABASE_URI}..."
	if [ ! -e ${ARTDAQ_DATABASE_DIR}/fromIndexRebuild ]; then
		# Rebuild ARTDAQ_DATABASE indicies
		rebuild_database_index >/dev/null 2>&1; rebuild_database_index --uri=${ARTDAQ_DATABASE_URI} >/dev/null 2>&1
		
		mv ${ARTDAQ_DATABASE_DIR} ${ARTDAQ_DATABASE_DIR}.bak.$$		
		mv ${ARTDAQ_DATABASE_DIR}_new ${ARTDAQ_DATABASE_DIR}
		echo "rebuilt" > ${ARTDAQ_DATABASE_DIR}/fromIndexRebuild
	else
		echo "${ARTDAQ_DATABASE_DIR}/fromIndexRebuild file exists, so not rebuilding indices."
	fi
	
	#substitute environment variables into template wiz-mode xdaq config xml
	envsubst <${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake.xml > ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake_Run.xml
	
	#use safe Message Facility fcl in config mode
	export OTSDAQ_LOG_FHICL=${USER_DATA}/MessageFacilityConfigurations/MessageFacility.fcl #MessageFacilityWithCout.fcl
	
	echo ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake_Run.xml
			
	if [ $QUIET == 1 ]; then
		echo "Quiet mode redirecting output to *** otsdaq_quiet_run-wiz.txt ***"
		xdaq.exe -p ${PORT} -h ${HOSTNAME} -e ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfiguration_CMake.xml -c ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake_Run.xml &> otsdaq_quiet_run-wiz.txt &
	else
		xdaq.exe -p ${PORT} -h ${HOSTNAME} -e ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfiguration_CMake.xml -c ${XDAQ_CONFIGURATION_DATA_PATH}/otsConfigurationNoRU_Wizard_CMake_Run.xml &
	fi

	################
	# start node db server
	
	#echo "ARTDAQ_UTILITIES_DIR=" ${ARTDAQ_UTILITIES_DIR}
	#cd $ARTDAQ_UTILITIES_DIR/node.js
	#as root, once...
	# chmod +x setupNodeServer.sh 
	# ./setupNodeServer.sh 
	# chown -R products:products *
	
	#uncomment to use artdaq db nodejs web gui
	#node serverbase.js > /tmp/${USER}_serverbase.log &
	
	sleep 2
	MAIN_URL="http://${HOSTNAME}:${PORT}/urn:xdaq-application:lid=$OTS_CONFIGURATION_WIZARD_SUPERVISOR_ID/Verify?code=$(cat ${SERVICE_DATA_PATH}//OtsWizardData/sequence.out)"
	#echo $MAIN_URL
}

####################################################################
####################################################################
################## Normal Mode OTS Launch ###########################
####################################################################
####################################################################
#make URL print out a function so that & syntax can be used to run in background (user has immediate terminal access)
launchOTS() {

	####################################################################
	########### start console & message facility handling ##############
	####################################################################
	#decide which MessageFacility console viewer to run
	# and configure otsdaq MF library with MessageFacility*.fcl to use
	
	export OTSDAQ_LOG_FHICL=${USER_DATA}/MessageFacilityConfigurations/MessageFacilityGen.fcl
	#this fcl tells the MF library used by ots source how to behave
	#echo "OTSDAQ_LOG_FHICL=" ${OTSDAQ_LOG_FHICL}
	
	
	USE_WEB_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseWebConsole.bool)"
	USE_QT_VIEWER="$(cat ${USER_DATA}/MessageFacilityConfigurations/UseQTViewer.bool)"
			
	
	#echo "USE_WEB_VIEWER" ${USE_WEB_VIEWER}
	#echo "USE_QT_VIEWER" ${USE_QT_VIEWER}
	
	
	if [[ $USE_WEB_VIEWER == "1" ]]; then
		echo "CONSOLE: Using web console viewer"
		
		#start quiet forwarder with receiving port and destination port parameter file
	
		if [ $QUIET == 1 ]; then
			echo "Quiet mode redirecting output to *** otsdaq_quiet_run-mf.txt ***"
			mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarderGen.cfg  &> otsdaq_quiet_run-mf.txt &
		else
			mf_rcv_n_fwd ${USER_DATA}/MessageFacilityConfigurations/QuietForwarderGen.cfg  &
		fi		 	
	fi
	
	if [[ $USE_QT_VIEWER == "1" ]]; then
		echo "CONSOLE: Using QT console viewer"
		if [ "x$ARTDAQ_MFEXTENSIONS_DIR" == "x" ]; then #qtviewer library missing!
			echo
			echo "Error: ARTDAQ_MFEXTENSIONS_DIR missing for qtviewer!"
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
	NODESERVERPS="$(netstat -apn | grep node | grep 8080 | grep LISTEN | rev | cut -d'.' -f1 | cut -c 16-22 | rev)"
	#killprocs $NODESERVERPS
        #kill -9 $NODESERVERPS
			
	envString="-genv OTSDAQ_LOG_ROOT ${OTSDAQ_LOG_DIR} -genv ARTDAQ_OUTPUT_DIR ${ARTDAQ_OUTPUT_DIR}"
	
	#echo "XDAQ Configuration XML:"
	#echo ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml
	export XDAQ_ARGS="${XDAQ_CONFIGURATION_DATA_PATH}/otsConfiguration_CMake.xml -c ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml"
	#echo
	#echo "XDAQ ARGS PASSED TO xdaq.exe:"
	#echo ${XDAQ_ARGS}
	#echo
	#echo
	value=`cat ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml`
	#echo "$value"
	#re="http://(${HOSTNAME}):([0-9]+)"
	re="http(s*)://(.+):([0-9]+)"
	superRe="id=\"([0-9]+)\""		
	#echo "MATCHING REGEX"
	haveXDAQContextPort=false
	insideContext=false
	ignore=false
	isLocal=false
	mainHostname=""
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
			#echo "------------------------------------------>>>>>>>>"
	
		fi
		if [[ $line == *"/xc:Context"* ]]; then
			insideContext=false
			haveXDAQContextPort=false
			#echo "<<<<<<<------------------------------------------"
		fi
		if [[ ($insideContext == true) ]]; then 
			if [[ ($line == *"ots::ARTDAQDataManagerSupervisor"*) ]]; then
			  boardReaderPort+=($port)
			  boardReaderHost+=($host)
			elif [[ ($line == *"ots::EventBuilderApp"*) ]]; then
			  builderPort+=($port)
			  builderHost+=($host)
			elif [[ ($line == *"ots::AggregatorApp"*) ]]; then 
			  aggregatorPort+=($port)
			  aggregatorHost+=($host)
			elif [[ ($line == *"class"*) ]] && [[ "${isLocal}" == "true" ]]; then #IT'S A XDAQ SUPERVISOR		
	
				if [[ ($haveXDAQContextPort == false) ]]; then 
					xdaqPort+=($port)
					xdaqHost+=($host)
					haveXDAQContextPort=true
				fi
				
				if [[ ($line == *"ots::Supervisor"*) ]]; then #IT's the SUPER supervisor, record LID 
					if [[ ($line =~ $superRe) ]]; then
					    mainHostname=${host}
						#echo ${BASH_REMATCH[1]}	#should be supervisor LID
						MAIN_URL="http://${host}:${port}/urn:xdaq-application:lid=${BASH_REMATCH[1]}/"
					fi
				fi
			  #IF THERE IS AT LEAST ONE NOT ARTDAQ APP THEN I CAN GET OUT OF THIS CONTEXT AND RUN XDAQ ONCE JUST FOR THIS
			  #insideContext=false #RAR commented because need Super Supervisor connection LID for URL
			  #echo $line          
			fi
		fi   
	done < ${XDAQ_CONFIGURATION_DATA_PATH}/${XDAQ_CONFIGURATION_XML}.xml
	
	echo
	echo "Launching all otsdaq Applications for host {${HOSTNAME}}..."
	i=0	
	for port in "${xdaqPort[@]}"
	do
	  : 
	  echo "xdaq.exe -h ${xdaqHost[$i]} -p ${port} -e ${XDAQ_ARGS} &"
	  echo
		  
	
	  if [ $QUIET == 1 ]; then
		  echo "Quiet mode redirecting output to *** otsdaq_quiet_run-${port}.txt ***"
		  xdaq.exe -h ${xdaqHost[$i]} -p ${port} -e ${XDAQ_ARGS} &> otsdaq_quiet_run-${port}.txt &
	  else
		  xdaq.exe -h ${xdaqHost[$i]} -p ${port} -e ${XDAQ_ARGS} &
	  fi
	  
	  i=$(( $i + 1 ))
	done
	
	if [[ (${#boardReaderPort[@]} != 0) || (${#builderPort[@]} != 0) || (${#aggregatorPort[@]} != 0) ]] && [[ "${HOSTNAME}" == "${mainHostname}" ]]; then
	  cmdstart="mpirun -launcher ssh ${envString}"
	  cmd=""
	  mpiHosts=""
	  i=0	
	  for port in "${boardReaderPort[@]}"
	  do
		: 
		#echo " -np xdaq.exe -h ${boardReaderHost[$i]} -p ${port} -e ${XDAQ_ARGS} :\"
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
		#echo " -np xdaq.exe -h ${builderHost[$i]} -p ${port} -e ${XDAQ_ARGS} :\"
		cmd=$cmd" -np 1 xdaq.exe -h ${builderHost[$i]} -p ${port} -e ${XDAQ_ARGS} :"
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
		#echo " -np xdaq.exe -h ${aggregatorHost[$i]} -p ${port} -e ${XDAQ_ARGS}\n"
		cmd=$cmd" -np 1 xdaq.exe -h ${aggregatorHost[$i]} -p ${port} -e ${XDAQ_ARGS}"
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
		#echo "Quiet mode redirecting output to *** otsdaq_quiet_run-mpi.txt ***"		  
		#$cmd &> otsdaq_quiet_run-mpi.txt &
		#else
		#$cmd &
		#fi
		  
	fi
	
	if [[ (${#boardReaderPort[@]} == 0) && (${#builderPort[@]} == 0) && (${#aggregatorPort[@]} == 0) && (${#xdaqPort[@]} == 0) ]]; then
	  echo "********************************************************************************************************************************"
	  echo "********************************************************************************************************************************"
	  echo "WARNING: There are no configured processes for hostname ${HOSTNAME}." 
	  echo "Are you sure your configuration is written for ${HOSTNAME}?" 
	  if [[ ${#contextHostname[@]} == 1 ]]; then 
		echo "This is the ONLY host configured to run xdaq applications: ${contextHostname[@]}" 	    
	  else
		echo "These are the hosts configured to run xdaq applications: ${contextHostname[@]}"
	  fi
	  echo "********************************************************************************************************************************"
	  echo "********************************************************************************************************************************"
	fi
}   

export -f launchOTS


#make URL print out a function so that & syntax can be used to run in background (user has immediate terminal access)
printMainURL() {	

	#check if StartOTS.sh was aborted
	OTSDAQ_STARTOTS_ACTION="$(cat ${OTSDAQ_STARTOTS_ACTION_FILE})"
	if [ "$OTSDAQ_STARTOTS_ACTION" == "EXIT_LOOP" ]; then
		exit
	fi
	
	echo
	echo "Open the URL below in your Google Chrome or Mozilla Firefox web browser:"
	if [ $QUIET == 0 ]; then
		sleep 3
	fi
	
	if [ $MAIN_URL == "unknown_url" ]; then
		echo "INFO: No gateway supervisor found for node {${HOSTNAME}}."
		exit
	fi
	
	for i in {1..5}
	do
		#check if StartOTS.sh was aborted
		OTSDAQ_STARTOTS_ACTION="$(cat ${OTSDAQ_STARTOTS_ACTION_FILE})"
		if [ "$OTSDAQ_STARTOTS_ACTION" == "EXIT_LOOP" ]; then
			exit
		fi
		
		echo
		echo "*********************************************************************"
		echo $MAIN_URL
		echo "*********************************************************************"
		
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
	launchOTS 
fi




#FIXME  -- temporary solution for keeping artdaq mpi alive through reconfiguring
otsActionHandler() {
		
	
	
	#clear file initially
	echo "0" > $OTSDAQ_STARTOTS_ACTION_FILE
	
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
		
		if [ "$OTSDAQ_STARTOTS_ACTION" == "REBUILD_OTS" ]; then
			echo " "
			echo "Rebuilding. . ."
			echo " "			
			#echo "1" > mrbresult.num; mrb b > otsdaq_startots_mrbreport.txt && echo "0" > mrbresult.num			
			echo " "
			#grep -A 1 -B 1 "INFO: Stage build successful." otsdaq_startots_mrbreport.txt
			echo " "
			sleep 5
		elif [ "$OTSDAQ_STARTOTS_ACTION" == "RESET_MPI" ]; then
		
			#only print first time
			if [ $FIRST_TIME == 1 ]; then
				echo " "
				echo "Restarting MPI (future restarts will be silent) . . ."
				#echo $MPI_RUN_CMD
				echo " "
			fi
			
			killprocs mpirun
			#killall -9 mpirun
			sleep 1

			export MPIEXEC_PORT_RANGE=8300:8349
			export MPIR_CVAR_CH3_PORT_RANGE=8450:8700

			if [ $QUIET == 1 ]; then
				
				#only print first time
				if [ $FIRST_TIME == 1 ]; then
					echo "Quiet mode redirecting output to *** otsdaq_quiet_run-mpi.txt ***"
					FIRST_TIME=0
				fi
				$MPI_RUN_CMD &> otsdaq_quiet_run-mpi.txt &
			else
				$MPI_RUN_CMD &
			fi
			#sleep 5
		elif [ "$OTSDAQ_STARTOTS_ACTION" == "LAUNCH_WIZ" ]; then
			
			echo
			echo "Starting otsdaq Wiz mode for host {${HOSTNAME}}..."
			echo
			killprocs
			#killall -9 xdaq.exe
			#killall -9 mf_rcv_n_fwd #message viewer display without decoration
			#killall -9 mpirun
			sleep 1
			
			launchOTSWiz

		elif [ "$OTSDAQ_STARTOTS_ACTION" == "LAUNCH_OTS" ]; then
				
			echo
			echo "Starting otsdaq in normal mode for host {${HOSTNAME}}..."
			echo
			killprocs
			#killall -9 xdaq.exe
			#killall -9 mf_rcv_n_fwd #message viewer display without decoration
			#killall -9 mpirun
			sleep 1
			
			launchOTS

		elif [ "$OTSDAQ_STARTOTS_ACTION" == "FLATTEN_TO_SYSTEM_ALIASES" ]; then

			echo
			echo "Removing unused tables and groups based on active System Aliases..."
			echo "otsdaq_flatten_system_aliases 0"
			echo	
			echo 
			if [ $QUIET == 1 ]; then
				echo "Quiet mode redirecting output to *** otsdaq_quiet_run-flatten.txt ***"	
				otsdaq_flatten_system_aliases 0 &> otsdaq_quiet_run-flatten.txt &
			else
				otsdaq_flatten_system_aliases 0 &
			fi		
						
		elif [ "$OTSDAQ_STARTOTS_ACTION" == "EXIT_LOOP" ]; then
		    exit
		elif [ "$OTSDAQ_STARTOTS_ACTION" != "0" ]; then
			echo "Exiting StartOTS.sh.. Unrecognized command OTSDAQ_STARTOTS_ACTION=${OTSDAQ_STARTOTS_ACTION}"			
			exit
		fi
		
		echo "0" > $OTSDAQ_STARTOTS_ACTION_FILE
		sleep 1
	done

		
}
export -f otsActionHandler

otsActionHandler &

printMainURL &
sleep 1 #so that the terminal comes back after the printouts in quiet mode

#launch chrome here if enabled
if [ $CHROME == 1 ]; then
	sleep 3 #give time for server to be live
	google-chrome $MAIN_URL &
fi













