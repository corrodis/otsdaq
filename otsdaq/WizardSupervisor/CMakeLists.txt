cet_make(LIBRARY_NAME WizardSupervisor
		 LIBRARIES
		 ${XDAQ_BASIC_LIB_LIST}
		 ConfigurationInterface
		 TableCore
		 SupervisorInfo
		 GatewaySupervisor
		 CoreSupervisors
		 CgiDataUtilities
		 WebUsersUtilities
		 CodeEditor
		 Boost::filesystem
)
install_headers()
install_source()


