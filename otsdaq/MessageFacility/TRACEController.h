#include "otsdaq/MessageFacility/ITRACEController.h"

namespace ots {
	class TRACEController : public ITRACEController {

		TRACEController();
		virtual ~TRACEController() = default;

		virtual std::unordered_map<std::string /*hostname*/, std::deque<TraceLevel>> GetTraceLevels();
		virtual void SetTraceLevelMask(TraceLevel const& lvl);
	};
}