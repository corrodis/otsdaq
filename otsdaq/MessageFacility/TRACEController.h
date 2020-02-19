#ifndef OTSDAQ_MESSAGEFACILITY_TRACECONTROLLER_H
#define OTSDAQ_MESSAGEFACILITY_TRACECONTROLLER_H

#include "otsdaq/MessageFacility/ITRACEController.h"

namespace ots
{
class TRACEController : public ITRACEController
{
	TRACEController();
	virtual ~TRACEController() = default;

	virtual HostTraceLevelMap GetTraceLevels();
	virtual void              SetTraceLevelMask(std::string name, TraceMasks const& lvl, std::string hostname = "localhost");
};
}

#endif  // OTSDAQ_MESSAGEFACILITY_TRACECONTROLLER_H