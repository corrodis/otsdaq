#ifndef OTSDAQ_CORE_MESSAGEFACILITY_MESSAGEFACILITY_H
#define OTSDAQ_CORE_MESSAGEFACILITY_MESSAGEFACILITY_H

#include "otsdaq/Macros/CoutMacros.h"

#include <messagefacility/MessageLogger/MessageLogger.h>
#include "artdaq-core/Utilities/configureMessageFacility.hh"

namespace ots
{
static bool MESSAGE_FACILITY_INITIALIZED;

inline void INIT_MF(const char* name)
{
	if(MESSAGE_FACILITY_INITIALIZED)
		return;

	char* logRootString = getenv("OTSDAQ_LOG_ROOT");
	if(logRootString == nullptr)
	{
		__COUT_ERR__ << "\n**********************************************************"
		             << std::endl;
		__COUT_ERR__ << "WARNING: OTSDAQ_LOG_ROOT environment variable was not set!"
		             << std::endl;
		__COUT_ERR__ << "**********************************************************\n"
		             << std::endl;
		// exit(0);
	}
	else
		setenv("ARTDAQ_LOG_ROOT", logRootString, 1);

	char* logFhiclCode = getenv("OTSDAQ_LOG_FHICL");
	if(logFhiclCode == nullptr)
	{
		__COUT_ERR__ << "\n***********************************************************"
		             << std::endl;
		__COUT_ERR__ << "WARNING: OTSDAQ_LOG_FHICL environment variable was not set!"
		             << std::endl;
		__COUT_ERR__ << "***********************************************************\n"
		             << std::endl;
		// exit(0);
	}
	else
	{
		setenv("ARTDAQ_LOG_FHICL", logFhiclCode, 1);
		setenv("DAQINTERFACE_MESSAGEFACILITY_FHICL", logFhiclCode, 1);
	}

	__COUT__ << "Configuring message facility with " << logFhiclCode << __E__;
	{
		FILE* fp = fopen(logFhiclCode, "r");
		if(fp)
		{
			char line[100];
			while(fgets(line, 100, fp))
				std::cout << line;
			std::cout << __E__;

			fclose(fp);
		}
	}
	artdaq::configureMessageFacility(name /*application name*/,
	                                 false /*cout display*/,
	                                 true /*enable debug messages*/);

	artdaq::setMsgFacAppName(name, 0);

	MESSAGE_FACILITY_INITIALIZED = true;

}  // end INIT_MF()

}  // namespace ots

#endif
