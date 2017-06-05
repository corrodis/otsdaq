#ifndef OTSDAQ_CORE_MESSAGEFACILITY_MESSAGEFACILITY_H
#define OTSDAQ_CORE_MESSAGEFACILITY_MESSAGEFACILITY_H

#include <messagefacility/MessageLogger/MessageLogger.h>
#include "otsdaq-core/MessageFacility/configureMessageFacility.hh"

namespace ots {

inline void INIT_MF(const char* name)
{
	configureMessageFacility(name);
	setMsgFacAppName(name);
}


}

#endif
