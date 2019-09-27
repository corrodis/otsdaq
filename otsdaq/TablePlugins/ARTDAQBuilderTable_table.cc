

#include "otsdaq/Macros/TablePluginMacros.h"
#include "otsdaq/TablePlugins/ARTDAQBuilderTable.h"
#include "otsdaq/TablePlugins/XDAQContextTable.h"

#include <stdio.h>
#include <sys/stat.h>  //for mkdir
#include <fstream>     // std::fstream
#include <iostream>

using namespace ots;

#define ARTDAQ_FILE_PREAMBLE "builder"

//========================================================================================================================
ARTDAQBuilderTable::ARTDAQBuilderTable(void) : ARTDAQTableBase("ARTDAQBuilderTable")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names used in C++ MUST match the Table INFO  //
	//////////////////////////////////////////////////////////////////////
}

//========================================================================================================================
ARTDAQBuilderTable::~ARTDAQBuilderTable(void) {}

//========================================================================================================================
void ARTDAQBuilderTable::init(ConfigurationManager* configManager)
{
	//	// use isFirstAppInContext to only run once per context, for example to avoid
	//	//	generating files on local disk multiple times.
	//	bool isFirstAppInContext = configManager->isOwnerFirstAppInContext();
	//
	//	//__COUTV__(isFirstAppInContext);
	//	if(!isFirstAppInContext)
	//		return;
	//
	//	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << __E__;
	//	__COUT__ << configManager->__SELF_NODE__ << __E__;
	//
	//	const XDAQContextTable* contextConfig =
	//	    configManager->__GET_CONFIG__(XDAQContextTable);
	//
	//	std::vector<const XDAQContextTable::XDAQContext*> builderContexts =
	//	    contextConfig->getEventBuilderContexts();
	//
	//	// for each builder context
	//	//	output associated fcl config file
	//	for(auto& builderContext : builderContexts)
	//	{
	//		ConfigurationTree builderAppNode = contextConfig->getApplicationNode(
	//		    configManager,
	//		    builderContext->contextUID_,
	//		    builderContext->applications_[0].applicationUID_);
	//		ConfigurationTree builderConfigNode = contextConfig->getSupervisorConfigNode(
	//		    configManager,
	//		    builderContext->contextUID_,
	//		    builderContext->applications_[0].applicationUID_);
	//
	//		__COUT__ << "Path for this EventBuilder config is " <<
	//builderContext->contextUID_
	//		         << "/" << builderContext->applications_[0].applicationUID_ << "/"
	//		         << builderConfigNode.getValueAsString() << __E__;
	//
	//		outputFHICL(
	//		    configManager,
	//		    builderConfigNode,
	//		    contextConfig->getARTDAQAppRank(builderContext->contextUID_),
	//		    contextConfig->getContextAddress(builderContext->contextUID_),
	//		    contextConfig->getARTDAQDataPort(configManager,
	//builderContext->contextUID_), 		    contextConfig, 0);
	//
	//		flattenFHICL(ARTDAQ_FILE_PREAMBLE, builderConfigNode.getValue());
	//	}
}  // end init()
//
////========================================================================================================================
// void ARTDAQBuilderTable::outputFHICL(
//                                     const ConfigurationTree& builderNode,
//                                     unsigned int             selfRank,
//									 const std::string&       selfHost,
//                                     unsigned int             selfPort,
//                                     size_t                   maxFragmentSizeBytes)
//{
//	ARTDAQTableBase::outputDataReceiverFHICL(
//	                        builderNode,
//	                        selfRank,
//	                        selfHost,
//	                        selfPort,
//							ARTDAQTableBase::DataReceiverAppType::EventBuilder,
//	                        maxFragmentSizeBytes);
//}  // end outputFHICL()

DEFINE_OTS_TABLE(ARTDAQBuilderTable)
