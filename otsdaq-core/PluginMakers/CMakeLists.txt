cet_make(LIBRARY_NAME PluginMakers
		 LIBRARIES
        ${XDAQ_XERCES-C} 
	${CETLIB}
         ${CETLIB_EXCEPT}
         ${Boost_SYSTEM_LIBRARY}
	 MF_MessageLogger
         pthread 
)

install_headers()
install_source()
