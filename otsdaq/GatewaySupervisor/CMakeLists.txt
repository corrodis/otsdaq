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
         CodeEditor         
         CoreSupervisors
         NetworkUtilities
         CodeEditor
         
		 #artdaq_ExternalComms
		 otsdaq_TablePlugins_XDAQContextTable_table
		 otsdaq_TablePlugins_IterateTable_table
		 otsdaq_TablePlugins_DesktopIconTable_table
)


install_headers()
install_source()
