cet_make(LIBRARY_NAME PluginMakers
		 LIBRARIES
         $ENV{XERCES_C_LIB}/libxerces-c.so
         ${CETLIB}
         ${CETLIB_EXCEPT}
         pthread 
)

install_headers()
install_source()
