cet_make(LIBRARY_NAME MessageFacility
  LIBRARIES
  ${ART_UTILITIES}
  ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
  ${CETLIB}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${FHICLCPP}
  ${TRACE}
  )

install_headers()
install_source()
