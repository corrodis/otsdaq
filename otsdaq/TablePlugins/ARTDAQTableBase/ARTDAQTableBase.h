#ifndef _ots_ARTDAQTableBase_h_
#define _ots_ARTDAQTableBase_h_

#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/ConfigurationInterface/ConfigurationManagerRW.h"
#include "otsdaq/TableCore/TableBase.h"

// helpers
#define OUT out << tabStr << commentStr
#define PUSHTAB tabStr += "\t"
#define POPTAB tabStr.resize(tabStr.size() - 1)
#define PUSHCOMMENT commentStr += "# "
#define POPCOMMENT commentStr.resize(commentStr.size() - 2)

namespace ots
{
class ARTDAQTableBase : public TableBase
{
  public:
	ARTDAQTableBase();
	ARTDAQTableBase(std::string tableName, std::string* accumulatedExceptions = 0);

	virtual ~ARTDAQTableBase(void);

	static const std::string ARTDAQ_FCL_PATH;
	static const std::string ARTDAQ_SUPERVISOR_TABLE;
	static const std::string ARTDAQ_READER_TABLE, ARTDAQ_BUILDER_TABLE, ARTDAQ_LOGGER_TABLE, ARTDAQ_DISPATCHER_TABLE, ARTDAQ_MONITOR_TABLE,
	    ARTDAQ_ROUTING_MASTER_TABLE;

	// ARTDAQ Supervisor Column names
	static struct ColARTDAQSupervisor
	{
		std::string const colDAQInterfaceDebugLevel_ = "DAQInterfaceDebugLevel";
		std::string const colDAQSetupScript_         = "DAQSetupScript";

		std::string const colLinkToBoardReaders_          = "boardreadersLink";
		std::string const colLinkToBoardReadersGroupID_   = "boardreadersLinkGroupID";
		std::string const colLinkToEventBuilders_         = "eventbuildersLink";
		std::string const colLinkToEventBuildersGroupID_  = "eventbuildersLinkGroupID";
		std::string const colLinkToDataLoggers_           = "dataloggersLink";
		std::string const colLinkToDataLoggersGroupID_    = "dataloggersLinkGroupID";
		std::string const colLinkToDispatchers_           = "dispatchersLink";
		std::string const colLinkToDispatchersGroupID_    = "dispatchersLinkGroupID";
		std::string const colLinkToRoutingMasters_        = "routingMastersLink";
		std::string const colLinkToRoutingMastersGroupID_ = "routingMastersLinkGroupID";
	} colARTDAQSupervisor_;

	enum class ARTDAQAppType
	{
		BoardReader,
		EventBuilder,
		DataLogger,
		Dispatcher,
		Monitor,
		RoutingMaster
	};

	static struct ProcessTypes
	{
		std::string const READER        = "reader";
		std::string const BUILDER       = "builder";
		std::string const LOGGER        = "logger";
		std::string const DISPATCHER    = "dispatcher";
		std::string const MONITOR       = "monitor";
		std::string const ROUTINGMASTER = "routingMaster";
	} processTypes_;

	enum
	{
		DEFAULT_MAX_FRAGMENT_SIZE   = 1048576,
		DEFAULT_ROUTING_TIMEOUT_MS  = 1000,
		DEFAULT_ROUTING_RETRY_COUNT = 5,
	};

	static const int NULL_SUBSYSTEM_DESTINATION;

	struct ProcessInfo
	{
		std::string label;
		std::string hostname;
		int         subsystem;

		ProcessInfo(std::string l, std::string h, int s, ARTDAQAppType t) : label(l), hostname(h), subsystem(s) {}
	};

	struct SubsystemInfo
	{
		int         id;
		std::string label;

		std::set<int> sources;      // by subsystem id
		int           destination;  // destination subsystem id, 0 := no destination

		bool hasRoutingMaster;

		SubsystemInfo() : sources(), destination(0), hasRoutingMaster(false) {}
	};

	struct ArtdaqInfo
	{
		std::map<int /*subsystem ID*/, ARTDAQTableBase::SubsystemInfo> subsystems;
		std::map<ARTDAQAppType, std::list<ProcessInfo>>                processes;
	};

	static const std::string& getTypeString(ARTDAQAppType type);
	static std::string        getFHICLFilename(ARTDAQAppType type, const std::string& name);
	static std::string        getFlatFHICLFilename(ARTDAQAppType type, const std::string& name);
	static void               flattenFHICL(ARTDAQAppType type, const std::string& name);

	static void        insertParameters(std::ostream&      out,
	                                    std::string&       tabStr,
	                                    std::string&       commentStr,
	                                    ConfigurationTree  parameterLink,
	                                    const std::string& parameterPreamble,
	                                    bool               onlyInsertAtTableParameters = false,
	                                    bool               includeAtTableParameters    = false);
	static std::string insertModuleType(std::ostream& out, std::string& tabStr, std::string& commentStr, ConfigurationTree moduleTypeNode);
	static void        insertMetricsBlock(std::ostream& out, std::string& tabStr, std::string& commentStr, ConfigurationTree daqNode);

	static void outputBoardReaderFHICL(const ConfigurationTree& boardReaderNode,
	                                   size_t                   maxFragmentSizeBytes = DEFAULT_MAX_FRAGMENT_SIZE,
	                                   size_t                   routingTimeoutMs     = DEFAULT_ROUTING_TIMEOUT_MS,
	                                   size_t                   routingRetryCount    = DEFAULT_ROUTING_RETRY_COUNT);

	static void outputDataReceiverFHICL(const ConfigurationTree& receiverNode,
	                                    ARTDAQAppType            appType,
	                                    size_t                   maxFragmentSizeBytes = DEFAULT_MAX_FRAGMENT_SIZE,
	                                    size_t                   routingTimeoutMs     = DEFAULT_ROUTING_TIMEOUT_MS,
	                                    size_t                   routingRetryCount    = DEFAULT_ROUTING_RETRY_COUNT);

	static void outputRoutingMasterFHICL(const ConfigurationTree& routingMasterNode,
	                                     size_t                   routingTimeoutMs  = DEFAULT_ROUTING_TIMEOUT_MS,
	                                     size_t                   routingRetryCount = DEFAULT_ROUTING_RETRY_COUNT);

	static ArtdaqInfo extractArtdaqInfo(ConfigurationTree artdaqSupervisorNode,
	                                    bool              doWriteFHiCL         = false,
	                                    size_t            maxFragmentSizeBytes = DEFAULT_MAX_FRAGMENT_SIZE,
	                                    size_t            routingTimeoutMs     = DEFAULT_ROUTING_TIMEOUT_MS,
	                                    size_t            routingRetryCount    = DEFAULT_ROUTING_RETRY_COUNT,
	                                    ProgressBar*      progressBar          = 0);
	static void       setAndActivateArtdaqSystem(
	          ConfigurationManagerRW*                                                                                        cfgMgr,
	          const std::map<std::string /*type*/, std::map<std::string /*record*/, std::vector<std::string /*property*/>>>& nodeTypeToObjectMap,
	          const std::map<std::string /*subsystemName*/, std::string /*destinationSubsystemName*/>&                       subsystemObjectMap);

  private:
	static int  getSubsytemId(ConfigurationTree subsystemNode);
	static void extractRoutingMastersInfo(ConfigurationTree artdaqSupervisorNode, bool doWriteFHiCL, size_t routingTimeoutMs, size_t routingRetryCount);
	static void extractBoardReadersInfo(
	    ConfigurationTree artdaqSupervisorNode, bool doWriteFHiCL, size_t maxFragmentSizeBytes, size_t routingTimeoutMs, size_t routingRetryCount);
	static void       extractEventBuildersInfo(ConfigurationTree artdaqSupervisorNode, bool doWriteFHiCL, size_t maxFragmentSizeBytes);
	static void       extractDataLoggersInfo(ConfigurationTree artdaqSupervisorNode, bool doWriteFHiCL, size_t maxFragmentSizeBytes);
	static void       extractDispatchersInfo(ConfigurationTree artdaqSupervisorNode, bool doWriteFHiCL, size_t maxFragmentSizeBytes);
	static ArtdaqInfo info_;
};
}  // namespace ots

#endif
