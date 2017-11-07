#ifndef _ots_Iterator_h
#define _ots_Iterator_h


#include "otsdaq-core/ConfigurationPluginDataFormats/IterateConfiguration.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include <mutex>        //for std::mutex
#include <string>

namespace ots
{

class Supervisor;
class ConfigurationManagerRW;

class Iterator
{
	friend class Supervisor;
	
public:
    Iterator (Supervisor* supervisor);
    ~Iterator(void);

    void									playIterationPlan			(HttpXmlDocument& xmldoc, const std::string& planName);
    void									pauseIterationPlan			(HttpXmlDocument& xmldoc);
    void									haltIterationPlan			(HttpXmlDocument& xmldoc);
    void									getIterationPlanStatus		(HttpXmlDocument& xmldoc);

    bool									handleCommandRequest		(HttpXmlDocument& xmldoc, const std::string& command, const std::string& parameter);

private:

    struct IteratorWorkLoopStruct
	{
    	std::string fsmName_;
    	ConfigurationManagerRW* cfgMgr;
	};

    static void								IteratorWorkLoop			(Iterator *iterator);
    static void								startCommand				(Iterator *iterator, std::vector<IterateConfiguration::Command>& commands, unsigned int& commandIndex);
    static bool								checkCommand				(Iterator *iterator, std::vector<IterateConfiguration::Command>& commands, unsigned int& commandIndex);
    static void								startCommandChooseFSM		(const std::string& fsmName);
    static void								startCommandConfigureAlias	(const std::string& systemAlias);
    static bool								checkCommandConfigureAlias	();

    std::mutex								accessMutex_;
    volatile bool							workloopRunning_;
    volatile bool							activePlanIsRunning_;
    volatile bool							iteratorBusy_;
    volatile bool							commandPlay_, commandPause_, commandHalt_; //commands are set by supervisor thread, and cleared by iterator thread
    std::string								activePlanName_, lastStartedPlanName_, lastFinishedPlanName_;
    volatile unsigned int					activeCommandIndex_;
    volatile time_t							activeCommandStartTime_;

    Supervisor* 							theSupervisor_;
};

}

#endif
