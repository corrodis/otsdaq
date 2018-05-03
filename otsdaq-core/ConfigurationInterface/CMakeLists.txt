cet_make(LIBRARY_NAME ConfigurationInterface
		 LIBRARIES
		 PluginMakers
		 ConfigurationDataFormats
		 ProgressBar		 
		 Macros

         ${CETLIB}
         artdaq-database_ConfigurationDB
         artdaq-database_DataFormats_Json
         artdaq-database_SharedCommon
         artdaq-database_Overlay
         artdaq-database_JsonDocument
         artdaq-database_BasicTypes
         artdaq-database_StorageProviders_FileSystemDB
)


install_headers()
install_source()


#		 otsdaq-core_ConfigurationPluginDataFormats_ConfigurationAliases_configuration
#		 otsdaq-core_ConfigurationPluginDataFormats_Configurations_configuration
#		 otsdaq-core_ConfigurationPluginDataFormats_DataManagerConfiguration_configuration
#		 otsdaq-core_ConfigurationPluginDataFormats_DefaultConfigurations_configuration
#		 otsdaq-core_ConfigurationPluginDataFormats_DetectorConfiguration_configuration
#		 otsdaq-core_ConfigurationPluginDataFormats_FEConfiguration_configuration
#		 otsdaq-core_ConfigurationPluginDataFormats_MaskConfiguration_configuration
#		 otsdaq-core_ConfigsurationPluginDataFormats_ModuleToFEConfiguration_configuration
#		 otsdaq-core_ConfigurationPluginDataFormats_DetectorToFEConfiguration_configuration
#		 otsdaq-core_ConfigurationPluginDataFormats_TemplateConfiguration_configuration
#		 otsdaq-core_ConfigurationPluginDataFormats_VersionAliases_configuration
#		 otsdaq-core_ConfigurationPluginDataFormats_XDAQApplicationConfiguration_configuration
#		 otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration