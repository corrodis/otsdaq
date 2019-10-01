art_make(LIBRARY_NAME artdaq-ots_Overlays
    LIB_LIBRARIES
    artdaq-core_Utilities
    artdaq_DAQdata
    ${CETLIB}
    ${CETLIB_EXCEPT}
  )
install_headers()
install_source()
