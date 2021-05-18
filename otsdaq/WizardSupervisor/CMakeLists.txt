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
		 Boost::filesystem
)
# stdc++fs needs to be added to make filesystem library work as of GCC 8.2 , it is fixed on GCC 9.0
install_headers()
install_source()


