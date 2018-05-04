cet_make(LIBRARY_NAME GatewaySupervisor
		 LIBRARIES
		 ${XDAQ_BASIC_LIB_LIST}
		 ConfigurationInterface
		 SupervisorInfo
		 SystemMessenger
         WorkLoopManager
         FiniteStateMachine
         CgiDataUtilities
         WebUsersUtilities
         CoreSupervisors
         NetworkUtilities
         #DetectorWriter
		 artdaq_ExternalComms
		 otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration
		 otsdaq-core_ConfigurationPluginDataFormats_IterateConfiguration_configuration
)


install_headers()
install_source()
