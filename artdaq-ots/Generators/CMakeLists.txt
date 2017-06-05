simple_plugin(UDPReceiver "generator"
  artdaq-ots_Overlays
  artdaq_Application
  artdaq_DAQdata
  artdaq-core_Utilities
  artdaq-core_Data
  canvas
  MessageFacility
  pthread
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  )

install_headers()
install_source()
