#include "artdaq/Application/Commandable.hh"
#include "fhiclcpp/make_ParameterSet.h"
#include "otsdaq-core/DataManager/DataManager.h"
#include "otsdaq-core/DataManager/DataManagerSingleton.h"
#include "otsdaq-core/DataProcessorPlugins/ARTDAQConsumer.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/Macros/ProcessorPluginMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>

using namespace ots;

#define ARTDAQ_FCL_PATH std::string(getenv("USER_DATA")) + "/" + "ARTDAQConfigurations/"
#define ARTDAQ_FILE_PREAMBLE "boardReader"

//========================================================================================================================
ARTDAQConsumer::ARTDAQConsumer(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& configurationPath)
    : WorkLoop(processorUID)
    , DataConsumer(supervisorApplicationUID, bufferUID, processorUID, LowConsumerPriority)
    , Configurable(theXDAQContextConfigTree, configurationPath)
{
	__COUT__ << "ARTDAQ CONSUMER CONSTRUCTOR!!!" << std::endl;
	//__COUT__ << "Configuration string:-" << theXDAQContextConfigTree.getNode(configurationPath).getNode("ConfigurationString").getValue<std::string>() << "-" << std::endl;

	std::string filename = ARTDAQ_FCL_PATH + ARTDAQ_FILE_PREAMBLE + "-";
	std::string uid      = theXDAQContextConfigTree.getNode(configurationPath).getValue();

	__COUT__ << "uid: " << uid << std::endl;
	for (unsigned int i = 0; i < uid.size(); ++i)
		if ((uid[i] >= 'a' && uid[i] <= 'z') ||
		    (uid[i] >= 'A' && uid[i] <= 'Z') ||
		    (uid[i] >= '0' && uid[i] <= '9'))  //only allow alpha numeric in file name
			filename += uid[i];
	filename += ".fcl";

	__COUT__ << std::endl;
	__COUT__ << std::endl;
	__COUT__ << "filename: " << filename << std::endl;

	std::string fileFclString;
	{
		std::ifstream in(filename, std::ios::in | std::ios::binary);
		if (in)
		{
			std::string contents;
			in.seekg(0, std::ios::end);
			fileFclString.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&fileFclString[0], fileFclString.size());
			in.close();
		}
	}
	//__COUT__ << fileFclString << std::endl;

	//find fragment_receiver {
	// and insert e.g.,
	//	SupervisorApplicationUID:"ARTDataManager0"
	//	BufferUID:"ART_S0_DM0_DataBuffer0"
	//	ProcessorUID:"ART_S0_DM0_DB0_ARTConsumer0"
	size_t fcli = fileFclString.find("fragment_receiver: {") +
	              +strlen("fragment_receiver: {");
	if (fcli == std::string::npos)
	{
		__SS__ << "Could not find 'fragment_receiver: {' in Board Reader fcl string!" << std::endl;
		__COUT__ << "\n"
		         << ss.str();
		__SS_THROW__;
	}

	//get the parent IDs from configurationPath
	__COUT__ << "configurationPath " << configurationPath << std::endl;

	std::string  consumerID, bufferID, appID;
	unsigned int backSteps;  //at 2, 4, and 7 are the important parent IDs
	size_t       backi = -1, backj;
	backSteps          = 7;
	for (unsigned int i = 0; i < backSteps; i++)
	{
		//__COUT__ << "backsteps: " << i+1 << std::endl;

		backj = backi;
		backi = configurationPath.rfind('/', backi - 1);

		//__COUT__ << "backi:" << backi << " backj:" << backj << std::endl;
		//__COUT__ << "substr: " << configurationPath.substr(backi+1,backj-backi-1) << std::endl;

		if (i + 1 == 2)
			consumerID = configurationPath.substr(backi + 1, backj - backi - 1);
		else if (i + 1 == 4)
			bufferID = configurationPath.substr(backi + 1, backj - backi - 1);
		else if (i + 1 == 7)
			appID = configurationPath.substr(backi + 1, backj - backi - 1);
	}

	//insert parent IDs into fcl string
	fileFclString = fileFclString.substr(0, fcli) + "\n\t\t" +
	                "SupervisorApplicationUID: \"" + appID + "\"\n\t\t" +
	                "BufferUID: \"" + bufferID + "\"\n\t\t" +
	                "ProcessorUID: \"" + consumerID + "\"\n" +
	                fileFclString.substr(fcli);

	__COUT__ << fileFclString << std::endl;

	fhicl::make_ParameterSet(fileFclString, fhiclConfiguration_);

	//fhicl::make_ParameterSet(theXDAQContextConfigTree.getNode(configurationPath).getNode("ConfigurationString").getValue<std::string>(), fhiclConfiguration_);
}

//========================================================================================================================
//ARTDAQConsumer::ARTDAQConsumer(std::string interfaceID, MPI_Comm local_group_comm, std::string name)
//:FEVInterface     (feId, 0)
//,local_group_comm_(local_group_comm)
//,name_            (name)
//{}

//========================================================================================================================
ARTDAQConsumer::~ARTDAQConsumer(void)
{
	halt();
	__COUT__ << "DONE DELETING!" << std::endl;
}

//========================================================================================================================
void ARTDAQConsumer::initLocalGroup(int rank)
{
	name_ = "BoardReader_" + DataConsumer::processorUID_;
	configure(rank);
}

#define ARTDAQ_FCL_PATH std::string(getenv("USER_DATA")) + "/" + "ARTDAQConfigurations/"
#define ARTDAQ_FILE_PREAMBLE "boardReader"

//========================================================================================================================
void ARTDAQConsumer::configure(int rank)
{
	std::cout << __COUT_HDR_FL__ << "\tConfigure" << std::endl;

	report_string_           = "";
	external_request_status_ = true;

	// in the following block, we first destroy the existing BoardReader
	// instance, then create a new one.  Doing it in one step does not
	// produce the desired result since that creates a new instance and
	// then deletes the old one, and we need the opposite order.
	fragment_receiver_ptr_.reset(nullptr);
	std::cout << __COUT_HDR_FL__ << "\tNew core" << std::endl;
	my_rank  = rank;
	app_name = name_;
	fragment_receiver_ptr_.reset(new artdaq::BoardReaderApp());
	//FIXME These are passed as parameters
	uint64_t timeout = 45;
	//uint64_t timestamp = 184467440737095516;
	uint64_t timestamp = 184467440737095516;
	std::cout << __COUT_HDR_FL__ << "\tInitialize: " << std::endl;  //<< fhiclConfiguration_.to_string() << std::endl;
	external_request_status_ = fragment_receiver_ptr_->initialize(fhiclConfiguration_, timeout, timestamp);
	std::cout << __COUT_HDR_FL__ << "\tDone Initialize" << std::endl;
	if (!external_request_status_)
	{
		report_string_ = "Error initializing ";
		report_string_.append(name_ + " ");
		report_string_.append("with ParameterSet = \"" + fhiclConfiguration_.to_string() + "\".");
	}
	std::cout << __COUT_HDR_FL__ << "\tDone Configure" << std::endl;
}

//========================================================================================================================
void ARTDAQConsumer::halt(void)
{
	std::cout << __COUT_HDR_FL__ << "\tHalt" << std::endl;
	//FIXME These are passed as parameters
	uint64_t timeout = 45;
	//uint64_t timestamp = 184467440737095516;
	report_string_           = "";
	external_request_status_ = fragment_receiver_ptr_->shutdown(timeout);
	if (!external_request_status_)
	{
		report_string_ = "Error shutting down ";
		report_string_.append(name_ + ".");
	}
}

//========================================================================================================================
void ARTDAQConsumer::pauseProcessingData(void)
{
	std::cout << __COUT_HDR_FL__ << "\tPause" << std::endl;
	//FIXME These are passed as parameters
	uint64_t timeout         = 45;
	uint64_t timestamp       = 184467440737095516;
	report_string_           = "";
	external_request_status_ = fragment_receiver_ptr_->pause(timeout, timestamp);
	if (!external_request_status_)
	{
		report_string_ = "Error pausing ";
		report_string_.append(name_ + ".");
	}
}

//========================================================================================================================
void ARTDAQConsumer::resumeProcessingData(void)
{
	std::cout << __COUT_HDR_FL__ << "\tResume" << std::endl;
	//FIXME These are passed as parameters
	uint64_t timeout         = 45;
	uint64_t timestamp       = 184467440737095516;
	report_string_           = "";
	external_request_status_ = fragment_receiver_ptr_->resume(timeout, timestamp);
	if (!external_request_status_)
	{
		report_string_ = "Error resuming ";
		report_string_.append(name_ + ".");
	}
}

//========================================================================================================================
void ARTDAQConsumer::startProcessingData(std::string runNumber)
{
	std::cout << __COUT_HDR_FL__ << "\tStart" << std::endl;

	art::RunID runId((art::RunNumber_t)boost::lexical_cast<art::RunNumber_t>(runNumber));

	//FIXME These are passed as parameters
	uint64_t timeout   = 45;
	uint64_t timestamp = 184467440737095516;

	report_string_ = "";
	std::cout << __COUT_HDR_FL__ << "\tStart run: " << runId << std::endl;
	external_request_status_ = fragment_receiver_ptr_->start(runId, timeout, timestamp);
	std::cout << __COUT_HDR_FL__ << "\tStart already crashed " << std::endl;
	if (!external_request_status_)
	{
		report_string_ = "Error starting ";
		report_string_.append(name_ + " ");
		report_string_.append("for run number ");
		report_string_.append(boost::lexical_cast<std::string>(runId.run()));
		report_string_.append(", timeout ");
		report_string_.append(boost::lexical_cast<std::string>(timeout));
		report_string_.append(", timestamp ");
		report_string_.append(boost::lexical_cast<std::string>(timestamp));
		report_string_.append(".");
	}

	std::cout << __COUT_HDR_FL__ << "STARTING BOARD READER THREAD" << std::endl;
}

//========================================================================================================================
void ARTDAQConsumer::stopProcessingData(void)
{
	std::cout << __COUT_HDR_FL__ << "\tStop" << std::endl;
	//FIXME These are passed as parameters
	uint64_t timeout   = 45;
	uint64_t timestamp = 184467440737095516;
	report_string_     = "";

	auto sts = fragment_receiver_ptr_->status();
	if (sts == "Ready") return;  // Already stopped/never started

	external_request_status_ = fragment_receiver_ptr_->stop(timeout, timestamp);
	if (!external_request_status_)
	{
		report_string_ = "Error stopping ";
		report_string_.append(name_ + ".");
		//return false;
	}
}

DEFINE_OTS_PROCESSOR(ARTDAQConsumer)
