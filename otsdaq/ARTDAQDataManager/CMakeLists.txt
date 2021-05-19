
cet_make(LIBRARY_NAME ARTDAQDataManager 
		LIBRARIES
		
		DataManager
        otsdaq_DataProcessorPlugins_ARTDAQConsumer_processor
        otsdaq_DataProcessorPlugins_ARTDAQProducer_processor
        
        )
install_headers()
install_source()
