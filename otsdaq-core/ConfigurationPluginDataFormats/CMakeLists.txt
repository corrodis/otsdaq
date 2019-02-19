#cet_make(LIBRARY_NAME ConfigurationPluginDataFormat
#		 LIBRARIES
#		 TableCore
#         ${CETLIB}
#)
  
simple_plugin(ARTDAQBoardReaderConfiguration "configuration"
  TableCore 
  ConfigurationInterface
  otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration
  ${CETLIB}
  )
  
simple_plugin(ARTDAQAggregatorConfiguration "configuration"
  TableCore 
  ConfigurationInterface
  otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration
  ${CETLIB}
  )
  
simple_plugin(ARTDAQBuilderConfiguration "configuration"
  TableCore 
  ConfigurationInterface
  otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration
  ${CETLIB}
  )
  
simple_plugin(FESlowControlsConfiguration "configuration"
  TableCore 
  ConfigurationInterface
  ${CETLIB}
  )
  
simple_plugin(XDAQContextConfiguration "configuration"
  TableCore  
  ConfigurationInterface
  ${CETLIB}
  )
  
simple_plugin(DesktopIconConfiguration "configuration"
  TableCore
  ConfigurationInterface
  WebUsersUtilities
  ${CETLIB}
  )
  
simple_plugin(MessageFacilityConfiguration "configuration"
  TableCore
  ConfigurationInterface
  ${CETLIB}
  )
    
simple_plugin(IterateConfiguration "configuration"
  TableCore
  ConfigurationInterface
  ${CETLIB}
  )


  
install_headers()
install_source()
