cet_make(LIBRARY_NAME SlowControlsCore
		 LIBRARIES
        	 ${XDAQ_BASIC_LIB_LIST}
		 ${CETLIB}
                 ConfigurationInterface
				 Configurable

)

install_headers()
install_source()
