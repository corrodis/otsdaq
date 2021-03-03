include_directories($ENV{XERCES_C_INC})
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
		 ${Boost_FILESYSTEM_LIBRARY}
)

install_headers()
install_source()
