#cet_make(LIBRARY_NAME TablePlugins
#		 LIBRARIES
#		 TableCore
#         ${CETLIB}
#)

add_subdirectory(ARTDAQTableBase)
add_subdirectory(SlowControlsTableBase)
    
  
simple_plugin(ARTDAQBoardReaderTable "table"
  ARTDAQTableBase
  )
  
simple_plugin(ARTDAQDataLoggerTable "table"
  ARTDAQTableBase
  )
  
simple_plugin(ARTDAQDispatcherTable "table"
  ARTDAQTableBase
  )
  
simple_plugin(ARTDAQEventBuilderTable "table"
  ARTDAQTableBase
  )
  
simple_plugin(ARTDAQRoutingMasterTable "table"
  ARTDAQTableBase
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
