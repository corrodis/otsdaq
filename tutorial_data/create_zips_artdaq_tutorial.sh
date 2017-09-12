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


#from otsdaq, move artdaq to repo
rm -rf ${SRC}/databases_tutorial_artdaq_demo; 
cp -r ${UDATABASES} ${SRC}/databases_tutorial_artdaq_demo;
rm -rf ${SRC}/databases_tutorial_artdaq_demo/filesystemdb/test_db.*; 
rm -rf ${SRC}/databases_tutorial_artdaq_demo/filesystemdb/test_db_*;

rm -rf ${SRC}/NoGitData_tutorial_artdaq_demo; 
cp -r ${UDATA} ${SRC}/NoGitData_tutorial_artdaq_demo; 
rm ${SRC}/NoGitData_tutorial_artdaq_demo/ServiceData/ActiveConfigurationGroups.cfg.*; 
rm -rf ${SRC}/NoGitData_tutorial_artdaq_demo/ConfigurationInfo.*



#from home/tmp, make artdaq zips from repo
rm tutorial_artdaq_Data.zip; 
mv NoGitData NoGitData.mv.bk; 
cp -r ${SRC}/NoGitData_tutorial_artdaq_demo NoGitData;
zip -r tutorial_artdaq_Data.zip NoGitData; 
rm -rf NoGitData; 
mv NoGitData.mv.bk NoGitData; 

rm -r tutorial_artdaq_database.zip;
mv databases databases.mv.bk; 
cp -r ${SRC}/databases_tutorial_artdaq_demo databases; 
zip -r tutorial_artdaq_database.zip databases; 
rm -rf databases; 
mv databases.mv.bk databases;





echo
echo "Done creating UserData and UserDatabases zips."
echo
	