cet_make(LIBRARY_NAME PluginMakers
		 LIBRARIES
         ${CETLIB}
         ${CETLIB_EXCEPT}
         pthread 
)

install_headers()
install_source()
