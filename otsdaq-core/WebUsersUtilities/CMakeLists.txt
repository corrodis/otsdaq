cet_make(LIBRARY_NAME WebUsersUtilities
		 LIBRARIES
		 ${XDAQ_BASIC_LIB_LIST}
		 crypto
		 SOAPUtilities
		 XmlUtilities
		 CgiDataUtilities
		 ConfigurationDataFormats
		 SupervisorDescriptorInfo
)

install_headers()
install_source()
