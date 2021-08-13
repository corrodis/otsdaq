include_directories("/usr/include/python2.7/")

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

cet_make(LIBRARY_NAME ARTDAQSupervisor
		 LIBRARIES
		 
		 #fhiclcpp
		 #cetlib
		 #cetlib_except
  		 #${ROOT_CORE}
		 #canvas
		 
		 
		 #${Boost_SYSTEM_LIBRARY}
		 #${Boost_FILESYSTEM_LIBRARY}
		 /usr/lib64/libpython2.7.so.1.0
		 #artdaq_Application
		 #artdaq_core::artdaq-core_Utilities
		 #artdaq_DAQdata
		 #artdaq_BuildInfo
		 artdaq_ExternalComms
		 CoreSupervisors
		 
		 ARTDAQTableBase
)

install_headers()
install_source()
