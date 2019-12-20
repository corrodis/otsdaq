#include <deque>
#include <unordered_map>
#include <string>

namespace ots {
    class ITRACEController {
    public:
        struct TraceLevel {
            std::string name;
            uint64_t lvlM;
            uint64_t lvlS;
            uint64_t lvlT;
        };

        ITRACEController() {}
        virtual ~ITRACEController() = default;

        virtual std::unordered_map<std::string /*hostname*/, std::deque<TraceLevel>> GetTraceLevels() = 0;
        virtual void SetTraceLevelMask(TraceLevel const& lvl) = 0;
    };

    class NullTRACEController : public ITRACEController {
    public:
        NullTRACEController() {}
        virtual ~NullTRACEController() = default;

        std::unordered_map<std::string, std::deque<TraceLevel>> GetTraceLevels() final { return std::unordered_map<std::string, std::deque<TraceLevel>>(); }
        void SetTraceLevelMask(TraceLevel const&) final {}
    };

}