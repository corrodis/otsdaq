cet_make(LIBRARY_NAME FiniteStateMachine
		LIBRARIES
		${XDAQ_BASIC_LIB_LIST}
		SOAPUtilities
		ProgressBar
		artdaq_core::artdaq-core_Utilities
		Macros
                )

install_headers()
install_source()
