cet_make(LIBRARY_NAME SlowControlsCore
		 LIBRARIES
        	 ${XDAQ_BASIC_LIB_LIST}
		 ${CETLIB}
        	 #$ENV{XERCES_C_LIB}/libxerces-c.so
                 ConfigurationInterface
				 Configurable

)

install_headers()
install_source()
