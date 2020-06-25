#ifndef OTSDAQ_MESSAGEFACILITY_TRACECONTROLLER_H
#define OTSDAQ_MESSAGEFACILITY_TRACECONTROLLER_H

#include "otsdaq/MessageFacility/ITRACEController.h"

namespace ots
{
class TRACEController : public ITRACEController
{
  public:
	TRACEController() : ITRACEController(){};
	virtual ~TRACEController() = default;

	virtual const HostTraceLevelMap& getTraceLevels();
	virtual void              setTraceLevelMask(std::string const& name, TraceMasks const& lvl, std::string const& hostname = "localhost");

	virtual bool getIsTriggered();
	virtual void setTriggerEnable(size_t entriesAfterTrigger);

	virtual void                   resetTraceBuffer();
	virtual void                   enableTrace();
};
}  // namespace ots

#endif  // OTSDAQ_MESSAGEFACILITY_TRACECONTROLLER_H
