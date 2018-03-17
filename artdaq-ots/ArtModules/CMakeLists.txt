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
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
)

simple_plugin(JSONDispatcher "module"
  ${Boost_SYSTEM_LIBRARY}
  ${CETLIB_EXCEPT}
)

install_headers()
install_source()
