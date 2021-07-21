cet_make(LIBRARY_NAME SlowControlsCore
		 LIBRARIES
        	 ${XDAQ_BASIC_LIB_LIST}
		 cetlib
                 ConfigurationInterface
				 Configurable

)

install_headers()
install_source()
