#ifndef _ots_IterateConfiguration_h_
#define _ots_IterateConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"


namespace ots
{

class IterateConfiguration : public ConfigurationBase
{

public:

	IterateConfiguration(void);
	virtual ~IterateConfiguration(void);

	//Methods
	void init(ConfigurationManager *configManager);

	//Getters



	static const std::string COMMAND_BEGIN_LABEL;
	static const std::string COMMAND_CONFIGURE_ACTIVE_GROUP;
	static const std::string COMMAND_CONFIGURE_ALIAS;
	static const std::string COMMAND_CONFIGURE_GROUP;
	static const std::string COMMAND_EXECUTE_FE_MACRO;
	static const std::string COMMAND_EXECUTE_MACRO;
	static const std::string COMMAND_MODIFY_ACTIVE_GROUP;
	static const std::string COMMAND_REPEAT_LABEL;
	static const std::string COMMAND_RUN;

	static const std::string ITERATE_TABLE;
	static const std::string PLAN_TABLE;

	static const std::map<std::string,std::string> commandToTableMap_;
	static std::map<std::string,std::string> createCommandToTableMap()
	{
		std::map<std::string,std::string> m;
		m[COMMAND_BEGIN_LABEL] 				=  "IterationCommandBeginLabelConfiguration";
		m[COMMAND_CONFIGURE_ACTIVE_GROUP] 	=  ""; //no parameters
		m[COMMAND_CONFIGURE_ALIAS] 			=  "IterationCommandConfigureAliasConfiguration";
		m[COMMAND_CONFIGURE_GROUP] 			=  "IterationCommandConfigureGroupConfiguration";
		m[COMMAND_EXECUTE_FE_MACRO] 		=  "IterationCommandExecuteFEMacroConfiguration";
		m[COMMAND_EXECUTE_MACRO] 			=  "IterationCommandExecuteMacroConfiguration";
		m[COMMAND_MODIFY_ACTIVE_GROUP] 		=  "IterationCommandModifyGroupConfiguration";
		m[COMMAND_REPEAT_LABEL] 			=  "IterationCommandRepeatLabelConfiguration";
		m[COMMAND_RUN] 						=  "IterationCommandRunConfiguration";
		return m;
	}




	//Table hierarchy is as follows:
	//	Iterate
	//		|- Plan
	//			|-Command Type 1
	//			|-Command Type 2 ...
	//			|-Command Type n

	//Column names
	static struct PlanTableColumns
	{
		std::string const GroupID_ = "IterationPlanGroupID";
		std::string const CommandLink_ = "LinkToCommandUID";
	} planTableCols_;

	static struct IterateTableColumns
	{
		std::string const PlanLink_ = "LinkToIterationPlanConfiguration";
	} iterateTableCols_;

};
}
#endif
