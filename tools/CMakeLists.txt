cet_script(
	#setupOtsEnvironment.sh
	#StartOTS.sh
	colordefs.sh
	ots
	ots_remote_start
	otsdaq_convert_config_to_table
	otsdaq_convert_artdaq_daq_tables
	otsdaq_beautify_artdaq_tables
	otsdaq_add_artdaq_tables
)


find_ups_product(fhiclcpp v3_20_01)
find_ups_product(artdaq_database v1_01_00)
find_ups_product(mongodb v3_4_0)

include(CetTest)
cet_enable_asserts()

link_libraries(
	    ConfigurationInterface
        ${CETLIB}
        ${FHICLCPP}
	    ${BSONCXXLIB}
	    ${MONGOCXXLIB}
    	${artdaq-database_ConfigurationDB}
		$ENV{XDAQ_LIB}/liblog4cplus.so
		$ENV{XDAQ_LIB}/libasyncresolv
		$ENV{XDAQ_LIB}/libxgi.so
		$ENV{XDAQ_LIB}/liblogudpappender.so
		$ENV{XDAQ_LIB}/liblogxmlappender.so
		$ENV{XDAQ_LIB}/libmimetic.so
		$ENV{XDAQ_LIB}/libxdaq.so
		$ENV{XDAQ_LIB}/libconfig.so
		$ENV{XDAQ_LIB}/libpeer.so
		$ENV{XDAQ_LIB}/libtoolbox.so
		$ENV{XDAQ_LIB}/libcgicc.so
		$ENV{XDAQ_LIB}/libxcept.so
		$ENV{XDAQ_LIB}/libxoap.so
		$ENV{XDAQ_LIB}/libxdata.so
		$ENV{XDAQ_LIB}/libb2innub.so
		/usr/lib64/libuuid.so
		${Boost_SYSTEM_LIBRARY}
)

#cet_make_exec(otsdaq_flatten_active_to_version)
cet_make_exec(otsdaq_flatten_system_aliases)
cet_make_exec(otsdaq_import_system_aliases)
cet_make_exec(otsdaq_fix_new_table_fields_tool)


