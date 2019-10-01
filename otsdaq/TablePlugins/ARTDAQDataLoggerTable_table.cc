#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/Macros/TablePluginMacros.h"
#include "otsdaq/TablePlugins/ARTDAQDataLoggerTable.h"
#include "otsdaq/TablePlugins/XDAQContextTable.h"

#include <stdio.h>
#include <sys/stat.h>  //for mkdir
#include <fstream>     // std::fstream
#include <iostream>

using namespace ots;

#define ARTDAQ_FCL_PATH std::string(__ENV__("USER_DATA")) + "/" + "ARTDAQConfigurations/"
#define ARTDAQ_FILE_PREAMBLE "datalogger"

// helpers
#define OUT out << tabStr << commentStr
#define PUSHTAB tabStr += "\t"
#define POPTAB tabStr.resize(tabStr.size() - 1)
#define PUSHCOMMENT commentStr += "# "
#define POPCOMMENT commentStr.resize(commentStr.size() - 2)

//========================================================================================================================
ARTDAQDataLoggerTable::ARTDAQDataLoggerTable(void)
    : ARTDAQTableBase("ARTDAQDataLoggerTable")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names used in C++ MUST match the Table INFO  //
	//////////////////////////////////////////////////////////////////////
}

//========================================================================================================================
ARTDAQDataLoggerTable::~ARTDAQDataLoggerTable(void) {}

//========================================================================================================================
void ARTDAQDataLoggerTable::init(ConfigurationManager* configManager)
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
	//	std::vector<const XDAQContextTable::XDAQContext*> loggerContexts =
	//	    contextConfig->getDataLoggerContexts();
	//
	//	// for each datalogger context
	//	//	output associated fcl config file
	//	for(auto& loggerContext : loggerContexts)
	//	{
	//		ConfigurationTree aggConfigNode = contextConfig->getSupervisorConfigNode(
	//		    configManager,
	//		    loggerContext->contextUID_,
	//		    loggerContext->applications_[0].applicationUID_);
	//
	//		__COUT__ << "Path for this DataLogger config is " <<
	//loggerContext->contextUID_
	//		         << "/" << loggerContext->applications_[0].applicationUID_ << "/"
	//		         << aggConfigNode.getValueAsString() << std::endl;
	//
	//		outputFHICL(
	//		    configManager,
	//		    aggConfigNode,
	//		    contextConfig->getARTDAQAppRank(loggerContext->contextUID_),
	//		    contextConfig->getContextAddress(loggerContext->contextUID_),
	//		    contextConfig->getARTDAQDataPort(configManager,
	//loggerContext->contextUID_), 		    contextConfig, 0);
	//	}
}

////========================================================================================================================
// void ARTDAQDataLoggerTable::outputFHICL(ConfigurationManager*    configManager,
//                                        const ConfigurationTree& dataLoggerNode,
//                                        unsigned int             selfRank,
//                                        std::string              selfHost,
//                                        unsigned int             selfPort,
//                                        const XDAQContextTable*  contextConfig,
//                                        size_t                   maxFragmentSizeBytes)
//{
//	outputDataReceiverFHICL(configManager,
//	                        dataLoggerNode,
//	                        selfRank,
//	                        selfHost,
//	                        selfPort,
//	                        DataReceiverAppType::DataLogger,
//	                        maxFragmentSizeBytes);
//}

DEFINE_OTS_TABLE(ARTDAQDataLoggerTable)
