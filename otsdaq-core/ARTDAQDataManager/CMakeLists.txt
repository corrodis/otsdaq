include_directories(${MPI_INCLUDE_PATH})
include_directories(${CMAKE_BINARY_DIR}/artdaq)

cet_make(LIBRARY_NAME ARTDAQDataManager 
		LIBRARIES
		DataManager
		MPICore
        otsdaq-core_DataProcessorPlugins_ARTDAQConsumer_processor
        otsdaq-core_DataProcessorPlugins_ARTDAQProducer_processor
        )
install_headers()
install_source()
