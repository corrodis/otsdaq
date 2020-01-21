cet_make(LIBRARY_NAME WebUsersUtilities
		 LIBRARIES
		 ${XDAQ_BASIC_LIB_LIST}
		 crypto
		 SOAPUtilities
		 XmlUtilities
		 CgiDataUtilities
		 TableCore
		 SupervisorInfo
		 Macros
)

install_headers()
install_source()
