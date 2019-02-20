cet_make(LIBRARY_NAME ConfigurationInterface
		 LIBRARIES
		 PluginMakers
		 TableCore
		 ProgressBar		 
		 Macros
		 dl
		 
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


#		 otsdaq-core_TablePluginDataFormats_ConfigurationAliases_table
#		 otsdaq-core_TablePluginDataFormats_Configurations_table
#		 otsdaq-core_TablePluginDataFormats_DataManagerConfiguration_table
#		 otsdaq-core_TablePluginDataFormats_DefaultConfigurations_table
#		 otsdaq-core_TablePluginDataFormats_DetectorConfiguration_table
#		 otsdaq-core_TablePluginDataFormats_FEConfiguration_table
#		 otsdaq-core_TablePluginDataFormats_MaskConfiguration_table
#		 otsdaq-core_ConfigsurationPluginDataFormats_ModuleToFEConfiguration_table
#		 otsdaq-core_TablePluginDataFormats_DetectorToFEConfiguration_table
#		 otsdaq-core_TablePluginDataFormats_TemplateConfiguration_table
#		 otsdaq-core_TablePluginDataFormats_VersionAliases_table
#		 otsdaq-core_TablePluginDataFormats_XDAQApplicationConfiguration_table
#		 otsdaq-core_TablePluginDataFormats_XDAQContextTable_table
