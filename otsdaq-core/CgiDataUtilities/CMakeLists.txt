cet_make(LIBRARY_NAME CgiDataUtilities
		LIBRARIES
		pthread
		dl
		${XDAQ_BASIC_LIB_LIST}
        ${XDAQ_CGICC}
	MF_MessageLogger
)

install_headers()
install_source()
