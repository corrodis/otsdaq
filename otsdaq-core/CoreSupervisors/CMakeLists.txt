include_directories(${MPI_INCLUDE_PATH})
include_directories(${CMAKE_BINARY_DIR}/artdaq)

#get_cmake_property(_variableNames VARIABLES)
#foreach(_variableName ${_variableNames})
#	message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

cet_make(LIBRARY_NAME CoreSupervisors
		 LIBRARIES
		 ARTDAQDataManager
		 DataManager
		 FECore
		 MPICore
         WorkLoopManager
         FiniteStateMachine
         CgiDataUtilities
         ${XDAQ_BASIC_LIB_LIST}
		 otsdaq-core_ConfigurationPluginDataFormats_XDAQContextConfiguration_configuration
        )

install_headers()
install_source()
