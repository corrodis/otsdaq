cet_make(LIBRARY_NAME SupervisorInfo
		 LIBRARIES
		 ${XDAQ_BASIC_LIB_LIST}
         $ENV{XERCES_C_LIB}/libxerces-c.so
 #       MessageFacility
		 otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration
)


install_headers()
install_source()
