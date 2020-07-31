art_make(LIBRARY_NAME artdaq-ots_Overlays
    LIB_LIBRARIES
    artdaq-core_Utilities
    artdaq_DAQdata
    ${CETLIB}
    ${CETLIB_EXCEPT}
  )
  basic_plugin(OTS "fragmentNameHelper"
  artdaq-core_Data
  artdaq-core-demo_Overlays
  artdaq-core_Data_Artdaq_fragmentNameHelper
  MF_MessageLogger
)
install_headers()
install_source()
