#include "otsdaq-core/ConfigurationPluginDataFormats/IterateConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

#include <iostream>
#include <string>

using namespace ots;


const std::string IterateConfiguration::COMMAND_BEGIN_LABEL 			= "BEGIN_LABEL";
const std::string IterateConfiguration::COMMAND_REPEAT_LABEL 			= "REPEAT_LABEL";
const std::string IterateConfiguration::COMMAND_CONFIGURE_ALIAS 		= "CONFIGURE_ALIAS";
const std::string IterateConfiguration::COMMAND_CONFIGURE_GROUP 		= "CONFIGURE_GROUP";
const std::string IterateConfiguration::COMMAND_MODIFY_ACTIVE_GROUP 	= "MODIFY_ACTIVE_GROUP";
const std::string IterateConfiguration::COMMAND_CONFIGURE_ACTIVE_GROUP 	= "CONFIGURE_GROUP";
const std::string IterateConfiguration::COMMAND_EXECUTE_MACRO 			= "EXECUTE_MACRO";
const std::string IterateConfiguration::COMMAND_EXECUTE_FE_MACRO 		= "EXECUTE_FE_MACRO";
const std::string IterateConfiguration::COMMAND_RUN 					= "RUN";

const std::string IterateConfiguration::ITERATE_TABLE	= "IterateConfiguration";
const std::string IterateConfiguration::PLAN_TABLE		= "IterationPlanConfiguration";

const std::map<std::string,std::string> IterateConfiguration::commandToTableMap_ = IterateConfiguration::createCommandToTableMap();

IterateConfiguration::PlanTableColumns IterateConfiguration::planTableCols_;
IterateConfiguration::IterateTableColumns IterateConfiguration::iterateTableCols_;


//==============================================================================
IterateConfiguration::IterateConfiguration(void)
: ConfigurationBase(IterateConfiguration::ITERATE_TABLE)
{
}

//==============================================================================
IterateConfiguration::~IterateConfiguration(void)
{
}

//==============================================================================
void IterateConfiguration::init(ConfigurationManager *configManager)
{
	//do something to validate or refactor table
	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
	__COUT__ << configManager->__SELF_NODE__ << std::endl;

	std::string value;
	auto childrenMap = configManager->__SELF_NODE__.getChildren();
	for(auto &childPair: childrenMap)
	{
		//do something for each row in table
		__COUT__ << childPair.first << std::endl;
		//		__COUT__ << childPair.second.getNode(colNames_.colColumnName_) << std::endl;
		//childPair.second.getNode(colNames_.colColumnName_	).getValue(value);
	}
}

//==============================================================================
std::vector<IterateConfiguration::Command> IterateConfiguration::getPlanCommands(
		ConfigurationManager *configManager, const std::string& plan) const
{
	__COUT__ << configManager->__SELF_NODE__ << std::endl;

	ConfigurationTree planNode = configManager->__SELF_NODE__.getNode(plan);

	if(!planNode.getNode(IterateConfiguration::planTableCols_.Status_).getValue<bool>())
	{
		__SS__ << "Error! Attempt to access disabled plan (Status=FALSE)." << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}

	std::vector<IterateConfiguration::Command> commands;

	auto commandChildren = planNode.getNode(
			IterateConfiguration::iterateTableCols_.PlanLink_).getChildren();

	for(auto &commandChild: commandChildren)
	{
		__COUT__ << "Command \t" << commandChild.first << std::endl;

		if(!commandChild.second.getNode(
				IterateConfiguration::planTableCols_.Status_).getValue<bool>())
			continue; //skip disabled commands

		__COUT__ << "\t\tType \t" << commandChild.second.getNode(
				IterateConfiguration::planTableCols_.CommandType_) << std::endl;

		auto commandSpecificFields = commandChild.second.getNode(
				IterateConfiguration::planTableCols_.CommandLink_).getChildren();

		for(auto &commandField: commandSpecificFields)
		{
			__COUT__ << "\t\tParameter \t" << commandField.first << std::endl;
			__COUT__ << "\t\tParameter \t" << commandField.second << std::endl;


//			for(auto &commandParameter: commandParameters)
//			{
//				__COUT__ << "\t\tParameter \t" << commandParameter.first << std::endl;
//
//			}
		}
	}

	return commands;
}

DEFINE_OTS_CONFIGURATION(IterateConfiguration)
