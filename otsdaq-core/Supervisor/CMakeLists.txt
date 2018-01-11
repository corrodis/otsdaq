cet_make(LIBRARY_NAME Supervisor
		 LIBRARIES
		 ${XDAQ_BASIC_LIB_LIST}
		 ConfigurationInterface
		 SupervisorDescriptorInfo
		 SystemMessenger
         WorkLoopManager
         FiniteStateMachine
         CgiDataUtilities
         WebUsersUtilities
         CoreSupervisors
         NetworkUtilities
         #DetectorWriter
		 otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration
		 otsdaq-core_ConfigurationPluginDataFormats_IterateConfiguration_configuration
)


install_headers()
install_source()
