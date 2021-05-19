
#get_cmake_property(_variableNames VARIABLES)
#foreach(_variableName ${_variableNames})
#	message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

cet_make(LIBRARY_NAME CoreSupervisors
		 LIBRARIES
		 ARTDAQDataManager
         ARTDAQTableBase
		 DataManager
		 FECore
         WorkLoopManager
         FiniteStateMachine
         CgiDataUtilities
         WebUsersUtilities
         MessageFacility
         SupervisorInfo
         SOAPUtilities
         XmlUtilities
         CgiDataUtilities         
         ${XDAQ_BASIC_LIB_LIST}
         config # From XDAQ
		 otsdaq_TablePlugins_XDAQContextTable_table
		 otsdaq_TablePlugins_DesktopIconTable_table
        )

install_headers()
install_source()
