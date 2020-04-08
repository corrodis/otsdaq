cet_make(LIBRARY_NAME WorkLoopManager
		   LIBRARIES
		   XmlUtilities
		   SOAPUtilities
		   ${XDAQ_BASIC_LIB_LIST}
        ${XDAQ_CGICC}
)

install_headers()
install_source()
