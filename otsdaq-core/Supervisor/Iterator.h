#ifndef _ots_Iterator_h
#define _ots_Iterator_h


#include "otsdaq-core/ConfigurationPluginDataFormats/IterateConfiguration.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include <mutex>        //for std::mutex
#include <string>

namespace ots
{

class Supervisor;

class Iterator
{
	
public:
    Iterator (Supervisor* supervisor);
    ~Iterator(void);

    void									playIterationPlan			(HttpXmlDocument& xmldoc, const std::string& planName);
    void									pauseIterationPlan			(HttpXmlDocument& xmldoc);
    void									haltIterationPlan			(HttpXmlDocument& xmldoc);
    void									getIterationPlanStatus		(HttpXmlDocument& xmldoc);

    bool									handleCommandRequest		(HttpXmlDocument& xmldoc, const std::string& command, const std::string& parameter);

private:

    static void								IteratorWorkLoop			(Iterator *iterator);
    void									startCommand				(std::vector<IterateConfiguration::Command>& commands, unsigned int commandIndex);
    bool									checkCommand				(std::vector<IterateConfiguration::Command>& commands, unsigned int commandIndex);
    void									startCommandConfigureAlias	(const std::string& systemAlias);
    bool									checkCommandConfigureAlias	();
//	m[COMMAND_BEGIN_LABEL] 				=  "IterationCommandBeginLabelConfiguration";
//	m[COMMAND_CONFIGURE_ACTIVE_GROUP] 	=  ""; //no parameters
//	m[COMMAND_CONFIGURE_ALIAS] 			=  "IterationCommandConfigureAliasConfiguration";
//	m[COMMAND_CONFIGURE_GROUP] 			=  "IterationCommandConfigureGroupConfiguration";
//	m[COMMAND_EXECUTE_FE_MACRO] 		=  "IterationCommandExecuteFEMacroConfiguration";
//	m[COMMAND_EXECUTE_MACRO] 			=  "IterationCommandExecuteMacroConfiguration";
//	m[COMMAND_MODIFY_ACTIVE_GROUP] 		=  "IterationCommandModifyGroupConfiguration";
//	m[COMMAND_REPEAT_LABEL] 			=  "IterationCommandRepeatLabelConfiguration";
//	m[COMMAND_RUN] 						=  "IterationCommandRunConfiguration";

    std::mutex								accessMutex_;
    volatile bool							workloopRunning_;
    volatile bool							activePlanIsRunning_;
    volatile bool							iteratorBusy_;
    volatile bool							commandPlay_, commandPause_, commandHalt_; //commands are set by supervisor thread, and cleared by iterator thread
    std::string								activePlanName_, lastStartedPlanName_, lastFinishedPlanName_;

    Supervisor* 							theSupervisor_;
};

}

#endif
