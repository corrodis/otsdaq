include_directories(${MPI_INCLUDE_PATH})

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

cet_make(LIBRARY_NAME DataLoggerApp
		 LIBRARIES
		 
		 ${FHICLCPP}
		 ${CETLIB}
		 ${CETLIB_EXCEPT}
		 #${XDAQ_BASIC_LIB_LIST}
  		 ${ROOT_CORE}
		 canvas
		 
		 #${MF_UTILITIES}
		 
		 ${Boost_SYSTEM_LIBRARY}
		 artdaq_Application
		 artdaq-core_Utilities
		 artdaq_DAQdata
		 artdaq_BuildInfo
		 
		 CoreSupervisors
		 
		 #WorkLoopManager
		 #SupervisorInfo
		 #FiniteStateMachine
		 #ConfigurationInterface
		 #otsdaq-core_TablePluginDataFormats_XDAQContextTable_table
)

install_headers()
install_source()
