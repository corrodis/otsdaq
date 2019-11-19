#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/Macros/TablePluginMacros.h"
#include "otsdaq/TablePlugins/ARTDAQDataLoggerTable.h"
#include "otsdaq/TablePlugins/XDAQContextTable.h"

#include <stdio.h>
#include <sys/stat.h>  //for mkdir
#include <fstream>     // std::fstream
#include <iostream>

using namespace ots;

//========================================================================================================================
ARTDAQDataLoggerTable::ARTDAQDataLoggerTable(void) : ARTDAQTableBase("ARTDAQDataLoggerTable")
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
	// use isFirstAppInContext to only run once per context, for example to avoid
	//	generating files on local disk multiple times.
	bool isFirstAppInContext = configManager->isOwnerFirstAppInContext();

	//__COUTV__(isFirstAppInContext);
	if(!isFirstAppInContext)
		return;

	// make directory just in case
	mkdir((ARTDAQTableBase::ARTDAQ_FCL_PATH).c_str(), 0755);

	//	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << __E__;
	//	__COUT__ << configManager->__SELF_NODE__ << __E__;

	// handle fcl file generation, wherever the level of this table

	auto dataloggers = configManager->__SELF_NODE__.getChildren(
	    /*default filterMap*/ std::map<std::string /*relative-path*/, std::string /*value*/>(),
	    /*default byPriority*/ false,
	    /*TRUE! onlyStatusTrue*/ true);

	for(auto& datalogger : dataloggers)
	{
		ARTDAQTableBase::outputDataReceiverFHICL(datalogger.second, ARTDAQTableBase::ARTDAQAppType::DataLogger);

		ARTDAQTableBase::flattenFHICL(ARTDAQAppType::DataLogger, datalogger.second.getValue());
	}
}

DEFINE_OTS_TABLE(ARTDAQDataLoggerTable)
