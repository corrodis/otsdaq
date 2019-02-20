cet_make(LIBRARY_NAME RootUtilities
	      LIBRARIES
	      ${ROOT_BASIC_LIB_LIST}
	     #zzzzzzzDataDecoders
	      NetworkUtilities
	      TableCore
	      ConfigurationInterface	      
          #otsdaq-core_TablePluginDataFormats_DataBufferConfiguration_table
          #otsdaq-core_TablePluginDataFormats_UDPDataStreamerConsumerConfiguration_table
          #otsdaq-core_TablePluginDataFormats_UDPDataListenerProducerConfiguration_table
          #added by RAR after artdaqdb change
          #otsdaq-core_TablePluginDataFormats_DataManagerConfiguration_table
          #otsdaq-core_TablePluginDataFormats_FEConfiguration_table
          #otsdaq-core_TablePluginDataFormats_DetectorToFEConfiguration_table
          #otsdaq-core_TablePluginDataFormats_DetectorConfiguration_table
)

install_headers()
install_source()
