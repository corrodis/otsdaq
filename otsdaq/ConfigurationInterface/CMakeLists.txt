
cet_make(LIBRARY_NAME ConfigurationInterface
		 LIBRARIES
		 TableCore
		 PluginMakers
		 ProgressBar
		cetlib_except
		artdaq_database::artdaq-database_ConfigurationDB
)


install_headers()
install_source()
