
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
		 cetlib
         artdaq_Application
         artdaq_BuildInfo
         artdaq_DAQdata
         artdaq_utilities::artdaq-utilities_Plugins
		 art_Persistency_Provenance
		 ${Boost_SYSTEM_LIBRARY}
		 art_Utilities
		 cetlib
		 fhiclcpp
	#	 MessageFacility
		 ${MF_UTILITIES}
         ${ROOT_CORE}
)

install_headers()
install_source()
