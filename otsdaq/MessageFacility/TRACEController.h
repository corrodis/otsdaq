#ifndef OTSDAQ_MESSAGEFACILITY_TRACECONTROLLER_H
#define OTSDAQ_MESSAGEFACILITY_TRACECONTROLLER_H

#include "otsdaq/MessageFacility/ITRACEController.h"

namespace ots
{
class TRACEController : public ITRACEController
{
public:
	TRACEController():ITRACEController() {};
	virtual ~TRACEController() = default;

	virtual const ITRACEController::HostTraceLevelMap& 	getTraceLevels		(void);
	virtual void              							setTraceLevelMask	(const std::string& name, TraceMasks const& lvl, const std::string& hostname = "localhost");
};
}

#endif  // OTSDAQ_MESSAGEFACILITY_TRACECONTROLLER_H
