cet_make(LIBRARY_NAME RootUtilities
	      LIBRARIES
	      ${ROOT_BASIC_LIB_LIST}
	     #zzzzzzzDataDecoders
	      NetworkUtilities
	      TableCore
	      ConfigurationInterface	      
          #otsdaq-core_TablePlugins_DataBufferConfiguration_table
          #otsdaq-core_TablePlugins_UDPDataStreamerConsumerConfiguration_table
          #otsdaq-core_TablePlugins_UDPDataListenerProducerConfiguration_table
          #added by RAR after artdaqdb change
          #otsdaq-core_TablePlugins_DataManagerConfiguration_table
          #otsdaq-core_TablePlugins_FEConfiguration_table
          #otsdaq-core_TablePlugins_DetectorToFEConfiguration_table
          #otsdaq-core_TablePlugins_DetectorConfiguration_table
)

install_headers()
install_source()
