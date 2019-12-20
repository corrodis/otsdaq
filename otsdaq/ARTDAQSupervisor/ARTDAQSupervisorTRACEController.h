#include "otsdaq/MessageFacility/ITRACEController.h"
#include "otsdaq/ARTDAQSupervisor/ARTDAQSupervisor.hh"

namespace ots {
	class ARTDAQSupervisorTRACEController : public ITRACEController {
	public:
		ARTDAQSupervisorTRACEController();
		virtual ~ARTDAQSupervisorTRACEController() {
			theSupervisor_ = nullptr;
		}

		std::unordered_map<std::string /*hostname*/, std::deque<TraceLevel>> GetTraceLevels() final;
		void SetTraceLevelMask(TraceLevel const& lvl) final;

		void SetSupervisorPtr(ARTDAQSupervisor* ptr) { theSupervisor_ = ptr; }
	private:
		ARTDAQSupervisor* theSupervisor_;
	};
}