cet_make(LIBRARY_NAME MessageFacility
  LIBRARIES
  art_Utilities
  MF_MessageLogger
  cetlib
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  fhiclcpp
  )

install_headers()
install_source()
