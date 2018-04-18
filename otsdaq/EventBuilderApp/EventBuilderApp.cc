#include "otsdaq/EventBuilderApp/EventBuilderApp.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"

#include <toolbox/fsm/FailedEvent.h>

#include <xdaq/NamespaceURI.h>
#include <xoap/Method.h>

#include <memory>
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "cetlib/exception.h"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "fhiclcpp/make_ParameterSet.h"

#include <fstream>
#include <iostream>
#include <cassert>

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(EventBuilderApp)

//========================================================================================================================
EventBuilderApp::EventBuilderApp(xdaq::ApplicationStub* stub)
throw (xdaq::exception::Exception)
: xdaq::Application            (stub)
, SOAPMessenger                (this)
, stateMachineWorkLoopManager_ (toolbox::task::bind(this, &EventBuilderApp::stateMachineThread, "StateMachine"))
, stateMachineSemaphore_       (toolbox::BSem::FULL)
, theConfigurationManager_     (new ConfigurationManager)//(Singleton<ConfigurationManager>::getInstance()) //I always load the full config but if I want to load a partial configuration (new ConfigurationManager)
, XDAQContextConfigurationName_(theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getConfigurationName())
, supervisorConfigurationPath_ ("INITIALIZED INSIDE THE CONTRUCTOR BECAUSE IT NEEDS supervisorContextUID_ and supervisorApplicationUID_")
, supervisorContextUID_        ("INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
, supervisorApplicationUID_    ("INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
//, theConfigurationGroupKey_   (nullptr)
//, supervisorInstance_         (this->getApplicationDescriptor()->getInstance())
//        theFEWInterfacesManager_    (theConfigurationManager_,this->getApplicationDescriptor()->getInstance())
{
	INIT_MF("EventBuilderApp");
	xgi::bind (this, &EventBuilderApp::Default,                "Default" );
	xgi::bind (this, &EventBuilderApp::stateMachineXgiHandler, "StateMachineXgiHandler");

	xoap::bind(this, &EventBuilderApp::stateMachineStateRequest, 			"StateMachineStateRequest", XDAQ_NS_URI );
	xoap::bind(this, &EventBuilderApp::stateMachineErrorMessageRequest,  "StateMachineErrorMessageRequest",  XDAQ_NS_URI );

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
	init();
}

//========================================================================================================================
EventBuilderApp::~EventBuilderApp(void)
{
	destroy();
}
//========================================================================================================================
void EventBuilderApp::init(void)
{
	std::cout << __COUT_HDR_FL__ << "ARTDAQBUILDER SUPERVISOR INIT START!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	allSupervisorInfo_.init(getApplicationContext());

	artdaq::configureMessageFacility("eventbuilder");

	// initialization
	
	std::cout << __COUT_HDR_FL__ << "ARTDAQBUILDER SUPERVISOR NO ERRORS MAKING MSG FACILITY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	std::string    name = "Builder";
	unsigned short port = 5200;
	//artdaq::setMsgFacAppName(supervisorApplicationUID_, port);
	artdaq::setMsgFacAppName(name, port);
	std::cout << __COUT_HDR_FL__ << "ARTDAQBUILDER SUPERVISOR MSG FACILITY DONE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	mf::LogDebug(name + "Supervisor") << "artdaq version " <<
//    mf::LogDebug(supervisorApplicationUID_) << " artdaq version " <<
			artdaq::GetPackageBuildInfo::getPackageBuildInfo().getPackageVersion()
			<< ", built " <<
			artdaq::GetPackageBuildInfo::getPackageBuildInfo().getBuildTimestamp();

	// create the EventBuilderInterface
	app_name = name;
	my_rank = this->getApplicationDescriptor()->getLocalId();
	theARTDAQEventBuilderInterfaces_[0] = new artdaq::EventBuilderApp( );
	std::cout << __COUT_HDR_FL__ << "ARTDAQBUILDER SUPERVISOR INIT DONE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
}

//========================================================================================================================
void EventBuilderApp::destroy(void)
{
}

//========================================================================================================================
void EventBuilderApp::Default(xgi::Input * in, xgi::Output * out )
throw (xgi::exception::Exception)
{

	*out << "<!DOCTYPE HTML><html lang='en'><frameset col='100%' row='100%'><frame src='/WebPath/html/EventBuilderApp.html?urn=" <<
			this->getApplicationDescriptor()->getLocalId() <<"'></frameset></html>";
}

//========================================================================================================================
void EventBuilderApp::stateMachineXgiHandler(xgi::Input * in, xgi::Output * out )
throw (xgi::exception::Exception)
{}

//========================================================================================================================
void EventBuilderApp::stateMachineResultXgiHandler(xgi::Input* in, xgi::Output* out )
throw (xgi::exception::Exception)
{}

//========================================================================================================================
xoap::MessageReference EventBuilderApp::stateMachineXoapHandler(xoap::MessageReference message )
throw (xoap::exception::Exception)
{
	std::cout << __COUT_HDR_FL__ << "Soap Handler!" << std::endl;
	stateMachineWorkLoopManager_.removeProcessedRequests();
	stateMachineWorkLoopManager_.processRequest(message);
	std::cout << __COUT_HDR_FL__ << "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
xoap::MessageReference EventBuilderApp::stateMachineResultXoapHandler(xoap::MessageReference message )
throw (xoap::exception::Exception)
{
	std::cout << __COUT_HDR_FL__ << "Soap Handler!" << std::endl;
	//stateMachineWorkLoopManager_.removeProcessedRequests();
	//stateMachineWorkLoopManager_.processRequest(message);
	std::cout << __COUT_HDR_FL__ << "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
bool EventBuilderApp::stateMachineThread(toolbox::task::WorkLoop* workLoop)
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
xoap::MessageReference EventBuilderApp::stateMachineStateRequest(xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	std::cout << __COUT_HDR_FL__ << theStateMachine_.getCurrentStateName() << std::endl;
	return SOAPUtilities::makeSOAPMessageReference(theStateMachine_.getCurrentStateName());
}

//========================================================================================================================
xoap::MessageReference EventBuilderApp::stateMachineErrorMessageRequest(xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	__COUT__<< "theStateMachine_.getErrorMessage() = " << theStateMachine_.getErrorMessage() << std::endl;

	SOAPParameters retParameters;
	retParameters.addParameter("ErrorMessage",theStateMachine_.getErrorMessage());
	return SOAPUtilities::makeSOAPMessageReference("stateMachineErrorMessageRequestReply",retParameters);
}

//========================================================================================================================
void EventBuilderApp::stateInitial(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{

}

//========================================================================================================================
void EventBuilderApp::stateHalted(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{

}

//========================================================================================================================
void EventBuilderApp::stateRunning(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{

}

//========================================================================================================================
void EventBuilderApp::stateConfigured(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{

}

//========================================================================================================================
void EventBuilderApp::statePaused(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{

}

//========================================================================================================================
void EventBuilderApp::inError (toolbox::fsm::FiniteStateMachine & fsm)
throw (toolbox::fsm::exception::Exception)
{
	std::cout << __COUT_HDR_FL__ << "Fsm current state: " << theStateMachine_.getCurrentStateName()<< std::endl;
	//rcmsStateNotifier_.stateChanged("Error", "");
}

//========================================================================================================================
void EventBuilderApp::enteringError (toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
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
#define ARTDAQ_FILE_PREAMBLE	"builder"

//========================================================================================================================
void EventBuilderApp::transitionConfiguring(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{

	std::cout << __COUT_HDR_FL__ << "ARTDAQBUILDER SUPERVISOR CONFIGURING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
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

	//theConfigurationManager_->setupEventBuilderAppConfiguration(theConfigurationGroupKey_,supervisorInstance_);

	//Now that the configuration manager has all the necessary configurations I can create all objects dependent of the configuration
	//std::string configString = "daq:{event_builder:{expected_fragments_per_event:2 first_fragment_receiver_rank:0 fragment_receiver_count:2 mpi_buffer_count:16 print_event_store_stats:true send_triggers:true trigger_port:5001 use_art:true verbose:false} max_fragment_size_words:2.097152e6} outputs:{netMonOutput:{module_type:\"NetMonOutput\"}} physics:{my_output_modules:[\"netMonOutput\"]} process_name:\"DAQ\" services:{Timing:{summaryOnly:true} scheduler:{fileMode:\"NOMERGE\"} user:{NetMonTransportServiceInterface:{data_receiver_count:1 first_data_receiver_rank:4 max_fragment_size_words:2.097152e6 mpi_buffer_count:8 service_provider:\"NetMonTransportService\"}}} source:{fragment_type_map:[[1,\"UDP\"]] module_type:\"RawInput\" resume_after_timeout:true waiting_time:900}";

	//ONLY 1 BOARD READER IN THE SYSTEM
	//std::string configString = "daq:{event_builder:{expected_fragments_per_event:1 first_fragment_receiver_rank:0 fragment_receiver_count:1 mpi_buffer_count:16 print_event_store_stats:true send_triggers:true trigger_port:5001 use_art:true verbose:false} max_fragment_size_words:2.097152e6} outputs:{netMonOutput:{module_type:\"RootMPIOutput\"}} physics:{analyzers:{wf: {module_type:\"WFViewer\" fragment_ids:[0] fragment_type_labels:[ \"DataGen\" ] prescale:60 write_to_file:true fileName:\"/tmp/otsdaqdemo_onmon_evb.root\"}}  a1:[\"wf\"] my_output_modules:[\"netMonOutput\"]} process_name:\"DAQ\" services:{Timing:{summaryOnly:true} scheduler:{fileMode:\"NOMERGE\" errorOnFailureToPut: false} user:{NetMonTransportServiceInterface:{data_receiver_count:1 first_data_receiver_rank:2 max_fragment_size_words:2.097152e6 mpi_buffer_count:8 service_provider:\"NetMonTransportService\"}}} source:{fragment_type_map:[[1,\"UDP\"], [4,\"DataGen\"]] module_type:\"RawInput\" resume_after_timeout:true waiting_time:900}";

	//2 BOARD READERS IN THE SYSTEM
	//std::string configString = "daq:{event_builder:{expected_fragments_per_event:1 first_fragment_receiver_rank:0 fragment_receiver_count:1 mpi_buffer_count:16 print_event_store_stats:true send_triggers:true trigger_port:5001 use_art:true verbose:false} max_fragment_size_words:2.097152e6} outputs:{netMonOutput:{module_type:\"RootMPIOutput\"}} physics:{analyzers:{wf: {module_type:\"WFViewer\" fragment_ids:[0] fragment_type_labels:[ \"DataGen\" ] prescale:60 write_to_file:true fileName:\"/tmp/otsdaqdemo_onmon_evb.root\"}}  a1:[\"wf\"] my_output_modules:[\"netMonOutput\"]} process_name:\"DAQ\" services:{Timing:{summaryOnly:true} scheduler:{fileMode:\"NOMERGE\" errorOnFailureToPut: false} user:{NetMonTransportServiceInterface:{data_receiver_count:1 first_data_receiver_rank:2 max_fragment_size_words:2.097152e6 mpi_buffer_count:8 service_provider:\"NetMonTransportService\"}}} source:{fragment_type_map:[[1,\"UDP\"], [4,\"DataGen\"]] module_type:\"RawInput\" resume_after_timeout:true waiting_time:900}";

	//__COUT__ << configString << std::endl;
	__COUT__ << std::endl;
	__COUT__ << std::endl;
	//__COUT__ << theConfigurationManager_->getNode(XDAQContextConfigurationName_).getNode(supervisorConfigurationPath_).getNode("ConfigurationString").getValue<std::string>() << std::endl;
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

	try
	{
		fhicl::make_ParameterSet(fileFclString, pset);

		//fhicl::make_ParameterSet(theConfigurationManager_->getNode(XDAQContextConfigurationName_).getNode(supervisorConfigurationPath_).getNode("ConfigurationString").getValue<std::string>(), pset);

		for(auto it=theARTDAQEventBuilderInterfaces_.begin(); it!=theARTDAQEventBuilderInterfaces_.end(); it++)
			it->second->initialize(pset,0,0);
	}
	catch(const cet::coded_exception<fhicl::error, &fhicl::detail::translate>& e)
	{
		__SS__ << "Error was caught while configuring: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"EventBuilderApp::transitionConfiguring" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
				);
	}

	std::cout << __COUT_HDR_FL__ << "ARTDAQBUILDER SUPERVISOR DONE CONFIGURING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;

}

//========================================================================================================================
void EventBuilderApp::transitionHalting(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{

  //	for(auto it=theARTDAQEventBuilderInterfaces_.begin(); it!=theARTDAQEventBuilderInterfaces_.end(); it++)
  //		it->second->shutdown(0);
}

//========================================================================================================================
void EventBuilderApp::transitionInitializing(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{

}

//========================================================================================================================
void EventBuilderApp::transitionPausing(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{

	for(auto it=theARTDAQEventBuilderInterfaces_.begin(); it!=theARTDAQEventBuilderInterfaces_.end(); it++)
		it->second->pause(0, 0);
}

//========================================================================================================================
void EventBuilderApp::transitionResuming(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{

	for(auto it=theARTDAQEventBuilderInterfaces_.begin(); it!=theARTDAQEventBuilderInterfaces_.end(); it++)
		it->second->resume(0, 0);
}

//========================================================================================================================
void EventBuilderApp::transitionStarting(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{

	art::RunID runId((art::RunNumber_t)boost::lexical_cast<art::RunNumber_t>(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getParameters().getValue("RunNumber")));
	for(auto it=theARTDAQEventBuilderInterfaces_.begin(); it!=theARTDAQEventBuilderInterfaces_.end(); it++)
		it->second->start(runId, 0, 0);
}

//========================================================================================================================
void EventBuilderApp::transitionStopping(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{

	for(auto it=theARTDAQEventBuilderInterfaces_.begin(); it!=theARTDAQEventBuilderInterfaces_.end(); it++)
		it->second->stop(45, 0);

	for(auto it=theARTDAQEventBuilderInterfaces_.begin(); it!=theARTDAQEventBuilderInterfaces_.end(); it++)
		it->second->shutdown(45);
}
