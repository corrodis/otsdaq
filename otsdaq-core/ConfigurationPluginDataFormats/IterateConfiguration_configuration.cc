#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/IterateConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>
#include <string>

using namespace ots;

// instantiate static members

const std::string IterateConfiguration::COMMAND_BEGIN_LABEL = "BEGIN_LABEL";
const std::string IterateConfiguration::COMMAND_CHOOSE_FSM  = "CHOOSE_FSM";
const std::string IterateConfiguration::COMMAND_CONFIGURE_ACTIVE_GROUP =
    "CONFIGURE_ACTIVE_GROUP";
const std::string IterateConfiguration::COMMAND_CONFIGURE_ALIAS  = "CONFIGURE_ALIAS";
const std::string IterateConfiguration::COMMAND_CONFIGURE_GROUP  = "CONFIGURE_GROUP";
const std::string IterateConfiguration::COMMAND_EXECUTE_FE_MACRO = "EXECUTE_FE_MACRO";
const std::string IterateConfiguration::COMMAND_EXECUTE_MACRO    = "EXECUTE_MACRO";
const std::string IterateConfiguration::COMMAND_MODIFY_ACTIVE_GROUP =
    "MODIFY_ACTIVE_GROUP";
const std::string IterateConfiguration::COMMAND_REPEAT_LABEL = "REPEAT_LABEL";
const std::string IterateConfiguration::COMMAND_RUN          = "RUN";

const std::string IterateConfiguration::ITERATE_TABLE = "IterateConfiguration";
const std::string IterateConfiguration::PLAN_TABLE    = "IterationPlanConfiguration";
const std::string IterateConfiguration::TARGET_TABLE  = "IterationTargetConfiguration";

const std::map<std::string, std::string> IterateConfiguration::commandToTableMap_ =
    IterateConfiguration::createCommandToTableMap();

IterateConfiguration::PlanTableColumns    IterateConfiguration::planTableCols_;
IterateConfiguration::IterateTableColumns IterateConfiguration::iterateTableCols_;

IterateConfiguration::CommandBeginLabelParams
    IterateConfiguration::commandBeginLabelParams_;
IterateConfiguration::CommandConfigureActiveParams
    IterateConfiguration::commandConfigureActiveParams_;
IterateConfiguration::CommandConfigureAliasParams
    IterateConfiguration::commandConfigureAliasParams_;
IterateConfiguration::CommandConfigureGroupParams
    IterateConfiguration::commandConfigureGroupParams_;
IterateConfiguration::CommandExecuteMacroParams
    IterateConfiguration::commandExecuteMacroParams_;
IterateConfiguration::CommandModifyActiveParams
    IterateConfiguration::commandModifyActiveParams_;
IterateConfiguration::CommandRepeatLabelParams
                                       IterateConfiguration::commandRepeatLabelParams_;
IterateConfiguration::CommandRunParams IterateConfiguration::commandRunParams_;
IterateConfiguration::CommandChooseFSMParams
    IterateConfiguration::commandChooseFSMParams_;

IterateConfiguration::TargetParams         IterateConfiguration::targetParams_;
IterateConfiguration::TargetTableColumns   IterateConfiguration::targetCols_;
IterateConfiguration::CommandTargetColumns IterateConfiguration::commandTargetCols_;

IterateConfiguration::MacroDimLoopTableColumns IterateConfiguration::macroDimLoopCols_;
IterateConfiguration::MacroParamTableColumns   IterateConfiguration::macroParamCols_;

//==============================================================================
IterateConfiguration::IterateConfiguration(void)
    : TableBase(IterateConfiguration::ITERATE_TABLE)
{
}

//==============================================================================
IterateConfiguration::~IterateConfiguration(void) {}

//==============================================================================
void IterateConfiguration::init(ConfigurationManager* configManager)
{
	// do something to validate or refactor table
	//	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
	//	__COUT__ << configManager->__SELF_NODE__ << std::endl;

	std::string value;
	auto        childrenMap = configManager->__SELF_NODE__.getChildren();
	for(auto& childPair : childrenMap)
	{
		// do something for each row in table
		//__COUT__ << childPair.first << std::endl;
		//		__COUT__ << childPair.second.getNode(colNames_.colColumnName_) <<
		// std::endl;  childPair.second.getNode(colNames_.colColumnName_
		// ).getValue(value);
	}
}

//==============================================================================
std::vector<IterateConfiguration::Command> IterateConfiguration::getPlanCommands(
    ConfigurationManager* configManager, const std::string& plan) const
{
	__COUT__ << configManager->__SELF_NODE__ << std::endl;

	ConfigurationTree planNode = configManager->__SELF_NODE__.getNode(plan);

	if(!planNode.getNode(IterateConfiguration::planTableCols_.Status_).getValue<bool>())
	{
		__SS__ << "Error! Attempt to access disabled plan (Status=FALSE)." << std::endl;
		__COUT_ERR__ << ss.str();
		__SS_THROW__;
	}

	std::vector<IterateConfiguration::Command> commands;

	auto commandChildren =
	    planNode.getNode(IterateConfiguration::iterateTableCols_.PlanLink_).getChildren();

	for(auto& commandChild : commandChildren)
	{
		__COUT__ << "Command \t" << commandChild.first << std::endl;

		__COUT__ << "\t\tStatus \t"
		         << commandChild.second.getNode(
		                IterateConfiguration::planTableCols_.Status_)
		         << std::endl;

		__COUT__ << "\t\tType \t"
		         << commandChild.second.getNode(
		                IterateConfiguration::planTableCols_.CommandType_)
		         << std::endl;

		if(!commandChild.second.getNode(IterateConfiguration::planTableCols_.Status_)
		        .getValue<bool>())
			continue;  // skip disabled commands

		commands.push_back(IterateConfiguration::Command());
		commands.back().type_ =
		    commandChild.second.getNode(IterateConfiguration::planTableCols_.CommandType_)
		        .getValue<std::string>();

		if(commandChild.second.getNode(IterateConfiguration::planTableCols_.CommandLink_)
		       .isDisconnected())
			continue;  // skip if no command parameters

		auto commandSpecificFields =
		    commandChild.second.getNode(IterateConfiguration::planTableCols_.CommandLink_)
		        .getChildren();

		for(unsigned int i = 0; i < commandSpecificFields.size() - 3;
		    ++i)  // ignore last three columns
		{
			// NOTE -- that links turn into one field with value LinkID/GroupID unless
			// specially handled

			__COUT__ << "\t\tParameter \t" << commandSpecificFields[i].first << " = \t"
			         << commandSpecificFields[i].second << std::endl;

			if(commandSpecificFields[i].first ==
			   IterateConfiguration::commandTargetCols_.TargetsLink_)
			{
				__COUT__ << "Extracting targets..." << __E__;
				auto targets = commandSpecificFields[i].second.getChildren();

				__COUTV__(targets.size());

				for(auto& target : targets)
				{
					__COUT__ << "\t\t\tTarget \t" << target.first << __E__;

					ConfigurationTree targetNode = target.second.getNode(
					    IterateConfiguration::targetCols_.TargetLink_);
					if(targetNode.isDisconnected())
					{
						__COUT_ERR__ << "Disconnected target!?" << __E__;
						continue;
					}

					__COUT__ << "\t\t = \t"
					         << "Table:" << targetNode.getTableName()
					         << " UID:" << targetNode.getUIDAsString() << std::endl;
					commands.back().addTarget();
					commands.back().targets_.back().table_ = targetNode.getTableName();
					commands.back().targets_.back().UID_   = targetNode.getUIDAsString();
				}
			}
			else if(commandSpecificFields[i].first ==
			        IterateConfiguration::commandExecuteMacroParams_.MacroParameterLink_)
			{
				// get Macro parameters, place them in params_
				__COUT__ << "Extracting macro parameters..." << __E__;

				// need to extract input arguments
				//	by dimension (by priority)
				//
				//	two vector by dimension <map of params>
				//		one vector for long and for double
				//
				//	map of params :=
				//		name => {
				//			<long/double current value>
				//			<long/double init value>
				//			<long/double step size>
				//		}

				auto dimensionalLoops = commandSpecificFields[i].second.getChildren(
				    std::map<std::string /*relative-path*/,
				             std::string /*value*/>() /*no filter*/,
				    true /*by Priority*/);

				__COUTV__(dimensionalLoops.size());

				//	inputs:
				//		- inputArgs: dimensional semi-colon-separated,
				//			comma separated: dimension iterations and arguments
				//(colon-separated name/value/stepsize sets)
				std::string argStr = "";
				// inputArgsStr = "3;3,myOtherArg:5:2"; //example

				// std::string name, value;
				unsigned long numberOfIterations;
				bool          firstDimension = true;

				for(auto& dimensionalLoop : dimensionalLoops)
				{
					__COUT__ << "\t\t\tDimensionalLoop \t" << dimensionalLoop.first
					         << __E__;

					numberOfIterations =
					    dimensionalLoop.second
					        .getNode(IterateConfiguration::macroDimLoopCols_
					                     .NumberOfIterations_)
					        .getValue<unsigned long>();

					__COUTV__(numberOfIterations);

					if(numberOfIterations == 0)
					{
						__SS__ << "Illegal number of iterations value of '"
						       << numberOfIterations << ".' Must be a positive integer!"
						       << __E__;
						__SS_THROW__;
					}

					// at this point add dimension parameter with value numberOfIterations

					if(!firstDimension)
						argStr += ";";
					firstDimension = false;
					argStr += std::to_string(numberOfIterations);

					auto paramLinkNode = dimensionalLoop.second.getNode(
					    IterateConfiguration::macroDimLoopCols_.ParamLink_);

					if(paramLinkNode.isDisconnected())
					{
						__COUT__ << "Disconnected parameter link, so no parameters for "
						            "this dimension."
						         << __E__;
						continue;
					}

					auto macroParams = paramLinkNode.getChildren();

					__COUTV__(macroParams.size());

					for(auto& macroParam : macroParams)
					{
						__COUT__ << "\t\t\tParam \t" << macroParam.first << __E__;

						// add parameter name:value:step

						argStr += ",";
						argStr +=
						    macroParam.second
						        .getNode(IterateConfiguration::macroParamCols_.Name_)
						        .getValue<std::string>();
						argStr += ":";
						argStr +=
						    macroParam.second
						        .getNode(IterateConfiguration::macroParamCols_.Value_)
						        .getValue<std::string>();
						argStr += ":";
						argStr +=
						    macroParam.second
						        .getNode(IterateConfiguration::macroParamCols_.Step_)
						        .getValue<std::string>();

					}  // end parameter loop
				}      // end dimension loop

				// Macro argument string is done
				__COUTV__(argStr);

				// assume no conflict with fixed parameters in map
				//	because of prepend
				// IterateConfiguration::commandExecuteMacroParams_.MacroParameterPrepend_
				//+
				commands.back().params_.emplace(std::pair<std::string /*param name*/,
				                                          std::string /*param value*/>(
				    IterateConfiguration::commandExecuteMacroParams_.MacroArgumentString_,
				    argStr));
			}
			else  // all other non-special fields
			{
				if(  // bool type, convert to 1 or 0
				    commandSpecificFields[i].second.isValueBoolType())
					commands.back().params_.emplace(
					    std::pair<std::string /*param name*/,
					              std::string /*param value*/>(
					        commandSpecificFields[i].first,
					        commandSpecificFields[i].second.getValue<bool>() ? "1"
					                                                         : "0"));
				else if(  // number data type, get raw value string (note: does not do
				          // math or variable substitution)
				    commandSpecificFields[i].second.isValueNumberDataType())
					commands.back().params_.emplace(
					    std::pair<std::string /*param name*/,
					              std::string /*param value*/>(
					        commandSpecificFields[i].first,
					        commandSpecificFields[i].second.getValueAsString()));
				else
					commands.back().params_.emplace(
					    std::pair<std::string /*param name*/,
					              std::string /*param value*/>(
					        commandSpecificFields[i].first,
					        commandSpecificFields[i].second.getValue<std::string>()));
			}
		}
	}

	return commands;
}

DEFINE_OTS_CONFIGURATION(IterateConfiguration)
