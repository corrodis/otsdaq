cet_make(LIBRARY_NAME RootUtilities
	      LIBRARIES
	      ${ROOT_BASIC_LIB_LIST}
	      DataDecoders
	      NetworkUtilities
	      ConfigurationDataFormats
	      ConfigurationInterface	      
          #otsdaq-core_ConfigurationPluginDataFormats_DataBufferConfiguration_configuration
          #otsdaq-core_ConfigurationPluginDataFormats_UDPDataStreamerConsumerConfiguration_configuration
          #otsdaq-core_ConfigurationPluginDataFormats_UDPDataListenerProducerConfiguration_configuration
          #added by RAR after artdaqdb change
          #otsdaq-core_ConfigurationPluginDataFormats_DataManagerConfiguration_configuration
          #otsdaq-core_ConfigurationPluginDataFormats_FEConfiguration_configuration
          #otsdaq-core_ConfigurationPluginDataFormats_DetectorToFEConfiguration_configuration
          #otsdaq-core_ConfigurationPluginDataFormats_DetectorConfiguration_configuration
)

install_headers()
install_source()
