
cet_make_library(LIBRARY_NAME SlowControlsTableBase
SOURCE
SlowControlsTableBase.cc
		LIBRARIES
  		otsdaq::ConfigurationInterface
)

install_headers()
install_source()
