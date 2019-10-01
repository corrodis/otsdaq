include_directories(${MPI_INCLUDE_PATH})

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

cet_make(LIBRARY_NAME ARTDAQSupervisor
		 LIBRARIES
		 
		 ${FHICLCPP}
		 ${CETLIB}
		 ${CETLIB_EXCEPT}
		 #${XDAQ_BASIC_LIB_LIST}
  		 ${ROOT_CORE}
		 canvas
		 
		 #${MF_UTILITIES}
		 
		 ${Boost_SYSTEM_LIBRARY}
		 ${Boost_FILESYSTEM_LIBRARY}
		 $ENV{PYTHON_LIB}/libpython2.7.so
		 artdaq_Application
		 artdaq-core_Utilities
		 artdaq_DAQdata
		 artdaq_BuildInfo
		 
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
