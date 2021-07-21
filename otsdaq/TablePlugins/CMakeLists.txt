#cet_make(LIBRARY_NAME TablePlugins
#		 LIBRARIES
#		 TableCore
#         cetlib
#)

add_subdirectory(ARTDAQTableBase)
add_subdirectory(SlowControlsTableBase)
    
  
simple_plugin(ARTDAQBoardReaderTable "table"
  ARTDAQTableBase
  SlowControlsTableBase
  )
  
simple_plugin(ARTDAQDataLoggerTable "table"
  ARTDAQTableBase
  SlowControlsTableBase
  )
  
simple_plugin(ARTDAQDispatcherTable "table"
  ARTDAQTableBase
  SlowControlsTableBase
  )
  
simple_plugin(ARTDAQEventBuilderTable "table"
  ARTDAQTableBase
  SlowControlsTableBase
  )
  
simple_plugin(ARTDAQRoutingMasterTable "table"
  ARTDAQTableBase
  SlowControlsTableBase
  )
  
simple_plugin(FESlowControlsTable "table"
  TableCore 
  ConfigurationInterface
  cetlib
  )
  
simple_plugin(XDAQContextTable "table"
  TableCore  
  ConfigurationInterface
  cetlib
  )
  
simple_plugin(DesktopIconTable "table"
  TableCore
  ConfigurationInterface
  WebUsersUtilities
  cetlib
  )
  
simple_plugin(MessageFacilityTable "table"
  TableCore
  ConfigurationInterface
  cetlib
  )
    
simple_plugin(IterateTable "table"
  TableCore
  ConfigurationInterface
  cetlib
  )


  
install_headers()
install_source()
