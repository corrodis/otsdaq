cet_make(LIBRARY_NAME SOAPUtilities
		   LIBRARIES
		   ${XDAQ_BASIC_LIB_LIST}
           $ENV{XERCES_C_LIB}/libxerces-c.so
)

install_headers()
install_source()
