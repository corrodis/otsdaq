cet_make(LIBRARY_NAME NetworkUtilities 
        LIBRARIES   
        ${Boost_REGEX_LIBRARY}
        MF_MessageLogger
        pthread
)

install_headers()
install_source()
