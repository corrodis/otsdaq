
cet_make(LIBRARY_NAME XmlUtilities
		   LIBRARIES
           ${XDAQ_XERCES-C}
		   MF_MessageLogger
		   artdaq-core_Utilities
)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
