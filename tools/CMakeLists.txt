cet_script(
	#setupOtsEnvironment.sh
	#StartOTS.sh
	common.sh
	ots
	ots_remote_start
	otsdaq_convert_config_to_table
	otsdaq_convert_artdaq_daq_tables
	otsdaq_beautify_artdaq_tables
	otsdaq_add_artdaq_tables
)

include(CetTest)
cet_enable_asserts()

#cet_make_exec(otsdaq_flatten_active_to_version)
cet_make_exec(otsdaq_flatten_system_aliases LIBRARIES otsdaq::ConfigurationInterface)
cet_make_exec(otsdaq_import_system_aliases LIBRARIES otsdaq::ConfigurationInterface)
cet_make_exec(otsdaq_fix_new_table_fields_tool LIBRARIES otsdaq::ConfigurationInterface)


