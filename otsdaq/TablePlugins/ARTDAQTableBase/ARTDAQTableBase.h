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

	static const std::string		ARTDAQ_FCL_PATH;
	static const std::string		ARTDAQ_SUPERVISOR_TABLE;
	static const std::string		ARTDAQ_READER_TABLE, ARTDAQ_BUILDER_TABLE, ARTDAQ_LOGGER_TABLE, ARTDAQ_DISPATCHER_TABLE, ARTDAQ_MONITOR_TABLE;
	static const std::string		ARTDAQ_SUBSYSTEM_TABLE;
	static const std::string		ARTDAQ_TYPE_TABLE_HOSTNAME, ARTDAQ_TYPE_TABLE_SUBSYSTEM_LINK, ARTDAQ_TYPE_TABLE_SUBSYSTEM_LINK_UID;

	enum class ARTDAQAppType {
		BoardReader,
		EventBuilder,
		DataLogger,
		Dispatcher,
		Monitor
	};


	enum {
		DEFAULT_MAX_FRAGMENT_SIZE = 1048576
	};

	struct SubsystemInfo
	{
		int           	id;
		std::string 	label;

		std::set<int> 	sources; //by subsystem id
		int           	destination; //destination subsystem id, 0 := no destination

		SubsystemInfo() : sources(),destination(0) {}
	};
	static const int 		 	NULL_SUBSYSTEM_DESTINATION;

	struct ProcessInfo
	{
		std::string label;
		std::string hostname;
		int         subsystem;

		ProcessInfo(std::string l, std::string h, int s)
		    : label(l), hostname(h), subsystem(s) {}
	};


	static const std::string&	getTypeString				(ARTDAQAppType type);
	static std::string 			getFHICLFilename			(ARTDAQTableBase::ARTDAQAppType type, const std::string& name);
	static std::string 			getFlatFHICLFilename		(ARTDAQTableBase::ARTDAQAppType type, const std::string& name);
	static void        			flattenFHICL				(ARTDAQTableBase::ARTDAQAppType type, const std::string& name);

	static void					insertParameters			(std::ostream& out, std::string& tabStr, std::string& commentStr, ConfigurationTree parameterLink, const std::string& parameterPreamble, bool onlyInsertAtTableParameters = false, bool includeAtTableParameters =false);
	static std::string			insertModuleType			(std::ostream& out, std::string& tabStr, std::string& commentStr, ConfigurationTree moduleTypeNode);

	static void 				outputReaderFHICL			(
															const ConfigurationTree& readerNode,
															size_t maxFragmentSizeBytes = ARTDAQTableBase::DEFAULT_MAX_FRAGMENT_SIZE);


	static void 				outputDataReceiverFHICL		(
															const ConfigurationTree& 		receiverNode,
															ARTDAQTableBase::ARTDAQAppType 	appType,
															size_t 							maxFragmentSizeBytes = ARTDAQTableBase::DEFAULT_MAX_FRAGMENT_SIZE);

	static void 				extractArtdaqInfo			(
															ConfigurationTree 				artdaqSupervisorNode,
															std::map<int /*subsystem ID*/, ARTDAQTableBase::SubsystemInfo>& subsystems,
															std::map<ARTDAQTableBase::ARTDAQAppType, std::list<ARTDAQTableBase::ProcessInfo>>& 	processes,
															bool							doWriteFHiCL = false,
															size_t 							maxFragmentSizeBytes = ARTDAQTableBase::DEFAULT_MAX_FRAGMENT_SIZE,
															ProgressBar* 					progressBar = 0);
	static void					setAndActivateArtdaqSystem	(
															ConfigurationManagerRW* 			cfgMgr,
															const std::map<std::string /*type*/,
																std::map<std::string /*record*/,
																	std::vector<std::string /*property*/>>>& 			nodeTypeToObjectMap,
															const std::map<std::string /*subsystemName*/,
																			std::string /*destinationSubsystemName*/>& 	subsystemObjectMap
															);

  private:
	static int					getSubsytemId				(ConfigurationTree subsystemNode);

  public:

	static struct ProcessTypes
	{
		std::string const READER 		= "reader";
		std::string const BUILDER     	= "builder";
		std::string const LOGGER        = "logger";
		std::string const DISPATCHER    = "dispatcher";
		std::string const MONITOR    	= "monitor";

		ProcessTypes()
		: mapToTable_({
			std::make_pair(READER,		ARTDAQTableBase::ARTDAQ_READER_TABLE		),
			std::make_pair(BUILDER,		ARTDAQTableBase::ARTDAQ_BUILDER_TABLE		),
			std::make_pair(LOGGER,		ARTDAQTableBase::ARTDAQ_LOGGER_TABLE		),
			std::make_pair(DISPATCHER,	ARTDAQTableBase::ARTDAQ_DISPATCHER_TABLE	),
			std::make_pair(MONITOR,		ARTDAQTableBase::ARTDAQ_MONITOR_TABLE		)
		})
		, mapToGroupIDAppend_({
					std::make_pair(READER,		"BoardReaders"		),
					std::make_pair(BUILDER,		"EventBuilders"		),
					std::make_pair(LOGGER,		"DataLoggers"		),
					std::make_pair(DISPATCHER,	"Dispatchers"		),
					std::make_pair(MONITOR,		"Monitors"			)
				})
		, mapToLinkGroupIDColumn_({
					std::make_pair(READER,		ARTDAQTableBase::colARTDAQSupervisor_.colLinkToBoardReadersGroupID_		),
					std::make_pair(BUILDER,		ARTDAQTableBase::colARTDAQSupervisor_.colLinkToEventBuildersGroupID_		),
					std::make_pair(LOGGER,		ARTDAQTableBase::colARTDAQSupervisor_.colLinkToDataLoggersGroupID_		),
					std::make_pair(DISPATCHER,	ARTDAQTableBase::colARTDAQSupervisor_.colLinkToDispatchersGroupID_		)
					//std::make_pair(MONITOR,		ARTDAQTableBase::colARTDAQSupervisor_.colLinkToMonitorssGroupID_		)
				})
		, mapToGroupIDColumn_({
					std::make_pair(READER,		"BoardReadersGroupID"		),
					std::make_pair(BUILDER,		"EventBuildersGroupID"		),
					std::make_pair(LOGGER,		"DataLoggersGroupID"		),
					std::make_pair(DISPATCHER,	"DispatchersGroupID"		),
					std::make_pair(MONITOR,		"MonitorsGroupID"			)
				})
		{}

		const std::map<std::string /*processType*/, std::string /*typeTable*/>
			mapToTable_, mapToGroupIDAppend_, mapToLinkGroupIDColumn_, mapToGroupIDColumn_;
	} processTypes_;

	// ARTDAQ Supervisor Column names
	static struct ColARTDAQSupervisor
	{
		std::string const colDAQInterfaceDebugLevel_      	= "DAQInterfaceDebugLevel";
		std::string const colDAQSetupScript_               	= "DAQSetupScript";

		std::string const colLinkToBoardReaders_   			= "boardreadersLink";
		std::string const colLinkToBoardReadersGroupID_ 	= "boardreadersLinkGroupID";
		std::string const colLinkToEventBuilders_   		= "eventbuildersLink";
		std::string const colLinkToEventBuildersGroupID_ 	= "eventbuildersLinkGroupID";
		std::string const colLinkToDataLoggers_   			= "dataloggersLink";
		std::string const colLinkToDataLoggersGroupID_ 		= "dataloggersLinkGroupID";
		std::string const colLinkToDispatchers_   			= "dispatchersLink";
		std::string const colLinkToDispatchersGroupID_ 		= "dispatchersLinkGroupID";
	} colARTDAQSupervisor_;

	// ARTDAQ Supervisor Column names
	static struct ColARTDAQSubsystem
	{
		std::string const colLinkToDestination_      	= "SubsystemDestinationLink";
		std::string const colLinkToDestinationUID_      = "SubsystemDestinationUID";
	} colARTDAQSubsystem_;


};
// clang-format on
}  // namespace ots

#endif
