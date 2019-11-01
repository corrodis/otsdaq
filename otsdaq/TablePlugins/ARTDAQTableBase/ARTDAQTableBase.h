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

	enum class ARTDAQAppType {
		BoardReader,
		EventBuilder,
		DataLogger,
		Dispatcher,
		Monitor
	};

	static struct ProcessTypes
	{
		std::string const READER 		= "reader";
		std::string const BUILDER     	= "builder";
		std::string const LOGGER        = "logger";
		std::string const DISPATCHER    = "dispatcher";
		std::string const MONITOR    	= "monitor";
	} processTypes_;

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

};
// clang-format on
}  // namespace ots

#endif
