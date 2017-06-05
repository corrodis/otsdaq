include_directories(${MPI_INCLUDE_PATH})

cet_make(LIBRARY_NAME MPICore
		 LIBRARIES
         artdaq_Application
         artdaq_Application_MPI2
         ${MPI_C_LIBRARIES}
	 MessageFacility
         artdaq_BuildInfo
		 ${CETLIB}
		 ${CETLIB_EXCEPT}
		 MessageFacility
		 ${MF_UTILITIES}
)

install_headers()
install_source()
