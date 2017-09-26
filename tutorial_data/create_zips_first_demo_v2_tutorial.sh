#!/bin/sh
echo
echo "Creating UserData and UserDatabases zips..."
echo

SRC='/home/rrivera/ots/srcs/otsdaq/tutorial_data'
UDATA='/home/rrivera/ots/NoGitData'
UDATABASES='/home/rrivera/databases'
		
if [[ "$1"  == "--src" && "x$2" != "x" ]]; then
	SRC="$2"
fi

if [[ "$3"  == "--user" && "x$4" != "x" ]]; then
	UDATA="$4/NoGitData"
	UDATABASES="$4/databases"
fi

if [ "x$1" != "x" ]; then
	"usage: --src <path to repo area> --user <path to user data>"
	exit
fi
		
SRC="${SRC}"

echo "source directory: ${SRC}/"
echo "user data directory: ${UDATA}"
echo "user databases directory: ${UDATABASES}"


#from otsdaq, move demo to repo
rm -rf ${SRC}/databases_tutorial_first_demo_v2; 
cp -r ${UDATABASES} ${SRC}/databases_tutorial_first_demo_v2;
rm -rf ${SRC}/databases_tutorial_first_demo_v2/filesystemdb/test_db.*; 
rm -rf ${SRC}/databases_tutorial_first_demo_v2/filesystemdb/test_db_*;

rm -rf ${SRC}/NoGitData_tutorial_first_demo_v2; 
cp -r ${UDATA} ${SRC}/NoGitData_tutorial_first_demo_v2; 
rm ${SRC}/NoGitData_tutorial_first_demo_v2/ServiceData/ActiveConfigurationGroups.cfg.*; 
rm -rf ${SRC}/NoGitData_tutorial_first_demo_v2/ConfigurationInfo.*
rm -rf ${SRC}/NoGitData_tutorial_first_demo_v2/OutputData/*
rm -rf ${SRC}/NoGitData_tutorial_first_demo_v2/ServiceData/RunNumber/*
rm -rf ${SRC}/NoGitData_tutorial_first_demo_v2/ServiceData/MacroHistory/*


#from home/tmp, make demo zips from repo
rm tutorial_Data_v2.zip; 
mv NoGitData NoGitData.mv.bk; 
cp -r ${SRC}/NoGitData_tutorial_first_demo_v2 NoGitData;
zip -r tutorial_Data_v2.zip NoGitData; 
rm -rf NoGitData; 
mv NoGitData.mv.bk NoGitData; 

rm -r tutorial_database_v2.zip;
mv databases databases.mv.bk; 
cp -r ${SRC}/databases_tutorial_first_demo_v2 databases; 
zip -r tutorial_database_v2.zip databases; 
rm -rf databases; 
mv databases.mv.bk databases;





echo
echo "Done creating UserData and UserDatabases zips."
echo
	