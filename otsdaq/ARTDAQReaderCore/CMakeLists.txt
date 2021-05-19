
cet_make(LIBRARY_NAME ARTDAQReaderCore
		LIBRARIES
		
		artdaq_Application
		artdaq_DAQdata
        artdaq_BuildInfo
        canvas
		${Boost_SYSTEM_LIBRARY}
		${Boost_THREAD_LIBRARY}
		${FHICLCPP}
		${MF_UTILITIES}
		
		Configurable
		
        )
install_headers()
install_source()
