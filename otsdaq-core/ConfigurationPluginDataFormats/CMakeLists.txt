#cet_make(LIBRARY_NAME ConfigurationPluginDataFormat
#		 LIBRARIES
#		 ConfigurationDataFormats
#         ${CETLIB}
#)
  
simple_plugin(ARTDAQBoardReaderConfiguration "configuration"
  ConfigurationDataFormats 
  ConfigurationInterface
  otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration
  ${CETLIB}
  )
  
simple_plugin(ARTDAQAggregatorConfiguration "configuration"
  ConfigurationDataFormats 
  ConfigurationInterface
  otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration
  ${CETLIB}
  )
  
simple_plugin(ARTDAQBuilderConfiguration "configuration"
  ConfigurationDataFormats 
  ConfigurationInterface
  otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration
  ${CETLIB}
  )
  
simple_plugin(FESlowControlsConfiguration "configuration"
  ConfigurationDataFormats 
  ConfigurationInterface
  ${CETLIB}
  )
  
simple_plugin(XDAQContextConfiguration "configuration"
  ConfigurationDataFormats  
  ConfigurationInterface
  ${CETLIB}
  )
  
simple_plugin(DesktopIconConfiguration "configuration"
  ConfigurationDataFormats
  ConfigurationInterface
  WebUsersUtilities
  ${CETLIB}
  )
  
simple_plugin(MessageFacilityConfiguration "configuration"
  ConfigurationDataFormats
  ConfigurationInterface
  ${CETLIB}
  )
    
simple_plugin(IterateConfiguration "configuration"
  ConfigurationDataFormats
  ConfigurationInterface
  ${CETLIB}
  )


  
install_headers()
install_source()
