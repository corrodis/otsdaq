cet_make(LIBRARY_NAME ControlsCore
		 LIBRARIES
        	 ${XDAQ_BASIC_LIB_LIST}
		 ${CETLIB}
        	 #$ENV{XERCES_C_LIB}/libxerces-c.so
                 ConfigurationInterface

)

install_headers()
install_source()
