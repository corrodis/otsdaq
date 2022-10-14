include_directories("/usr/include/python2.7/")

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

cet_make_library(LIBRARY_NAME ARTDAQSupervisor
SOURCE
ARTDAQSupervisor.cc
ARTDAQSupervisorTRACEController.cc
version.cc
		 LIBRARIES PRIVATE
		 /usr/lib64/libpython2.7.so.1.0
		 artdaq_plugin_support::commanderMaker
		 otsdaq::CoreSupervisors
		 otsdaq::ARTDAQTableBase
)

install_headers()
install_source()
