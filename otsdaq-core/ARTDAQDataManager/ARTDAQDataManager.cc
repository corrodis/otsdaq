#include "otsdaq-core/ARTDAQDataManager/ARTDAQDataManager.h"
#include "otsdaq-core/DataProcessorPlugins/ARTDAQConsumer.h"
#include "otsdaq-core/DataProcessorPlugins/ARTDAQProducer.h"

#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"

#include <cassert>
#include <iostream>

using namespace ots;

//========================================================================================================================
ARTDAQDataManager::ARTDAQDataManager(const ConfigurationTree& theXDAQContextConfigTree,
                                     const std::string&       supervisorConfigurationPath)
    : DataManager(theXDAQContextConfigTree, supervisorConfigurationPath)
{
	INIT_MF("BoardReaderDataManager");
	__CFG_COUT__ << "Constructor." << __E__;

	std::string name = "BoardReader";

	__CFG_MCOUT__("artdaq version " <<
	              //    mf::LogDebug(supervisorApplicationUID_) << " artdaq version " <<
	              artdaq::GetPackageBuildInfo::getPackageBuildInfo().getPackageVersion()
	                                << ", built "
	                                << artdaq::GetPackageBuildInfo::getPackageBuildInfo()
	                                       .getBuildTimestamp());

	INIT_MF((name + "App").c_str());

	// artdaq::configureMessageFacility("boardreader");
	// artdaq::configureMessageFacility(name.c_str());

	__CFG_COUT__ << "MF initialized" << std::endl;

	rank_ = Configurable::getApplicationLID();

	__CFG_COUTV__(rank_);

	unsigned short port = 5100;

	// artdaq::setMsgFacAppName(name, port);

	// create the BoardReaderApp
	//	  artdaq::BoardReaderApp br_app(local_group_comm, name);
	__CFG_COUT__ << "END" << std::endl;

	__CFG_COUT__ << "Constructed." << __E__;
}

//========================================================================================================================
ARTDAQDataManager::~ARTDAQDataManager(void) {}

//========================================================================================================================
void ARTDAQDataManager::configure(void)
{
	__CFG_COUT__ << "ARTDAQDataManager configuring..." << std::endl;

	DataManager::configure();

	// find the ARTDAQ processor (can only be 1)
	//	and initialize with rank info

	__CFG_COUT__ << "ARTDAQDataManager DataManager configured now pass the MPI stuff"
	             << std::endl;
	for(auto it = DataManager::buffers_.begin(); it != DataManager::buffers_.end(); it++)
		for(auto& consumer : it->second.consumers_)
			if(dynamic_cast<ARTDAQConsumer*>(consumer))
			{
				dynamic_cast<ARTDAQConsumer*>(consumer)->initLocalGroup(rank_);
				return;  // There can only be 1 ARTDAQConsumer for each
				         // ARTDAQDataManager!!!!!!!
			}

	__CFG_SS__ << "There was no ARTDAQ Consumer found on a buffer!" << std::endl;
	__CFG_COUT__ << ss.str();

	__CFG_COUT__ << "Looking for an ARTDAQ Producer..." << std::endl;

	for(auto it = DataManager::buffers_.begin(); it != DataManager::buffers_.end(); it++)
		for(auto& producer : it->second.producers_)
			if(dynamic_cast<ARTDAQProducer*>(producer))
			{
				dynamic_cast<ARTDAQProducer*>(producer)->initLocalGroup(rank_);
				return;  // There can only be 1 ARTDAQProducer for each
				         // ARTDAQDataManager!!!!!!!
			}

	__CFG_COUT__ << "No ARTDAQ Producers found either... so error!" << std::endl;

	__CFG_COUT_ERR__ << ss.str();
	__CFG_SS_THROW__;
}  // end configure()

//========================================================================================================================
void ARTDAQDataManager::stop(void) { DataManager::stop(); }  // end stop()
