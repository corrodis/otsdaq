include_directories(${MPI_INCLUDE_PATH})
include_directories(${CMAKE_BINARY_DIR}/artdaq)

#get_cmake_property(_variableNames VARIABLES)
#foreach(_variableName ${_variableNames})
#	message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()


simple_plugin(ARTDAQConsumer "processor"
  		${CETLIB}
  		DataManager
  		ARTDAQReaderCore
  )

simple_plugin(ARTDAQProducer "processor"
  		${CETLIB}
  		DataManager
 		ARTDAQReaderCore
  )

simple_plugin(UDPDataListenerProducer "processor"
	DataManager
	NetworkUtilities
	${CETLIB}
  )

simple_plugin(UDPDataStreamerConsumer "processor"
	DataManager
	NetworkUtilities
	${CETLIB}
  )

simple_plugin(TCPDataListenerProducer "processor"
	DataManager
	NetworkUtilities
	${CETLIB}
  )

#simple_plugin(TCPDataStreamerConsumer "processor"
#	DataManager
#	NetworkUtilities
#	${CETLIB}
#  )

simple_plugin(RawDataSaverConsumer "processor"
	DataManager
	${CETLIB}
  )

simple_plugin(RawDataVisualizerConsumer "processor"
    DataManager
    WorkLoopManager
	#MessageFacility
    ConfigurationInterface
    TableCore
	${MF_MESSAGELOGGER}
  )


simple_plugin(OtsDataSaverConsumer "processor"
	DataManager
	${CETLIB}
  )

#simple_plugin(DataDecoderConsumer "processor"
#	DataManager
#	DataDecoders
#	${CETLIB}
#  )


install_headers()
install_source()
