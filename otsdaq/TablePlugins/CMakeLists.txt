#cet_make(LIBRARY_NAME TablePlugins
#		 LIBRARIES
#		 TableCore
#         cetlib
#)

add_subdirectory(ARTDAQTableBase)
add_subdirectory(SlowControlsTableBase)
    
  
basic_plugin(ARTDAQBoardReaderTable "table"
  ARTDAQTableBase
  SlowControlsTableBase
  )
  
basic_plugin(ARTDAQDataLoggerTable "table"
  ARTDAQTableBase
  SlowControlsTableBase
  )
  
basic_plugin(ARTDAQDispatcherTable "table"
  ARTDAQTableBase
  SlowControlsTableBase
  )
  
basic_plugin(ARTDAQEventBuilderTable "table"
  ARTDAQTableBase
  SlowControlsTableBase
  )
  
basic_plugin(ARTDAQRoutingManagerTable "table"
  ARTDAQTableBase
  SlowControlsTableBase
  )
  
basic_plugin(FESlowControlsTable "table"
  TableCore 
  ConfigurationInterface
  cetlib
  )
  
basic_plugin(XDAQContextTable "table"
  TableCore  
  ConfigurationInterface
  cetlib
  )
  
basic_plugin(DesktopIconTable "table"
  TableCore
  ConfigurationInterface
  WebUsersUtilities
  cetlib
  )
  
basic_plugin(MessageFacilityTable "table"
  TableCore
  ConfigurationInterface
  cetlib
  )
    
basic_plugin(IterateTable "table"
  TableCore
  ConfigurationInterface
  cetlib
  )


  
install_headers()
install_source()
