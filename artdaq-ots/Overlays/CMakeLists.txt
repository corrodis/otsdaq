art_make(LIBRARY_NAME artdaq-ots_Overlays
EXCLUDE OTS_fragmentNameHelper.cc
    LIB_LIBRARIES
    artdaq_core::artdaq-core_Utilities
    artdaq_DAQdata
    cetlib
    cetlib_except
  )

  basic_plugin(OTS "fragmentNameHelper"
  artdaq_core::artdaq-core_Data
  artdaq-ots_Overlays
  artdaq_core::artdaq-core_Data_Artdaq_fragmentNameHelper
  MF_MessageLogger
)
install_headers()
install_source()
