
cet_make(LIBRARY_NAME FECore
		 LIBRARIES
         ${XDAQ_BASIC_LIB_LIST}
		 Configurable
		 SupervisorInfo
         WorkLoopManager
         FiniteStateMachine
         CgiDataUtilities
         WebUsersUtilities
         NetworkUtilities
         PluginMakers
         DataManager
		 ${CETLIB}
         artdaq_Application
         artdaq_BuildInfo
         artdaq_DAQdata
         artdaq_utilities::artdaq-utilities_Plugins
		 ${ART_PERSISTENCY_PROVENANCE}
		 ${Boost_SYSTEM_LIBRARY}
		 ${ART_UTILITIES}
		 ${CETLIB}
		 ${FHICLCPP}
	#	 MessageFacility
		 ${MF_UTILITIES}
         ${ROOT_CORE}
)

install_headers()
install_source()
