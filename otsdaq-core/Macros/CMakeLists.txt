cet_make(LIBRARY_NAME Macros
		 LIBRARIES
		 MF_MessageLogger
		 #${ROOT_CORE} #added for ROOT's stackTrace() in StringMacros:: and then removed when it caused ugly things
        )
        
install_headers()
install_source()
