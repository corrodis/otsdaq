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


simple_plugin(OtsInput "source"
  artdaq-ots_Overlays
  artdaq-core_Core
  artdaq-core_Utilities
  rt
  artdaq_DAQdata
  artdaq-utilities_Plugins
  artdaq_ArtModules
  artdaq_ArtModules_ArtdaqSharedMemoryService_service
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_Principal
  art_Framework_Services_Registry
  art_Persistency_Provenance
  art_Utilities
  MF_MessageLogger
  ${CETLIB_EXCEPT}
)


install_headers()
install_source()
