cet_make(LIBRARY_NAME DataManager
		LIBRARIES
		WorkLoopManager
		ConfigurationInterface
		PluginMakers
		Configurable
        ${Boost_SYSTEM_LIBRARY}
        )
install_headers()
install_source()
