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

	struct Command {
		std::string type;
		std::map<
			std::string /*param name*/,
			std::string /*param value*/> params;
	};

	std::vector<IterateConfiguration::Command> getPlanCommands(ConfigurationManager *configManager, const std::string& plan) const;


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

	static struct CommandBeginLabelParams
	{
		const std::string Label_ = "Label";
	} commandBeginLabelParams_;
	static struct CommandConfigureActiveParams
	{
		const std::string Label_ = "Label";
	} commandConfigureActiveParams_;
	static struct CommandConfigureAliasParams
	{
		const std::string Label_ = "Label";
	} commandConfigureAliasParams_;
	static struct CommandConfigureGroupParams
	{
		const std::string Label_ = "Label";
	} commandConfigureGroupParams_;
	static struct CommandExecuteFEMacroParams
	{
		const std::string Label_ = "Label";
	} commandExecuteFEMacroParams_;
	static struct CommandExecuteMacroParams
	{
		const std::string Label_ = "Label";
	} commandExecuteMacroParams_;
	static struct CommandModifyActiveParams
	{
		const std::string Label_ = "Label";
	} commandModifyActiveParams_;
	static struct CommandRepeatLabelParams
	{
		const std::string Label_ = "Label";
		const std::string NumberOfRepetitions_ = "NumberOfRepetitions";
	} commandRepeatLabelParams_;
	static struct CommandRunParams
	{
		const std::string Label_ = "Label";
	} commandRunParams_;




	//Table hierarchy is as follows:
	//	Iterate
	//		|- Plan
	//			|-Command Type 1
	//			|-Command Type 2 ...
	//			|-Command Type n

	//Column names

	static struct IterateTableColumns
	{
		const std::string PlanLink_ = "LinkToIterationPlanConfiguration";
	} iterateTableCols_;

	static struct PlanTableColumns
	{
		const std::string Status_ = ViewColumnInfo::COL_NAME_STATUS;
		const std::string GroupID_ = "IterationPlanGroupID";
		const std::string CommandLink_ = "LinkToCommandUID";
		const std::string CommandType_ = "CommandType";
	} planTableCols_;



};
}
#endif
