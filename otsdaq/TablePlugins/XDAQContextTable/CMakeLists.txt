cet_make_library(LIBRARY_NAME XDAQContextTable SOURCE XDAQContextTable.cc
LIBRARIES
PUBLIC
otsdaq::ConfigurationInterface
PRIVATE

)

install_headers()
install_source()
