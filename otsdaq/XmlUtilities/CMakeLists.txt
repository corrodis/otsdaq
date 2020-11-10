include_directories($ENV{XERCES_C_INC})

cet_make(LIBRARY_NAME XmlUtilities
	 LIBRARIES
         ${XDAQ_XERCES-C}
	 MF_MessageLogger
	 artdaq-core_Utilities
	 ${Boost_REGEX_LIBRARY}
	 Macros
        )

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
