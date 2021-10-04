
cet_make(LIBRARY_NAME ConfigurationInterface
		NO_PLUGINS
		EXCLUDE File_configInterface.cc Database_configInterface.cc Database_configInterface_Impl.cc
		 LIBRARIES
		 TableCore
		 PluginMakers
		 ProgressBar
		cetlib_except
		artdaq_database::artdaq-database_ConfigurationDB
)

basic_plugin(Database "configInterface"
IMPL_SOURCE Database_configInterface_Impl.cc
LIBRARIES
ConfigurationInterface
artdaq_database::artdaq-database_ConfigurationDB
)

basic_plugin(File "configInterface"
ConfigurationInterface)

install_headers()
install_source()
