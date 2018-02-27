include_directories(${MPI_INCLUDE_PATH})

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

cet_make(LIBRARY_NAME EventBuilderApp
		 LIBRARIES
		 ${FHICLCPP}
		 ${CETLIB}
		 ${CETLIB_EXCEPT}
		 ${XDAQ_BASIC_LIB_LIST}
		 canvas
		 MessageFacility
		 ${MF_UTILITIES}
		 ${Boost_SYSTEM_LIBRARY}
		 artdaq_Application
		 artdaq-core_Utilities
		 artdaq_DAQdata
		 artdaq_BuildInfo
		 MessageFacility
		 WorkLoopManager
		 SupervisorInfo
		 FiniteStateMachine
		 ConfigurationInterface
		 otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration
)

install_headers()
install_source()
