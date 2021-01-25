cet_make(LIBRARY_NAME GatewaySupervisor
		 LIBRARIES
		 ${XDAQ_BASIC_LIB_LIST}		 
         $ENV{XDAQ_LIB}/libxgi.so
         $ENV{POSTGRESQL_LIBRARIES}/libpq.so
		 ConfigurationInterface
		 SupervisorInfo
         WorkLoopManager
         FiniteStateMachine
         CgiDataUtilities
         WebUsersUtilities
         CodeEditor         
         CoreSupervisors
         NetworkUtilities
         
		 otsdaq_TablePlugins_XDAQContextTable_table
		 otsdaq_TablePlugins_IterateTable_table
		 otsdaq_TablePlugins_DesktopIconTable_table
)


install_headers()
install_source()
