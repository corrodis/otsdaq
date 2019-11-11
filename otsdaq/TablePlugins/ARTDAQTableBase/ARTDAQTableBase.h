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
// clang-format off
class ARTDAQTableBase : public TableBase
{
  public:
	ARTDAQTableBase();
	ARTDAQTableBase(std::string tableName, std::string* accumulatedExceptions = 0);

	virtual ~ARTDAQTableBase(void);

	static const std::string ARTDAQ_FCL_PATH;
	static const std::string ARTDAQ_SUPERVISOR_TABLE;
	static const std::string ARTDAQ_READER_TABLE, ARTDAQ_BUILDER_TABLE, ARTDAQ_LOGGER_TABLE, ARTDAQ_DISPATCHER_TABLE, ARTDAQ_MONITOR_TABLE, ARTDAQ_ROUTER_TABLE;
	static const std::string ARTDAQ_SUBSYSTEM_TABLE;
	static const std::string ARTDAQ_TYPE_TABLE_HOSTNAME, ARTDAQ_TYPE_TABLE_SUBSYSTEM_LINK, ARTDAQ_TYPE_TABLE_SUBSYSTEM_LINK_UID;

	enum class ARTDAQAppType
	{
		BoardReader,
		EventBuilder,
		DataLogger,
		Dispatcher,
		Monitor,
		RoutingMaster
	};

	enum
	{
		DEFAULT_MAX_FRAGMENT_SIZE   = 1048576,
		DEFAULT_ROUTING_TIMEOUT_MS  = 1000,
		DEFAULT_ROUTING_RETRY_COUNT = 5,
	};

	static const int         NULL_SUBSYSTEM_DESTINATION;
	static const std::string NULL_SUBSYSTEM_DESTINATION_LABEL;

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

	struct ARTDAQInfo
	{
		std::map<int /*subsystem ID*/, ARTDAQTableBase::SubsystemInfo> subsystems;
		std::map<ARTDAQAppType, std::list<ProcessInfo>>                processes;
	};

	static const std::string& 				getTypeString				(ARTDAQAppType type);
	static std::string        				getFHICLFilename			(ARTDAQAppType type, const std::string& name);
	static std::string        				getFlatFHICLFilename		(ARTDAQAppType type, const std::string& name);
	static void               				flattenFHICL				(ARTDAQAppType type, const std::string& name);

	static void        						insertParameters			(std::ostream&      out,
	                   						                			 std::string&       tabStr,
	                   						                			 std::string&       commentStr,
	                   						                			 ConfigurationTree  parameterLink,
	                   						                			 const std::string& parameterPreamble,
	                   						                			 bool               onlyInsertAtTableParameters = false,
	                   						                			 bool               includeAtTableParameters    = false);
	static std::string 						insertModuleType			(std::ostream& out, std::string& tabStr, std::string& commentStr, ConfigurationTree moduleTypeNode);
	static void        						insertMetricsBlock			(std::ostream& out, std::string& tabStr, std::string& commentStr, ConfigurationTree daqNode);

	static void 							outputBoardReaderFHICL		(const ConfigurationTree& boardReaderNode,
	            							                      		 size_t                   maxFragmentSizeBytes = DEFAULT_MAX_FRAGMENT_SIZE,
	            							                      		 size_t                   routingTimeoutMs     = DEFAULT_ROUTING_TIMEOUT_MS,
	            							                      		 size_t                   routingRetryCount    = DEFAULT_ROUTING_RETRY_COUNT);

	static void 							outputDataReceiverFHICL		(const ConfigurationTree& receiverNode,
	            							                       		 ARTDAQAppType            appType,
	            							                       		 size_t                   maxFragmentSizeBytes = DEFAULT_MAX_FRAGMENT_SIZE,
	            							                       		 size_t                   routingTimeoutMs     = DEFAULT_ROUTING_TIMEOUT_MS,
	            							                       		 size_t                   routingRetryCount    = DEFAULT_ROUTING_RETRY_COUNT);

	static void 							outputRoutingMasterFHICL	(const ConfigurationTree& routingMasterNode,
	            							                        	 size_t                   routingTimeoutMs  = DEFAULT_ROUTING_TIMEOUT_MS,
	            							                        	 size_t                   routingRetryCount = DEFAULT_ROUTING_RETRY_COUNT);

	static const ARTDAQInfo& 				extractARTDAQInfo			(ConfigurationTree artdaqSupervisorNode,
	                  						                 			 bool              doWriteFHiCL         = false,
	                  						                 			 size_t            maxFragmentSizeBytes = DEFAULT_MAX_FRAGMENT_SIZE,
	                  						                 			 size_t            routingTimeoutMs     = DEFAULT_ROUTING_TIMEOUT_MS,
	                  						                 			 size_t            routingRetryCount    = DEFAULT_ROUTING_RETRY_COUNT,
	                  						                 			 ProgressBar*      progressBar          = 0);
	static void       						setAndActivateARTDAQSystem	(ConfigurationManagerRW*                                                                                        cfgMgr,
																         const std::map<std::string /*type*/, std::map<std::string /*record*/, std::vector<std::string /*property*/>>>& nodeTypeToObjectMap,
																         const std::map<std::string /*subsystemName*/, std::string /*destinationSubsystemName*/>&                       subsystemObjectMap);

  private:
	static int  							getSubsytemId				(ConfigurationTree subsystemNode);
	static void 							extractRoutingMastersInfo	(ConfigurationTree artdaqSupervisorNode, bool doWriteFHiCL, size_t routingTimeoutMs, size_t routingRetryCount);
	static void 							extractBoardReadersInfo		(ConfigurationTree artdaqSupervisorNode, bool doWriteFHiCL, size_t maxFragmentSizeBytes, size_t routingTimeoutMs, size_t routingRetryCount);
	static void       						extractEventBuildersInfo	(ConfigurationTree artdaqSupervisorNode, bool doWriteFHiCL, size_t maxFragmentSizeBytes);
	static void       						extractDataLoggersInfo		(ConfigurationTree artdaqSupervisorNode, bool doWriteFHiCL, size_t maxFragmentSizeBytes);
	static void       						extractDispatchersInfo		(ConfigurationTree artdaqSupervisorNode, bool doWriteFHiCL, size_t maxFragmentSizeBytes);
		
	static ARTDAQInfo 						info_;

  public:
	static struct ProcessTypes
	{
		std::string const READER        = "reader";
		std::string const BUILDER       = "builder";
		std::string const LOGGER        = "logger";
		std::string const DISPATCHER    = "dispatcher";
		std::string const MONITOR       = "monitor";
		std::string const ROUTER		= "router";

		ProcessTypes()
		    : mapToTable_({		    	
				std::make_pair(READER, 		ARTDAQTableBase::ARTDAQ_READER_TABLE),
				std::make_pair(BUILDER, 	ARTDAQTableBase::ARTDAQ_BUILDER_TABLE),
				std::make_pair(LOGGER, 		ARTDAQTableBase::ARTDAQ_LOGGER_TABLE),
				std::make_pair(DISPATCHER, 	ARTDAQTableBase::ARTDAQ_DISPATCHER_TABLE),
				std::make_pair(MONITOR, 	ARTDAQTableBase::ARTDAQ_MONITOR_TABLE),
				std::make_pair(ROUTER, 		ARTDAQTableBase::ARTDAQ_ROUTER_TABLE)})
		    , mapToGroupIDAppend_({		    	
				std::make_pair(READER, 		"BoardReaders"),
				std::make_pair(BUILDER, 	"EventBuilders"),
				std::make_pair(LOGGER, 		"DataLoggers"),
				std::make_pair(DISPATCHER, 	"Dispatchers"),
				std::make_pair(MONITOR, 	"Monitors"),
				std::make_pair(ROUTER, 		"RoutingMasters")})
		    , mapToLinkGroupIDColumn_({		    	
				std::make_pair(READER, 		ARTDAQTableBase::colARTDAQSupervisor_.colLinkToBoardReadersGroupID_),
				std::make_pair(BUILDER, 	ARTDAQTableBase::colARTDAQSupervisor_.colLinkToEventBuildersGroupID_),
				std::make_pair(LOGGER, 		ARTDAQTableBase::colARTDAQSupervisor_.colLinkToDataLoggersGroupID_),
				std::make_pair(DISPATCHER, 	ARTDAQTableBase::colARTDAQSupervisor_.colLinkToDispatchersGroupID_),
				std::make_pair(ROUTER, 		ARTDAQTableBase::colARTDAQSupervisor_.colLinkToRoutingMastersGroupID_)})
		    , mapToGroupIDColumn_({		    	
				std::make_pair(READER, 		"BoardReaderGroupID"),
				std::make_pair(BUILDER, 	"EventBuilderGroupID"),
				std::make_pair(LOGGER, 		"DataLoggerGroupID"),
				std::make_pair(DISPATCHER, 	"DispatcherGroupID"),
				std::make_pair(MONITOR, 	"MonitorGroupID"),
				std::make_pair(ROUTER, 		"RoutingMasterGroupID")})
		{}

		const std::map<std::string /*processType*/, std::string /*typeTable*/> mapToTable_, mapToGroupIDAppend_, mapToLinkGroupIDColumn_, mapToGroupIDColumn_;
	} processTypes_;

	// ARTDAQ Supervisor Column names
	static struct ColARTDAQSupervisor
	{
		std::string const colDAQInterfaceDebugLevel_ 		= "DAQInterfaceDebugLevel";
		std::string const colDAQSetupScript_         		= "DAQSetupScript";

		std::string const colLinkToBoardReaders_          	= "BoardReadersLink";
		std::string const colLinkToBoardReadersGroupID_   	= "BoardReadersLinkGroupID";
		std::string const colLinkToEventBuilders_         	= "EventBuildersLink";
		std::string const colLinkToEventBuildersGroupID_  	= "EventBuildersLinkGroupID";
		std::string const colLinkToDataLoggers_           	= "DataLoggersLink";
		std::string const colLinkToDataLoggersGroupID_    	= "DataLoggersLinkGroupID";
		std::string const colLinkToDispatchers_           	= "DispatchersLink";
		std::string const colLinkToDispatchersGroupID_    	= "DispatchersLinkGroupID";
		std::string const colLinkToRoutingMasters_        	= "RoutingMastersLink";
		std::string const colLinkToRoutingMastersGroupID_ 	= "RoutingMastersLinkGroupID";
	} colARTDAQSupervisor_;

	// ARTDAQ Supervisor Column names
	static struct ColARTDAQSubsystem
	{
		std::string const colLinkToDestination_    			= "SubsystemDestinationLink";
		std::string const colLinkToDestinationUID_ 			= "SubsystemDestinationUID";
	} colARTDAQSubsystem_;
};
// clang-format on
}  // namespace ots

#endif
