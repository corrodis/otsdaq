include_directories(${MPI_INCLUDE_PATH})
include_directories(${CMAKE_BINARY_DIR}/artdaq)

cet_make(LIBRARY_NAME ARTDAQDataManager 
		LIBRARIES
		
		DataManager
        otsdaq_DataProcessorPlugins_ARTDAQConsumer_processor
        otsdaq_DataProcessorPlugins_ARTDAQProducer_processor
        
        )
install_headers()
install_source()
