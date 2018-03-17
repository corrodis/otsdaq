#ifndef OTSDAQ_CORE_MESSAGEFACILITY_MESSAGEFACILITY_H
#define OTSDAQ_CORE_MESSAGEFACILITY_MESSAGEFACILITY_H

#include <messagefacility/MessageLogger/MessageLogger.h>
#include "artdaq-core/Utilities/configureMessageFacility.hh"

namespace ots {

inline void INIT_MF(const char* name)
{
	char* logRootString = getenv("OTSDAQ_LOG_ROOT");
	char* logFhiclCode = getenv("OTSDAQ_LOG_FHICL");

	if (logRootString != nullptr)
	{
		setenv("ARTDAQ_LOG_ROOT", logRootString, 1);
	}
	if (logFhiclCode != nullptr)
	{
		char* alf = getenv("ARTDAQ_LOG_FHICL");
		if (alf != nullptr)
		{
			logFhiclCode = strcat(logFhiclCode, alf);
		}
		setenv("ARTDAQ_LOG_FHICL", logFhiclCode, 1);
	}
	artdaq::configureMessageFacility(name,true,true);
	artdaq::setMsgFacAppName(name,0);
}


}

#endif
