cet_make(LIBRARY_NAME PluginMakers
		 LIBRARIES
        ${XDAQ_XERCES-C} 
	${CETLIB}
         ${CETLIB_EXCEPT}
	 MF_MessageLogger
         pthread 
)

install_headers()
install_source()
