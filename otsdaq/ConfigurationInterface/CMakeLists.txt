
cet_make(LIBRARY_NAME ConfigurationInterface
		 LIBRARIES
		 PluginMakers
		 TableCore
		 ProgressBar		 
		 Macros
		 dl
		 
		 ${FHICLCPP}
         ${CETLIB}
         artdaq_database::artdaq-database_ConfigurationDB
         artdaq_database::artdaq-database_DataFormats_Json
         artdaq_database::artdaq-database_SharedCommon
         artdaq_database::artdaq-database_Overlay
         artdaq_database::artdaq-database_JsonDocument
         artdaq_database::artdaq-database_BasicTypes
         artdaq_database::artdaq-database_StorageProviders_FileSystemDB
		 ${Boost_SYSTEM_LIBRARY}		 		 
)


install_headers()
install_source()
