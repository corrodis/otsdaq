#cet_make(LIBRARY_NAME TablePlugins
#		 LIBRARIES
#		 TableCore
#         ${CETLIB}
#)
  
simple_plugin(ARTDAQBoardReaderTable "table"
  TableCore 
  ConfigurationInterface
  otsdaq-core_TablePlugins_XDAQContextTable_table
  ${CETLIB}
  )
  
simple_plugin(ARTDAQAggregatorTable "table"
  TableCore 
  ConfigurationInterface
  otsdaq-core_TablePlugins_XDAQContextTable_table
  ${CETLIB}
  )
  
simple_plugin(ARTDAQBuilderTable "table"
  TableCore 
  ConfigurationInterface
  otsdaq-core_TablePlugins_XDAQContextTable_table
  ${CETLIB}
  )
  
simple_plugin(FESlowControlsTable "table"
  TableCore 
  ConfigurationInterface
  ${CETLIB}
  )
  
simple_plugin(XDAQContextTable "table"
  TableCore  
  ConfigurationInterface
  ${CETLIB}
  )
  
simple_plugin(DesktopIconTable "table"
  TableCore
  ConfigurationInterface
  WebUsersUtilities
  ${CETLIB}
  )
  
simple_plugin(MessageFacilityTable "table"
  TableCore
  ConfigurationInterface
  ${CETLIB}
  )
    
simple_plugin(IterateTable "table"
  TableCore
  ConfigurationInterface
  ${CETLIB}
  )


  
install_headers()
install_source()
