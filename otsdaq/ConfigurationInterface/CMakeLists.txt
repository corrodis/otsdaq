
cet_make(LIBRARY_NAME ConfigurationInterface
		NO_PLUGINS
		EXCLUDE FileConfigurationInterface.cc DatabaseConfigurationInterface.cc
		 LIBRARIES
		 TableCore
		 PluginMakers
		 ProgressBar
		cetlib_except
		artdaq_database::artdaq-database_ConfigurationDB
)

basic_plugin(Database "configInterface"
ConfigurationInterface
artdaq_database::artdaq-database_ConfigurationDB
)

basic_plugin(File "configInterface"
ConfigurationInterface)

install_headers()
install_source()
