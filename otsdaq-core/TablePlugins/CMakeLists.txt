#cet_make(LIBRARY_NAME TablePlugins
#		 LIBRARIES
#		 TableCore
#         ${CETLIB}
#)

add_subdirectory(ARTDAQTableBase)
    
  
simple_plugin(ARTDAQBoardReaderTable "table"
  TableCore 
  ConfigurationInterface
  otsdaq-core_TablePlugins_XDAQContextTable_table
  ${CETLIB}
  )
  
simple_plugin(ARTDAQDataLoggerTable "table"
  TableCore 
  ConfigurationInterface
  otsdaq-core_TablePlugins_XDAQContextTable_table
  ${CETLIB}
  )
  simple_plugin(ARTDAQDispatcherTable "table"
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
