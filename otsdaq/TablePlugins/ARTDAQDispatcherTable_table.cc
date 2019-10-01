#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/Macros/TablePluginMacros.h"
#include "otsdaq/TablePlugins/ARTDAQDispatcherTable.h"
#include "otsdaq/TablePlugins/XDAQContextTable.h"

#include <stdio.h>
#include <sys/stat.h>  //for mkdir
#include <fstream>     // std::fstream
#include <iostream>

using namespace ots;

#define ARTDAQ_FCL_PATH std::string(__ENV__("USER_DATA")) + "/" + "ARTDAQConfigurations/"
#define ARTDAQ_FILE_PREAMBLE "dispatcher"

// helpers
#define OUT out << tabStr << commentStr
#define PUSHTAB tabStr += "\t"
#define POPTAB tabStr.resize(tabStr.size() - 1)
#define PUSHCOMMENT commentStr += "# "
#define POPCOMMENT commentStr.resize(commentStr.size() - 2)

//========================================================================================================================
ARTDAQDispatcherTable::ARTDAQDispatcherTable(void)
    : ARTDAQTableBase("ARTDAQDispatcherTable")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names used in C++ MUST match the Table INFO  //
	//////////////////////////////////////////////////////////////////////
}

//========================================================================================================================
ARTDAQDispatcherTable::~ARTDAQDispatcherTable(void) {}

//========================================================================================================================
void ARTDAQDispatcherTable::init(ConfigurationManager* configManager)
{
	//	// use isFirstAppInContext to only run once per context, for example to avoid
	//	//	generating files on local disk multiple times.
	//	bool isFirstAppInContext = configManager->isOwnerFirstAppInContext();
	//
	//	//__COUTV__(isFirstAppInContext);
	//	if(!isFirstAppInContext)
	//		return;
	//
	//	// make directory just in case
	//	mkdir((ARTDAQ_FCL_PATH).c_str(), 0755);
	//
	//	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
	//	__COUT__ << configManager->__SELF_NODE__ << std::endl;
	//
	//	const XDAQContextTable* contextConfig =
	//	    configManager->__GET_CONFIG__(XDAQContextTable);
	//	std::vector<const XDAQContextTable::XDAQContext*> dispContexts =
	//	    contextConfig->getDispatcherContexts();
	//
	//	// for each dispatcher context
	//	//	output associated fcl config file
	//	for(auto& dispContext : dispContexts)
	//	{
	//		ConfigurationTree dispConfigNode = contextConfig->getSupervisorConfigNode(
	//		    configManager,
	//		    dispContext->contextUID_,
	//		    dispContext->applications_[0].applicationUID_);
	//
	//		__COUT__ << "Path for this dispatcher config is " << dispContext->contextUID_
	//		         << "/" << dispContext->applications_[0].applicationUID_ << "/"
	//		         << dispConfigNode.getValueAsString() << std::endl;
	//
	//		outputFHICL(
	//		    configManager,
	//		    dispConfigNode,
	//		    contextConfig->getARTDAQAppRank(dispContext->contextUID_),
	//		    contextConfig->getContextAddress(dispContext->contextUID_),
	//		    contextConfig->getARTDAQDataPort(configManager, dispContext->contextUID_),
	//		    contextConfig, 0);
	//	}
}  // end init()

////========================================================================================================================
// void ARTDAQDispatcherTable::outputFHICL(ConfigurationManager*    configManager,
//                                        const ConfigurationTree& dispatcherNode,
//                                        unsigned int             selfRank,
//										const std::string&       selfHost,
//                                        unsigned int             selfPort,
//                                        const XDAQContextTable*  contextConfig,
//                                        size_t                   maxFragmentSizeBytes)
//{
//	ARTDAQTableBase::outputDataReceiverFHICL(configManager,
//	                        dispatcherNode,
//	                        selfRank,
//	                        selfHost,
//	                        selfPort,
//	                        DataReceiverAppType::Dispatcher,
//	                        maxFragmentSizeBytes);
//} //end outputFHICL()

DEFINE_OTS_TABLE(ARTDAQDispatcherTable)
