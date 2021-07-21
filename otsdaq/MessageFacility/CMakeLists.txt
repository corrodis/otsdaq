cet_make(LIBRARY_NAME MessageFacility
  LIBRARIES
  ${ART_UTILITIES}
  MF_MessageLogger
  ${MF_UTILITIES}
  cetlib
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  fhiclcpp
  ${TRACE}
  )

install_headers()
install_source()
