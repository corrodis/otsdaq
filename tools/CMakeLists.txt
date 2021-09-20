
include(CetTest)
cet_enable_asserts()

#cet_make_exec(otsdaq_flatten_active_to_version)
cet_make_exec(otsdaq_flatten_system_aliases LIBRARIES otsdaq::ConfigurationInterface)
cet_make_exec(otsdaq_import_system_aliases LIBRARIES otsdaq::ConfigurationInterface)
cet_make_exec(otsdaq_fix_new_table_fields_tool LIBRARIES otsdaq::ConfigurationInterface)

cet_script(ALWAYS_COPY ots)
cet_script(ALWAYS_COPY common.sh )
cet_script(ALWAYS_COPY ots_remote_start )
cet_script(ALWAYS_COPY otsdaq_convert_config_to_table )
cet_script(ALWAYS_COPY otsdaq_convert_artdaq_daq_tables) 
cet_script(ALWAYS_COPY otsdaq_beautify_artdaq_tables )
cet_script(ALWAYS_COPY otsdaq_add_artdaq_tables)

install_headers()
install_source()
install_fhicl(SUBDIRS fcl)

