#include "otsdaq-core/ARTDAQDataManager/ARTDAQDataManager.h"
#include "otsdaq-core/DataProcessorPlugins/ARTDAQConsumer.h"
#include "otsdaq-core/DataProcessorPlugins/ARTDAQProducer.h"

#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"

#include <iostream>
#include <cassert>

using namespace ots;


//========================================================================================================================
ARTDAQDataManager::ARTDAQDataManager(const ConfigurationTree& theXDAQContextConfigTree, const std::string& supervisorConfigurationPath)
: DataManager (theXDAQContextConfigTree, supervisorConfigurationPath)
{
	INIT_MF("BoardReaderDataManager");
	__COUT__ << "Begin!" << std::endl;
	__COUT__ << "Begin!" << std::endl;
	__COUT__ << "Begin!" << std::endl;
	__COUT__ << "Begin!" << std::endl;
	__COUT__ << "Begin!" << std::endl;
	


	std::string name = "BoardReader";
	
	__CFG_MOUT__ << "artdaq version " <<
	//    mf::LogDebug(supervisorApplicationUID_) << " artdaq version " <<
				artdaq::GetPackageBuildInfo::getPackageBuildInfo().getPackageVersion()
				<< ", built " <<
				artdaq::GetPackageBuildInfo::getPackageBuildInfo().getBuildTimestamp();
	
	INIT_MF((name + "App").c_str());
	
	//artdaq::configureMessageFacility("boardreader");
	//artdaq::configureMessageFacility(name.c_str());

	__COUT__ << "MF initialized" << std::endl;

	rank_ = Configurable::getApplicationLID();

	__COUTV__(rank_);
	
	unsigned short port = 5100;

	//artdaq::setMsgFacAppName(name, port);

	// create the BoardReaderApp
	//	  artdaq::BoardReaderApp br_app(local_group_comm, name);
	__COUT__ << "END" << std::endl;


	__COUT__ << "Initialized!" << std::endl;
	__COUT__ << "Initialized!" << std::endl;
	__COUT__ << "Initialized!" << std::endl;
	__COUT__ << "Initialized!" << std::endl;
	__COUT__ << "Initialized!" << std::endl;
}

//========================================================================================================================
ARTDAQDataManager::~ARTDAQDataManager(void)
{}

//========================================================================================================================
void ARTDAQDataManager::configure(void)
{
	__COUT__ << "ARTDAQDataManager configuring..." << std::endl;

	DataManager::configure();

	__COUT__ << "ARTDAQDataManager DataManager configured now pass the MPI stuff" << std::endl;
	for(auto it=DataManager::buffers_.begin(); it!=DataManager::buffers_.end(); it++)
		for(auto& itc: it->second.consumers_)
			if(dynamic_cast<ARTDAQConsumer*>(itc.get()))
			{
				dynamic_cast<ARTDAQConsumer*>(itc.get())->initLocalGroup(rank_);
				return;//There can only be 1 ARTDAQConsumer for each ARTDAQDataManager!!!!!!!
			}

	__SS__ << "There was no ARTDAQ Consumer found on a buffer!" << std::endl;
	__COUT__ << ss.str();

	__COUT__ << "Looking for an ARTDAQ Producer..." << std::endl;

	for(auto it=DataManager::buffers_.begin(); it!=DataManager::buffers_.end(); it++)
		for(auto& itc: it->second.producers_)
			if(dynamic_cast<ARTDAQProducer*>(itc.get()))
			{
				dynamic_cast<ARTDAQProducer*>(itc.get())->initLocalGroup(rank_);
				return;//There can only be 1 ARTDAQProducer for each ARTDAQDataManager!!!!!!!
			}

	__COUT__ << "No ARTDAQ Producers found either... so error!" << std::endl;

	__COUT_ERR__ << ss.str();
	throw std::runtime_error(ss.str());
}


//========================================================================================================================
void ARTDAQDataManager::stop(void)
{
	DataManager::stop();
}
