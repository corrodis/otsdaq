#include "otsdaq-core/ConfigurationPluginDataFormats/IterateConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

#include <iostream>
#include <string>

using namespace ots;

//instantiate static members

const std::string IterateConfiguration::COMMAND_BEGIN_LABEL 			= "BEGIN_LABEL";
const std::string IterateConfiguration::COMMAND_CHOOSE_FSM		 		= "CHOOSE_FSM";
const std::string IterateConfiguration::COMMAND_CONFIGURE_ACTIVE_GROUP 	= "CONFIGURE_ACTIVE_GROUP";
const std::string IterateConfiguration::COMMAND_CONFIGURE_ALIAS 		= "CONFIGURE_ALIAS";
const std::string IterateConfiguration::COMMAND_CONFIGURE_GROUP 		= "CONFIGURE_GROUP";
const std::string IterateConfiguration::COMMAND_EXECUTE_FE_MACRO 		= "EXECUTE_FE_MACRO";
const std::string IterateConfiguration::COMMAND_EXECUTE_MACRO 			= "EXECUTE_MACRO";
const std::string IterateConfiguration::COMMAND_MODIFY_ACTIVE_GROUP 	= "MODIFY_ACTIVE_GROUP";
const std::string IterateConfiguration::COMMAND_REPEAT_LABEL 			= "REPEAT_LABEL";
const std::string IterateConfiguration::COMMAND_RUN 					= "RUN";

const std::string IterateConfiguration::ITERATE_TABLE	= "IterateConfiguration";
const std::string IterateConfiguration::PLAN_TABLE		= "IterationPlanConfiguration";
const std::string IterateConfiguration::TARGET_TABLE	= "IterationTargetConfiguration";

const std::map<std::string,std::string> 			IterateConfiguration::commandToTableMap_ = IterateConfiguration::createCommandToTableMap();

IterateConfiguration::PlanTableColumns 				IterateConfiguration::planTableCols_;
IterateConfiguration::IterateTableColumns 			IterateConfiguration::iterateTableCols_;

IterateConfiguration::CommandBeginLabelParams 		IterateConfiguration::commandBeginLabelParams_;
IterateConfiguration::CommandConfigureActiveParams 	IterateConfiguration::commandConfigureActiveParams_;
IterateConfiguration::CommandConfigureAliasParams 	IterateConfiguration::commandConfigureAliasParams_;
IterateConfiguration::CommandConfigureGroupParams 	IterateConfiguration::commandConfigureGroupParams_;
IterateConfiguration::CommandExecuteFEMacroParams 	IterateConfiguration::commandExecuteFEMacroParams_;
IterateConfiguration::CommandExecuteMacroParams 	IterateConfiguration::commandExecuteMacroParams_;
IterateConfiguration::CommandModifyActiveParams 	IterateConfiguration::commandModifyActiveParams_;
IterateConfiguration::CommandRepeatLabelParams 		IterateConfiguration::commandRepeatLabelParams_;
IterateConfiguration::CommandRunParams 				IterateConfiguration::commandRunParams_;
IterateConfiguration::CommandChooseFSMParams 		IterateConfiguration::commandChooseFSMParams_;

IterateConfiguration::TargetParams 					IterateConfiguration::targetParams_;
IterateConfiguration::TargetTableColumns			IterateConfiguration::targetCols_;
IterateConfiguration::CommandTargetColumns	 		IterateConfiguration::commandTargetCols_;



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

		__COUT__ << "\t\tStatus \t" << commandChild.second.getNode(
				IterateConfiguration::planTableCols_.Status_) << std::endl;

		__COUT__ << "\t\tType \t" << commandChild.second.getNode(
				IterateConfiguration::planTableCols_.CommandType_) << std::endl;

		if(!commandChild.second.getNode(
				IterateConfiguration::planTableCols_.Status_).getValue<bool>())
			continue; //skip disabled commands

		commands.push_back(IterateConfiguration::Command());
		commands.back().type_ = commandChild.second.getNode(
				IterateConfiguration::planTableCols_.CommandType_).getValue<std::string>();

		if(commandChild.second.getNode(
						IterateConfiguration::planTableCols_.CommandLink_).isDisconnected())
			continue; //skip if no command parameters

		auto commandSpecificFields = commandChild.second.getNode(
				IterateConfiguration::planTableCols_.CommandLink_).getChildren();

		for(unsigned int i=0; i<commandSpecificFields.size()-3; ++i) //ignore last three columns
		{
			__COUT__ << "\t\tParameter \t" << commandSpecificFields[i].first << " = \t" <<
					commandSpecificFields[i].second << std::endl;

			if(commandSpecificFields[i].first ==
					IterateConfiguration::commandTargetCols_.TargetsLink_)
			{
				__COUT__ << "Extracting targets..." << __E__;
				auto targets = commandSpecificFields[i].second.getChildren();
				for(auto& target:targets)
				{
					__COUT__ << "\t\t\tTarget \t" << target.first << __E__;

					ConfigurationTree targetNode =
							target.second.getNode(
									IterateConfiguration::targetCols_.TargetLink_);
					if(targetNode.isDisconnected())
					{
						__COUT_ERR__ << "Disconnected target!?" << __E__;
						continue;
					}

					__COUT__ << "\t\t = \t" <<
							"Table:" <<
							targetNode.getConfigurationName() <<
							" UID:" <<
							targetNode.getValueAsString() << std::endl;
					commands.back().addTarget();
					commands.back().targets_.back().table_ = targetNode.getConfigurationName();
					commands.back().targets_.back().UID_ = targetNode.getValueAsString();
				}
			}
			else
				commands.back().params_.emplace(std::pair<
					std::string /*param name*/,
					std::string /*param value*/>(
							commandSpecificFields[i].first,
							commandSpecificFields[i].second.getValueAsString()));
		}
	}

	return commands;
}

DEFINE_OTS_CONFIGURATION(IterateConfiguration)
