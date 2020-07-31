art_make(NO_PLUGINS # WAnt different link lists for modues
 BASENAME_ONLY
 LIBRARY_NAME artdaq-ots_ArtModules
 LIB_LIBRARIES artdaq-ots_Overlays
)

simple_plugin(ArtdaqOtsBuildInfo "module"
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq-core_BuildInfo
  artdaq-ots_BuildInfo
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
)

simple_plugin(UDPDump "module"
  artdaq-core_Utilities
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  MF_MessageLogger
)

simple_plugin(JSONDispatcher "module"
  artdaq-core_Utilities
  ${Boost_SYSTEM_LIBRARY}
  ${CETLIB_EXCEPT}
  pthread
  MF_MessageLogger
)

install_headers()
install_source()
