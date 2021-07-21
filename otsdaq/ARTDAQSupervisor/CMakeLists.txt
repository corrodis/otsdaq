include_directories("/usr/include/python2.7/")

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

cet_make(LIBRARY_NAME ARTDAQSupervisor
		 LIBRARIES
		 
		 fhiclcpp
		 cetlib
		 cetlib_except
		 #${XDAQ_BASIC_LIB_LIST}
  		 ${ROOT_CORE}
		 canvas
		 
		 #${MF_UTILITIES}
		 
		 ${Boost_SYSTEM_LIBRARY}
		 ${Boost_FILESYSTEM_LIBRARY}
		 /usr/lib64/libpython2.7.so.1.0
		 artdaq_Application
		 artdaq_core::artdaq-core_Utilities
		 artdaq_DAQdata
		 artdaq_BuildInfo
		 artdaq_ExternalComms
		 CoreSupervisors
		 
		 #WorkLoopManager
		 #SupervisorInfo
		 #FiniteStateMachine
		 #ConfigurationInterface
		 ARTDAQTableBase
		 #otsdaq_TablePlugins_ARTDAQDataLoggerTable_table
		 #otsdaq_TablePlugins_ARTDAQDispatcherTable_table
		 #otsdaq_TablePlugins_ARTDAQBoardReaderTable_table
		 #otsdaq_TablePlugins_ARTDAQBuilderTable_table
)

install_headers()
install_source()
