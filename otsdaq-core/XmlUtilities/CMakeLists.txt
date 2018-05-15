
cet_make(LIBRARY_NAME XmlUtilities
		   LIBRARIES
           $ENV{XERCES_C_LIB}/libxerces-c.so
		   MF_MessageLogger
		   artdaq-core_Utilities
)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
