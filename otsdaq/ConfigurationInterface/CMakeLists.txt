cet_make(LIBRARY_NAME ConfigurationInterface
		 LIBRARIES
		 PluginMakers
		 TableCore
		 ProgressBar		 
		 Macros
		 dl
		 
		 ${FHICLCPP}
         ${CETLIB}
         artdaq-database_ConfigurationDB
         artdaq-database_DataFormats_Json
         artdaq-database_SharedCommon
         artdaq-database_Overlay
         artdaq-database_JsonDocument
         artdaq-database_BasicTypes
         artdaq-database_StorageProviders_FileSystemDB
		 ${Boost_SYSTEM_LIBRARY}
)


install_headers()
install_source()
