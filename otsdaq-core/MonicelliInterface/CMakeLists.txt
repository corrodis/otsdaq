include(CetRootCint)
cet_rootcint(Monicelli)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
cet_make_library(LIBRARY_NAME MonicelliInterface
	        SOURCE ${CMAKE_CURRENT_BINARY_DIR}/MonicelliCint.cc
                       Detector.cxx
		       Event.cxx
		       EventHeader.cxx
		       Geometry.cxx
		       MonicelliEventAnalyzer.cpp
		       MonicelliFileReader.cpp
		       MonicelliGeometryConverter.cpp
		       ROC.cxx
		LIBRARIES
		       ${ROOT_BASIC_LIB_LIST}
		        
                )

install_headers()
install_source()
