#ifndef _ots_ARTDAQTableBase_h_
#define _ots_ARTDAQTableBase_h_

#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/TableCore/TableBase.h"

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

	std::string 				getFHICLFilename			(const std::string& type, const std::string& name);
	std::string 				getFlatFHICLFilename		(const std::string& type, const std::string& name);
	void        				flattenFHICL				(const std::string& type, const std::string& name);

	void						insertParameters			(std::ostream& out, std::string& tabStr, std::string& commentStr, ConfigurationTree parameterLink, const std::string& parameterPreamble, bool onlyInsertAtTableParameters = false, bool includeAtTableParameters =false);
	std::string					insertModuleType			(std::ostream& out, std::string& tabStr, std::string& commentStr, ConfigurationTree moduleTypeNode);
	
	enum class DataReceiverAppType {
		EventBuilder,
		DataLogger,
		Dispatcher
	};

	void outputDataReceiverFHICL(ConfigurationManager*    configManager,
	                 const ConfigurationTree& appNode,
	                 unsigned int             selfRank,
	                 std::string              selfHost,
	                 unsigned int             selfPort,
	                 DataReceiverAppType appType);

	std::string getPreamble(DataReceiverAppType type) {
		switch (type) {
			case DataReceiverAppType::EventBuilder: return "builder";
			case DataReceiverAppType::DataLogger: return "datalogger";
			case DataReceiverAppType::Dispatcher: return "dispatcher";
		}
		return "UNKNOWN";
	}
};
// clang-format on
}  // namespace ots

#endif
