cet_make(LIBRARY_NAME SupervisorDescriptorInfo
		 LIBRARIES
		 ${XDAQ_BASIC_LIB_LIST}
         $ENV{XERCES_C_LIB}/libxerces-c.so
         MessageFacility
)


install_headers()
install_source()
