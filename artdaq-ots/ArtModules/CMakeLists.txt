art_make(NO_PLUGINS # WAnt different link lists for modues
 BASENAME_ONLY
 LIBRARY_NAME artdaq-ots_ArtModules
 LIB_LIBRARIES artdaq-ots_Overlays
)

simple_plugin(ArtdaqOtsBuildInfo "module"
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq_core::artdaq-core_BuildInfo
  artdaq-ots_BuildInfo
  fhiclcpp
  cetlib
  cetlib_except
)

simple_plugin(UDPDump "module"
  artdaq_core::artdaq-core_Utilities
  fhiclcpp
  cetlib
  cetlib_except
  MF_MessageLogger
)

simple_plugin(JSONDispatcher "module"
  artdaq_core::artdaq-core_Utilities
  ${Boost_SYSTEM_LIBRARY}
  cetlib_except
  pthread
  MF_MessageLogger
)

install_headers()
install_source()
