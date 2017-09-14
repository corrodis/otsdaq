#include "otsdaq-core/ARTDAQDataManager/ARTDAQDataManager.h"
#include "otsdaq-core/DataProcessorPlugins/ARTDAQConsumer.h"

#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"

#include <iostream>
#include <cassert>

using namespace ots;


//========================================================================================================================
ARTDAQDataManager::ARTDAQDataManager(const ConfigurationTree& theXDAQContextConfigTree, const std::string& supervisorConfigurationPath)
: DataManager (theXDAQContextConfigTree, supervisorConfigurationPath)
{
	INIT_MF("BoardReaderDataManager");
	__MOUT__ << "Begin!" << std::endl;
	__MOUT__ << "Begin!" << std::endl;
	__MOUT__ << "Begin!" << std::endl;
	__MOUT__ << "Begin!" << std::endl;
	__MOUT__ << "Begin!" << std::endl;
	mf::LogDebug("BoardReader") << "artdaq version " <<
	//    mf::LogDebug(supervisorApplicationUID_) << " artdaq version " <<
				artdaq::GetPackageBuildInfo::getPackageBuildInfo().getPackageVersion()
				<< ", built " <<
				artdaq::GetPackageBuildInfo::getPackageBuildInfo().getBuildTimestamp();
	theMPIProcess_.init("BoardReader", artdaq::TaskType::BoardReaderTask);
	__MOUT__ << "Initialized!" << std::endl;
	__MOUT__ << "Initialized!" << std::endl;
	__MOUT__ << "Initialized!" << std::endl;
	__MOUT__ << "Initialized!" << std::endl;
	__MOUT__ << "Initialized!" << std::endl;
}

//========================================================================================================================
ARTDAQDataManager::~ARTDAQDataManager(void)
{}

//========================================================================================================================
void ARTDAQDataManager::configure(void)
{
	__MOUT__ << "ARTDAQDataManager configuring..." << std::endl;

	DataManager::configure();

	__MOUT__ << "ARTDAQDataManager DataManager configured now pass the MPI stuff" << std::endl;
	for(auto it=DataManager::buffers_.begin(); it!=DataManager::buffers_.end(); it++)
		for(auto& itc: it->second.consumers_)
			if(dynamic_cast<ARTDAQConsumer*>(itc.get()))
			{
				dynamic_cast<ARTDAQConsumer*>(itc.get())->initLocalGroup(theMPIProcess_.getRank());
				return;//There can only be 1 ARTDAQConsumer for each ARTDAQDataManager!!!!!!!
			}

	__SS__ << "There was no ARTDAQ Consumer found on a buffer!" << std::endl;
	__MOUT_ERR__ << ss.str();
	throw std::runtime_error(ss.str());
}


//========================================================================================================================
void ARTDAQDataManager::stop(void)
{
	DataManager::stop();
}
