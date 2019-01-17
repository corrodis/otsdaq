#include "otsdaq/DataLoggerApp/DataLoggerApp.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"

#include <toolbox/fsm/FailedEvent.h>

#include <xdaq/NamespaceURI.h>
#include <xoap/Method.h>

#include "artdaq/DAQdata/Globals.hh"
#include <memory>
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "cetlib/exception.h"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "fhiclcpp/make_ParameterSet.h"

#include <fstream>
#include <iostream>
#include <cassert>
#include <boost/exception/all.hpp>

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(DataLoggerApp)

//========================================================================================================================
DataLoggerApp::DataLoggerApp(xdaq::ApplicationStub * s) 
: xdaq::Application           (s)
, SOAPMessenger               (this)
, stateMachineWorkLoopManager_(toolbox::task::bind(this, &DataLoggerApp::stateMachineThread, "StateMachine"))
, stateMachineSemaphore_      (toolbox::BSem::FULL)
, theConfigurationManager_    (new ConfigurationManager)//(Singleton<ConfigurationManager>::getInstance()) //I always load the full config but if I want to load a partial configuration (new ConfigurationManager)
, XDAQContextConfigurationName_(theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getConfigurationName())
, supervisorConfigurationPath_ ("INITIALIZED INSIDE THE CONTRUCTOR BECAUSE IT NEEDS supervisorContextUID_ and supervisorApplicationUID_")
, supervisorContextUID_        ("INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
, supervisorApplicationUID_    ("INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
{

	INIT_MF("DataLoggerApp");
    xgi::bind (this, &DataLoggerApp::Default,                "Default" );
    xgi::bind (this, &DataLoggerApp::stateMachineXgiHandler, "StateMachineXgiHandler");

    xoap::bind(this, &DataLoggerApp::stateMachineStateRequest, 			"StateMachineStateRequest", XDAQ_NS_URI );
    xoap::bind(this, &DataLoggerApp::stateMachineErrorMessageRequest,  	"StateMachineErrorMessageRequest",  XDAQ_NS_URI );

	try
	{
		supervisorContextUID_ = theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getContextUID(getApplicationContext()->getContextDescriptor()->getURL());
	}
	catch(...)
	{
		__COUT_ERR__ << "XDAQ Supervisor could not access it's configuration through the Configuration Context Group." <<
				" The XDAQContextConfigurationName = " << XDAQContextConfigurationName_ <<
				". The supervisorApplicationUID = " << supervisorApplicationUID_ << std::endl;
		throw;
	}
	try
	{
	  supervisorApplicationUID_ = theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getApplicationUID
			(
					getApplicationContext()->getContextDescriptor()->getURL(),
					getApplicationDescriptor()->getLocalId()
			);
	}
	catch(...)
	{
		__COUT_ERR__ << "XDAQ Supervisor could not access it's configuration through the Configuration Application Group."
				<< " The supervisorApplicationUID = " << supervisorApplicationUID_ << std::endl;
		throw;
	}
	supervisorConfigurationPath_  = "/" + supervisorContextUID_ + "/LinkToApplicationConfiguration/" + supervisorApplicationUID_ + "/LinkToSupervisorConfiguration";

	setStateMachineName(supervisorApplicationUID_);
}

//========================================================================================================================
DataLoggerApp::~DataLoggerApp(void)
{
    destroy();
}
//========================================================================================================================
void DataLoggerApp::init(void)
{
    std::cout << __COUT_HDR_FL__ << "ARTDAQDataLogger SUPERVISOR INIT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	allSupervisorInfo_.init(getApplicationContext());
    artdaq::configureMessageFacility("DataLogger");

    // initialization

    std::cout << __COUT_HDR_FL__ << "ARTDAQDataLogger SUPERVISOR INIT4!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::string    name = "DataLogger";
    unsigned short port = 5300;
//    artdaq::setMsgFacAppName(supervisorApplicationUID_, port);
    artdaq::setMsgFacAppName(name, port);
//    mf::LogDebug(supervisorApplicationUID_) << "artdaq version " <<
    TLOG(TLVL_DEBUG, name + "Supervisor") << "artdaq version " <<
    artdaq::GetPackageBuildInfo::getPackageBuildInfo().getPackageVersion()
    << ", built " <<
    artdaq::GetPackageBuildInfo::getPackageBuildInfo().getBuildTimestamp();

    // create the DataLoggerInterface
	app_name = name;
	my_rank = this->getApplicationDescriptor()->getLocalId();
	theDataLoggerInterface_.reset( new artdaq::DataLoggerApp());
}

//========================================================================================================================
void DataLoggerApp::destroy(void)
{
  theDataLoggerInterface_.reset(nullptr);
}

//========================================================================================================================
void DataLoggerApp::Default(xgi::Input * in, xgi::Output * out )
{
    
    *out << "<!DOCTYPE HTML><html lang='en'><frameset col='100%' row='100%'><frame src='/WebPath/html/DataLoggerApp.html?urn=" <<
    		this->getApplicationDescriptor()->getLocalId() << "'></frameset></html>";
}

//========================================================================================================================
void DataLoggerApp::stateMachineXgiHandler(xgi::Input * in, xgi::Output * out )
{}

//========================================================================================================================
void DataLoggerApp::stateMachineResultXgiHandler(xgi::Input* in, xgi::Output* out ) 
{}

//========================================================================================================================
xoap::MessageReference DataLoggerApp::stateMachineXoapHandler(xoap::MessageReference message )
{
    std::cout << __COUT_HDR_FL__ << "Soap Handler!" << std::endl;
    stateMachineWorkLoopManager_.removeProcessedRequests();
    stateMachineWorkLoopManager_.processRequest(message);
    std::cout << __COUT_HDR_FL__ << "Done - Soap Handler!" << std::endl;
    return message;
}

//========================================================================================================================
xoap::MessageReference DataLoggerApp::stateMachineResultXoapHandler(xoap::MessageReference message )
{
    std::cout << __COUT_HDR_FL__ << "Soap Handler!" << std::endl;
    //stateMachineWorkLoopManager_.removeProcessedRequests();
    //stateMachineWorkLoopManager_.processRequest(message);
    std::cout << __COUT_HDR_FL__ << "Done - Soap Handler!" << std::endl;
    return message;
}

//========================================================================================================================
bool DataLoggerApp::stateMachineThread(toolbox::task::WorkLoop* workLoop)
{
    stateMachineSemaphore_.take();
    std::cout << __COUT_HDR_FL__ << "Re-sending message..." << SOAPUtilities::translate(stateMachineWorkLoopManager_.getMessage(workLoop)).getCommand() << std::endl;
    std::string reply = send(this->getApplicationDescriptor(),stateMachineWorkLoopManager_.getMessage(workLoop));
    stateMachineWorkLoopManager_.report(workLoop, reply, 100, true);
    std::cout << __COUT_HDR_FL__ << "Done with message" << std::endl;
    stateMachineSemaphore_.give();
    return false;//execute once and automatically remove the workloop so in WorkLoopManager the try workLoop->remove(job_) could be commented out
    //return true;//go on and then you must do the workLoop->remove(job_) in WorkLoopManager
}

//========================================================================================================================
xoap::MessageReference DataLoggerApp::stateMachineStateRequest(xoap::MessageReference message) 
{
    std::cout << __COUT_HDR_FL__ << theStateMachine_.getCurrentStateName() << std::endl;
    return SOAPUtilities::makeSOAPMessageReference(theStateMachine_.getCurrentStateName());
}

//========================================================================================================================
xoap::MessageReference DataLoggerApp::stateMachineErrorMessageRequest(xoap::MessageReference message)
{
	__COUT__<< "theStateMachine_.getErrorMessage() = " << theStateMachine_.getErrorMessage() << std::endl;

	SOAPParameters retParameters;
	retParameters.addParameter("ErrorMessage",theStateMachine_.getErrorMessage());
	return SOAPUtilities::makeSOAPMessageReference("stateMachineErrorMessageRequestReply",retParameters);
}

//========================================================================================================================
void DataLoggerApp::stateInitial(toolbox::fsm::FiniteStateMachine& fsm) 
{
    
}

//========================================================================================================================
void DataLoggerApp::stateHalted(toolbox::fsm::FiniteStateMachine& fsm)
{
    
}

//========================================================================================================================
void DataLoggerApp::stateRunning(toolbox::fsm::FiniteStateMachine& fsm)
{
    
}

//========================================================================================================================
void DataLoggerApp::stateConfigured(toolbox::fsm::FiniteStateMachine& fsm)
{
    
}

//========================================================================================================================
void DataLoggerApp::statePaused(toolbox::fsm::FiniteStateMachine& fsm)
{
    
}

//========================================================================================================================
void DataLoggerApp::inError (toolbox::fsm::FiniteStateMachine & fsm)
{
    std::cout << __COUT_HDR_FL__ << "Fsm current state: " << theStateMachine_.getCurrentStateName()<< std::endl;
    //rcmsStateNotifier_.stateChanged("Error", "");
}

//========================================================================================================================
void DataLoggerApp::enteringError (toolbox::Event::Reference e) 
{
    std::cout << __COUT_HDR_FL__ << "Fsm current state: " << theStateMachine_.getCurrentStateName()<< std::endl;
    toolbox::fsm::FailedEvent& failedEvent = dynamic_cast<toolbox::fsm::FailedEvent&>(*e);
    std::ostringstream error;
    error << "Failure performing transition from "
    << failedEvent.getFromState()
    <<  " to "
    << failedEvent.getToState()
    << " exception: " << failedEvent.getException().what();
    std::cout << __COUT_HDR_FL__ << error.str() << std::endl;
    //diagService_->reportError(errstr.str(),DIAGERROR);

}


#define ARTDAQ_FCL_PATH			std::string(getenv("USER_DATA")) + "/"+ "ARTDAQConfigurations/"
#define ARTDAQ_FILE_PREAMBLE	"aggregator"
//========================================================================================================================
void DataLoggerApp::transitionConfiguring(toolbox::Event::Reference e)
{

    std::cout << __COUT_HDR_FL__ << "ARTDAQDataLogger SUPERVISOR CONFIGURING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::cout << __COUT_HDR_FL__  << SOAPUtilities::translate(theStateMachine_.getCurrentMessage()) << std::endl;

    std::pair<std::string /*group name*/, ConfigurationGroupKey> theGroup(
    		SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).
    		    		    		getParameters().getValue("ConfigurationGroupName"),
    		    		ConfigurationGroupKey(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).
    		    		getParameters().getValue("ConfigurationGroupKey")));

	__COUT__ << "Configuration group name: " << theGroup.first << " key: " <<
			theGroup.second << std::endl;

    theConfigurationManager_->loadConfigurationGroup(
    		theGroup.first,
			theGroup.second, true);


	std::string path = "";
    char* dirMRB = getenv("MRB_BUILDDIR");
    char* dirP = getenv("OTSDAQ_DIR");

    if(dirMRB) { path = std::string(dirMRB) + "/otsdaq_demo/"; }
    else if(dirP) { path = std::string(dirP) + "/"; }

    //Now that the configuration manager has all the necessary configurations I can create all objects dependent of the configuration
    //std::string configString = "daq:{DataLogger:{event_builder_count:2 event_queue_depth:20 event_queue_wait_time:5 expected_events_per_bunch:1 file_duration:0 file_event_count:0 file_size_MB:0 first_event_builder_rank:2 mpi_buffer_count:8 print_event_store_stats:true xmlrpc_client_list:\";http://localhost:5603/RPC2,3;http://localhost:5604/RPC2,3;http://localhost:5605/RPC2,4;http://localhost:5606/RPC2,4;http://localhost:5601/RPC2,5;http://localhost:5602/RPC2,5\"} max_fragment_size_words:2.097152e6} outputs:{normalOutput:{fileName:\"/data/otsdata/data/artdaqots_r%06r_sr%02s_%to.root\" module_type:\"RootOutput\"}} physics:{my_output_modules:[\"normalOutput\"] p2:[\"BuildInfo\"] producers:{BuildInfo:{instance_name:\"ArtdaqOts\" module_type:\"ArtdaqOtsBuildInfo\"}}} process_name:\"DAQAG\" services:{Timing:{summaryOnly:true} scheduler:{fileMode:\"NOMERGE\"} user:{NetMonTransportServiceInterface:{max_fragment_size_words:2.097152e6 service_provider:\"NetMonTransportService\"}}} source:{module_type:\"NetMonInput\"}";
    //ONLY 1 BOARD READER
    //    std::string configString = "daq:{DataLogger:{event_builder_count:1 event_queue_depth:20 event_queue_wait_time:5 expected_events_per_bunch:1 file_duration:0 file_event_count:0 file_size_MB:0 first_event_builder_rank:1 mpi_buffer_count:8 print_event_store_stats:true xmlrpc_client_list:\";http://localhost:5100/RPC2,3;http://localhost:5101/RPC2,3;http://localhost:5200/RPC2,4;http://localhost:5201/RPC2,4;http://localhost:5300/RPC2,5;http://localhost:5301/RPC2,5\"} max_fragment_size_words:2.097152e6} outputs:{normalOutput:{fileName:\""+path+"artdaqots_r%06r_sr%02s_%to.root\" module_type:\"RootOutput\"}} physics:{my_output_modules:[\"normalOutput\"] p2:[\"BuildInfo\"] a1:[\"wf\"] producers:{BuildInfo:{instance_name:\"ArtdaqOts\" module_type:\"ArtdaqOtsBuildInfo\"}} analyzers:{wf: {module_type:\"WFViewer\" fragment_ids:[0] fragment_type_labels:[ \"DataGen\" ] prescale:60 write_to_file:true fileName:\""+path+"otsdaqdemo_onmon.root\"}}} process_name:\"DAQAG\" services:{Timing:{summaryOnly:true} scheduler:{fileMode:\"NOMERGE\" errorOnFailureToPut: false} NetMonTransportServiceInterface:{max_fragment_size_words:2097152 service_provider:\"NetMonTransportService\"}} source:{module_type:\"NetMonInput\"}";
    //2 BOARD READERS
    //std::string configString = "daq:{DataLogger:{event_builder_count:1 event_queue_depth:20 event_queue_wait_time:5 expected_events_per_bunch:1 file_duration:0 file_event_count:0 file_size_MB:0 first_event_builder_rank:1 mpi_buffer_count:8 print_event_store_stats:true xmlrpc_client_list:\";http://localhost:5100/RPC2,3;http://localhost:5101/RPC2,3;http://localhost:5200/RPC2,4;http://localhost:5201/RPC2,4;http://localhost:5300/RPC2,5;http://localhost:5301/RPC2,5\"} max_fragment_size_words:2.097152e6} outputs:{normalOutput:{fileName:\""+path+"artdaqots_r%06r_sr%02s_%to.root\" module_type:\"RootOutput\"}} physics:{my_output_modules:[\"normalOutput\"] p2:[\"BuildInfo\"] a1:[\"wf\"] producers:{BuildInfo:{instance_name:\"ArtdaqOts\" module_type:\"ArtdaqOtsBuildInfo\"}} analyzers:{wf: {module_type:\"WFViewer\" fragment_ids:[0] fragment_type_labels:[ \"DataGen\" ] prescale:60 write_to_file:true fileName:\""+path+"otsdaqdemo_onmon.root\"}}} process_name:\"DAQAG\" services:{Timing:{summaryOnly:true} scheduler:{fileMode:\"NOMERGE\" errorOnFailureToPut: false} NetMonTransportServiceInterface:{max_fragment_size_words:2097152 service_provider:\"NetMonTransportService\"}} source:{module_type:\"NetMonInput\"}";

    fhicl::ParameterSet pset;
    //fhicl::make_ParameterSet(configString, pset);

	std::string filename = ARTDAQ_FCL_PATH + ARTDAQ_FILE_PREAMBLE + "-";
	std::string uid = theConfigurationManager_->getNode(XDAQContextConfigurationName_).getNode(supervisorConfigurationPath_).getValue();

	__COUT__ << "uid: " << uid << std::endl;
	for(unsigned int i=0;i<uid.size();++i)
		if((uid[i] >= 'a' && uid[i] <= 'z') ||
				(uid[i] >= 'A' && uid[i] <= 'Z') ||
				(uid[i] >= '0' && uid[i] <= '9')) //only allow alpha numeric in file name
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

	__COUT__ << fileFclString << std::endl;
	fhicl::make_ParameterSet(fileFclString, pset);
	//fhicl::make_ParameterSet(theConfigurationManager_->getNode(XDAQContextConfigurationName_).getNode(supervisorConfigurationPath_).getNode("ConfigurationString").getValue<std::string>(), pset);


    theDataLoggerInterface_->initialize(pset, 0, 0);
    TLOG(TLVL_INFO, "DataLoggerInterface") << "ARTDAQDataLogger SUPERVISOR DONE CONFIGURING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";

}

//========================================================================================================================
void DataLoggerApp::transitionHalting(toolbox::Event::Reference e) 
{
	  try {
		  theDataLoggerInterface_->stop(45, 0);
	  }
	  catch (...) {
		  // It is okay for this to fail, esp. if already stopped...
	  }

	  try {
		  theDataLoggerInterface_->shutdown(45);
	  }
	  catch (...)
	  {
		  __MOUT_ERR__ << "ERROR OCCURRED DURING SHUTDOWN! STATE=" << theDataLoggerInterface_->status();
	  }

	  init();
}

//========================================================================================================================
void DataLoggerApp::transitionInitializing(toolbox::Event::Reference e) 
{

	init();
}

//========================================================================================================================
void DataLoggerApp::transitionPausing(toolbox::Event::Reference e) 
{
    theDataLoggerInterface_->pause(0, 0);
}

//========================================================================================================================
void DataLoggerApp::transitionResuming(toolbox::Event::Reference e) 
{
    theDataLoggerInterface_->resume(0, 0);
}

//========================================================================================================================
void DataLoggerApp::transitionStarting(toolbox::Event::Reference e) 
{
  auto runnumber= SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getParameters().getValue("RunNumber");
  try {
	art::RunID runId((art::RunNumber_t)boost::lexical_cast<art::RunNumber_t>(runnumber));
    theDataLoggerInterface_->start(runId,0,0);
  }
  catch(const boost::exception& e)
    {
      std::cerr << "Error parsing string to art::RunNumber_t: " << runnumber << std::endl;
      exit(1);
    }
}

//========================================================================================================================
void DataLoggerApp::transitionStopping(toolbox::Event::Reference e) 
{
    theDataLoggerInterface_->stop(0,0);
}
