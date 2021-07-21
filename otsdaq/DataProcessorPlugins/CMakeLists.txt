
#get_cmake_property(_variableNames VARIABLES)
#foreach(_variableName ${_variableNames})
#	message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()


simple_plugin(ARTDAQConsumer "processor"
  		cetlib
  		DataManager
  		ARTDAQReaderCore
  )

simple_plugin(ARTDAQProducer "processor"
  		cetlib
  		DataManager
 		ARTDAQReaderCore
  )

simple_plugin(UDPDataListenerProducer "processor"
	DataManager
	NetworkUtilities
	cetlib
  )

simple_plugin(UDPDataStreamerConsumer "processor"
	DataManager
	NetworkUtilities
	cetlib
  )
  
simple_plugin(TCPDataReceiverProducer "processor"
	DataManager
	NetworkUtilities
	cetlib
  )

simple_plugin(TCPDataListenerProducer "processor"
	DataManager
	NetworkUtilities
	cetlib
  )

#simple_plugin(TCPDataStreamerConsumer "processor"
#	DataManager
#	NetworkUtilities
#	cetlib
#  )

simple_plugin(RawDataSaverConsumer "processor"
	DataManager
	cetlib
  )

simple_plugin(RawDataVisualizerConsumer "processor"
    DataManager
    WorkLoopManager
	#MessageFacility
    ConfigurationInterface
    TableCore
	MF_MessageLogger
  )


simple_plugin(OtsDataSaverConsumer "processor"
	DataManager
	cetlib
  )

#simple_plugin(DataDecoderConsumer "processor"
#	DataManager
#	DataDecoders
#	cetlib
#  )


install_headers()
install_source()
