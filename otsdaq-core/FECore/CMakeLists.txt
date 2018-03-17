include_directories(${MPI_INCLUDE_PATH})
include_directories(${CMAKE_BINARY_DIR}/artdaq)

cet_make(LIBRARY_NAME FECore
		 LIBRARIES
         ${XDAQ_BASIC_LIB_LIST}
		 Configurable
		 SupervisorInfo
		 SystemMessenger
         WorkLoopManager
         FiniteStateMachine
         CgiDataUtilities
         WebUsersUtilities
         NetworkUtilities
         PluginMakers
         DataManager
		 ${CETLIB}
         artdaq_Application
         artdaq_Application_MPI2
         artdaq_BuildInfo
		 ${ARTDAQ_APPLICATION}
		 ${ARTDAQ_APPLICATION_MPI2}		 
		 ${ARTDAQ_BUILDINFO}
		 ${ART_PERSISTENCY_PROVENANCE}		 
		 ${Boost_SYSTEM_LIBRARY}
		 ${ART_UTILITIES}
		 ${CETLIB}
		 ${FHICLCPP}
	#	 MessageFacility
		 ${MF_UTILITIES}
         ${XDAQ_BASIC_LIB_LIST}
         ${ROOT_CORE}         
)

install_headers()
install_source()
