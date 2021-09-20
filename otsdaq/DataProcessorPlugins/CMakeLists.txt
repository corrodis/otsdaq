
#get_cmake_property(_variableNames VARIABLES)
#foreach(_variableName ${_variableNames})
#	message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()


basic_plugin(ARTDAQConsumer "processor"
  		cetlib
  		DataManager
  		ARTDAQReaderCore
  )

basic_plugin(ARTDAQProducer "processor"
  		cetlib
  		DataManager
 		ARTDAQReaderCore
  )

basic_plugin(UDPDataListenerProducer "processor"
	DataManager
	NetworkUtilities
	cetlib
  )

basic_plugin(UDPDataStreamerConsumer "processor"
	DataManager
	NetworkUtilities
	cetlib
  )
  
basic_plugin(TCPDataReceiverProducer "processor"
	DataManager
	NetworkUtilities
	cetlib
  )

basic_plugin(TCPDataListenerProducer "processor"
	DataManager
	NetworkUtilities
	cetlib
  )

#basic_plugin(TCPDataStreamerConsumer "processor"
#	DataManager
#	NetworkUtilities
#	cetlib
#  )

basic_plugin(RawDataSaverConsumer "processor"
	DataManager
	cetlib
  )

basic_plugin(RawDataVisualizerConsumer "processor"
    DataManager
    WorkLoopManager
	#MessageFacility
    ConfigurationInterface
    TableCore
	MF_MessageLogger
  )


basic_plugin(OtsDataSaverConsumer "processor"
	DataManager
	cetlib
  )

#basic_plugin(DataDecoderConsumer "processor"
#	DataManager
#	DataDecoders
#	cetlib
#  )


install_headers()
install_source()
