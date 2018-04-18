simple_plugin(UDPReceiver "generator"
  artdaq-ots_Overlays
  artdaq_Application
  artdaq_DAQdata
  artdaq-core_Utilities
  artdaq-core_Data
  canvas
  pthread
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  ${MF_MESSAGELOGGER}
  ${Boost_SYSTEM_LIBRARY}
  )

install_headers()
install_source()
