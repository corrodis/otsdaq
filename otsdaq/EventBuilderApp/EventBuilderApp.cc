#include "otsdaq/EventBuilderApp/EventBuilderApp.h"
//#include "otsdaq-core/Macros/CoutMacros.h"
//#include "otsdaq-core/MessageFacility/MessageFacility.h"
//#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"
//#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
//#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
//
//#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
//#include "otsdaq-core/TablePluginDataFormats/XDAQContextTable.h"
//
//#include <toolbox/fsm/FailedEvent.h>
//
//#include <xdaq/NamespaceURI.h>
//#include <xoap/Method.h>
//
//#include <memory>
#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/make_ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

//#include <cassert>
//#include <fstream>
//#include <iostream>
//#include "otsdaq-core/TableCore/TableGroupKey.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(EventBuilderApp)

#define ARTDAQ_FCL_PATH std::string(getenv("USER_DATA")) + "/" + "ARTDAQConfigurations/"
#define ARTDAQ_FILE_PREAMBLE "builder"

//========================================================================================================================
EventBuilderApp::EventBuilderApp(xdaq::ApplicationStub* stub) : CoreSupervisorBase(stub)

//    : xdaq::Application(stub)
//    , SOAPMessenger(this)
//    , stateMachineWorkLoopManager_(
//          toolbox::task::bind(this, &EventBuilderApp::stateMachineThread,
//          "StateMachine"))
//    , stateMachineSemaphore_(toolbox::BSem::FULL)
//    , theConfigurationManager_(
//          new ConfigurationManager)  //(Singleton<ConfigurationManager>::getInstance())
//                                     ////I always load the full config but if I want to
//                                     // load a partial configuration (new
//                                     // ConfigurationManager)
//    , XDAQContextTableName_(
//          theConfigurationManager_->__GET_CONFIG__(XDAQContextTable)->getTableName())
//    , supervisorConfigurationPath_(
//          "INITIALIZED INSIDE THE CONTRUCTOR BECAUSE IT NEEDS supervisorContextUID_ and
//          " "supervisorApplicationUID_")
//    , supervisorContextUID_("INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
//    , supervisorApplicationUID_(
//          "INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
//, theConfigurationTableGroupKey_   (nullptr)
//, supervisorInstance_         (this->getApplicationDescriptor()->getInstance())
//        theFEWInterfacesManager_
//        (theConfigurationManager_,this->getApplicationDescriptor()->getInstance())
{
	__SUP_COUT__ << "Constructor." << __E__;

	INIT_MF("EventBuilderApp");
	//	xgi::bind(this, &EventBuilderApp::Default, "Default");
	//	xgi::bind(this, &EventBuilderApp::stateMachineXgiHandler,
	//"StateMachineXgiHandler");
	//
	//	xoap::bind(this,
	//	           &EventBuilderApp::stateMachineStateRequest,
	//	           "StateMachineStateRequest",
	//	           XDAQ_NS_URI);
	//	xoap::bind(this,
	//	           &EventBuilderApp::stateMachineErrorMessageRequest,
	//	           "StateMachineErrorMessageRequest",
	//	           XDAQ_NS_URI);
	//
	//	try
	//	{
	//		supervisorContextUID_ =
	//		    theConfigurationManager_->__GET_CONFIG__(XDAQContextTable)
	//		        ->getContextUID(
	//		            getApplicationContext()->getContextDescriptor()->getURL());
	//	}
	//	catch(...)
	//	{
	//		__COUT_ERR__
	//		    << "XDAQ Supervisor could not access it's configuration through "
	//		       "the Configuration Manager."
	//		    << ". The getApplicationContext()->getContextDescriptor()->getURL() = "
	//		    << getApplicationContext()->getContextDescriptor()->getURL() << __E__;
	//		throw;
	//	}
	//	try
	//	{
	//		supervisorApplicationUID_ =
	//		    theConfigurationManager_->__GET_CONFIG__(XDAQContextTable)
	//		        ->getApplicationUID(
	//		            getApplicationContext()->getContextDescriptor()->getURL(),
	//		            getApplicationDescriptor()->getLocalId());
	//	}
	//	catch(...)
	//	{
	//		__COUT_ERR__ << "XDAQ Supervisor could not access it's configuration through "
	//		                "the Configuration Manager."
	//		             << " The supervisorContextUID_ = " << supervisorContextUID_
	//		             << ". The supervisorApplicationUID = " <<
	// supervisorApplicationUID_
	//		             << __E__;
	//		throw;
	//	}
	//	supervisorConfigurationPath_ = "/" + supervisorContextUID_ +
	//	                               "/LinkToApplicationTable/" +
	//	                               supervisorApplicationUID_ +
	//"/LinkToSupervisorTable";
	//
	//	setStateMachineName(supervisorApplicationUID_);
	__SUP_COUT__ << "Constructed." << __E__;
}  // end constructor()

//========================================================================================================================
EventBuilderApp::~EventBuilderApp(void)
{
	__SUP_COUT__ << "Destructor." << __E__;
	destroy();
	__SUP_COUT__ << "Destructed." << __E__;
}  // end destructor()

//========================================================================================================================
void EventBuilderApp::init(void)
{
	__SUP_COUT__ << "Initializing..." << __E__;

	// allSupervisorInfo_.init(getApplicationContext());

	artdaq::configureMessageFacility("eventbuilder");
	__SUP_COUT__ << "artdaq MF configured." << __E__;

	// initialization

	std::string    name = "Builder";
	unsigned short port = 5200;
	// artdaq::setMsgFacAppName(supervisorApplicationUID_, port);
	artdaq::setMsgFacAppName(name, port);

	TLOG(TLVL_DEBUG, name + "Supervisor")
	    << "artdaq version "
	    << artdaq::GetPackageBuildInfo::getPackageBuildInfo().getPackageVersion()
	    << ", built "
	    << artdaq::GetPackageBuildInfo::getPackageBuildInfo().getBuildTimestamp();

	// create the EventBuilderInterface
	app_name = name;
	my_rank  = this->getApplicationDescriptor()->getLocalId();
	theARTDAQEventBuilderInterface_.reset(new artdaq::EventBuilderApp());

	__SUP_COUT__ << "Initialized." << __E__;
}  // end init()

//========================================================================================================================
void EventBuilderApp::destroy(void)
{
	__SUP_COUT__ << "Destroying..." << __E__;
	theARTDAQEventBuilderInterface_.reset(nullptr);
	__SUP_COUT__ << "Destroyed." << __E__;
}  // end destroy()
//
////========================================================================================================================
// void EventBuilderApp::Default(xgi::Input* in, xgi::Output* out)
//
//{
//	*out << "<!DOCTYPE HTML><html lang='en'><frameset col='100%' row='100%'><frame "
//	        "src='/WebPath/html/EventBuilderApp.html?urn="
//	     << this->getApplicationDescriptor()->getLocalId() << "'></frameset></html>";
//}
//
////========================================================================================================================
// void EventBuilderApp::stateMachineXgiHandler(xgi::Input* in, xgi::Output* out) {}
//
////========================================================================================================================
// void EventBuilderApp::stateMachineResultXgiHandler(xgi::Input* in, xgi::Output* out) {}
//
////========================================================================================================================
// xoap::MessageReference EventBuilderApp::stateMachineXoapHandler(
//    xoap::MessageReference message)
//
//{
//	std::cout << __COUT_HDR_FL__ << "Soap Handler!" << std::endl;
//	stateMachineWorkLoopManager_.removeProcessedRequests();
//	stateMachineWorkLoopManager_.processRequest(message);
//	std::cout << __COUT_HDR_FL__ << "Done - Soap Handler!" << std::endl;
//	return message;
//}
//
////========================================================================================================================
// xoap::MessageReference EventBuilderApp::stateMachineResultXoapHandler(
//    xoap::MessageReference message)
//
//{
//	std::cout << __COUT_HDR_FL__ << "Soap Handler!" << std::endl;
//	// stateMachineWorkLoopManager_.removeProcessedRequests();
//	// stateMachineWorkLoopManager_.processRequest(message);
//	std::cout << __COUT_HDR_FL__ << "Done - Soap Handler!" << std::endl;
//	return message;
//}
//
////========================================================================================================================
// bool EventBuilderApp::stateMachineThread(toolbox::task::WorkLoop* workLoop)
//{
//	stateMachineSemaphore_.take();
//	std::cout << __COUT_HDR_FL__ << "Re-sending message..."
//	          << SOAPUtilities::translate(
//	                 stateMachineWorkLoopManager_.getMessage(workLoop))
//	                 .getCommand()
//	          << std::endl;
//	std::string reply = send(this->getApplicationDescriptor(),
//	                         stateMachineWorkLoopManager_.getMessage(workLoop));
//	stateMachineWorkLoopManager_.report(workLoop, reply, 100, true);
//	std::cout << __COUT_HDR_FL__ << "Done with message" << std::endl;
//	stateMachineSemaphore_.give();
//	return false;  // execute once and automatically remove the workloop so in
//	               // WorkLoopManager the try workLoop->remove(job_) could be commented
//	               // out return true;//go on and then you must do the
//	               // workLoop->remove(job_) in WorkLoopManager
//}
//
////========================================================================================================================
// xoap::MessageReference EventBuilderApp::stateMachineStateRequest(
//    xoap::MessageReference message)
//
//{
//	std::cout << __COUT_HDR_FL__ << theStateMachine_.getCurrentStateName() << std::endl;
//	return SOAPUtilities::makeSOAPMessageReference(
//	    theStateMachine_.getCurrentStateName());
//}
//
////========================================================================================================================
// xoap::MessageReference EventBuilderApp::stateMachineErrorMessageRequest(
//    xoap::MessageReference message)
//
//{
//	__COUT__ << "theStateMachine_.getErrorMessage() = "
//	         << theStateMachine_.getErrorMessage() << std::endl;
//
//	SOAPParameters retParameters;
//	retParameters.addParameter("ErrorMessage", theStateMachine_.getErrorMessage());
//	return SOAPUtilities::makeSOAPMessageReference("stateMachineErrorMessageRequestReply",
//	                                               retParameters);
//}
//
////========================================================================================================================
// void EventBuilderApp::stateInitial(toolbox::fsm::FiniteStateMachine& fsm) {}
//
////========================================================================================================================
// void EventBuilderApp::stateHalted(toolbox::fsm::FiniteStateMachine& fsm) {}
//
////========================================================================================================================
// void EventBuilderApp::stateRunning(toolbox::fsm::FiniteStateMachine& fsm) {}
//
////========================================================================================================================
// void EventBuilderApp::stateConfigured(toolbox::fsm::FiniteStateMachine& fsm) {}
//
////========================================================================================================================
// void EventBuilderApp::statePaused(toolbox::fsm::FiniteStateMachine& fsm) {}
//
////========================================================================================================================
// void EventBuilderApp::inError(toolbox::fsm::FiniteStateMachine& fsm)
//
//{
//	std::cout << __COUT_HDR_FL__
//	          << "Fsm current state: " << theStateMachine_.getCurrentStateName()
//	          << std::endl;
//	// rcmsStateNotifier_.stateChanged("Error", "");
//}
//
////========================================================================================================================
// void EventBuilderApp::enteringError(toolbox::Event::Reference e)
//
//{
//	std::cout << __COUT_HDR_FL__
//	          << "Fsm current state: " << theStateMachine_.getCurrentStateName()
//	          << std::endl;
//	toolbox::fsm::FailedEvent& failedEvent = dynamic_cast<toolbox::fsm::FailedEvent&>(*e);
//	std::ostringstream         error;
//	error << "Failure performing transition from " << failedEvent.getFromState() << " to "
//	      << failedEvent.getToState()
//	      << " exception: " << failedEvent.getException().what();
//	std::cout << __COUT_HDR_FL__ << error.str() << std::endl;
//	// diagService_->reportError(errstr.str(),DIAGERROR);
//}

//========================================================================================================================
void EventBuilderApp::transitionConfiguring(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Configuring..." << __E__;

	__SUP_COUT__ << SOAPUtilities::translate(theStateMachine_.getCurrentMessage())
	             << __E__;

	std::pair<std::string /*group name*/, TableGroupKey> theGroup(
	    SOAPUtilities::translate(theStateMachine_.getCurrentMessage())
	        .getParameters()
	        .getValue("ConfigurationTableGroupName"),
	    TableGroupKey(SOAPUtilities::translate(theStateMachine_.getCurrentMessage())
	                      .getParameters()
	                      .getValue("ConfigurationTableGroupKey")));

	__SUP_COUT__ << "Configuration group name: " << theGroup.first
	             << " key: " << theGroup.second << __E__;

	theConfigurationManager_->loadTableGroup(theGroup.first, theGroup.second, true);

	// load fcl string from dedicated file
	fhicl::ParameterSet pset;

	std::string        filename = ARTDAQ_FCL_PATH + ARTDAQ_FILE_PREAMBLE + "-";
	const std::string& uid      = theConfigurationManager_ ->getNode(
		ConfigurationManager::XDAQ_APPLICATION_TABLE_NAME + "/" +
		CorePropertySupervisorBase::getSupervisorUID() + "/" +
		"LinkToSupervisorTable").getValueAsString();

	__SUP_COUTV__(uid);
	for(unsigned int i = 0; i < uid.size(); ++i)
		if((uid[i] >= 'a' && uid[i] <= 'z') || (uid[i] >= 'A' && uid[i] <= 'Z') ||
		   (uid[i] >= '0' && uid[i] <= '9'))  // only allow alpha numeric in file name
			filename += uid[i];
	filename += ".fcl";

	__SUP_COUTV__(filename);

	std::string fileFclString;
	{
		std::ifstream in(filename, std::ios::in | std::ios::binary);
		if(in)
		{
			std::string contents;
			in.seekg(0, std::ios::end);
			fileFclString.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&fileFclString[0], fileFclString.size());
			in.close();
		}
		else
		{
			__SUP_SS__ << "Fhicl file not found! " << filename << __E__;
			__SUP_SS_THROW__;
		}
	}

	__SUP_COUTV__(fileFclString);

	try
	{
		fhicl::make_ParameterSet(fileFclString, pset);
		theARTDAQEventBuilderInterface_->initialize(pset, 0, 0);
	}
	catch(const cet::coded_exception<fhicl::error, &fhicl::detail::translate>& e)
	{
		__SUP_SS__ << "Error was caught while configuring: " << e.what() << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "EventBuilderApp::transitionConfiguring" /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}

	__SUP_COUT__ << "Configured." << __E__;
}  // end transitionConfiguring()

//========================================================================================================================
void EventBuilderApp::transitionHalting(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Halting..." << __E__;
	try
	{
		theARTDAQEventBuilderInterface_->stop(45, 0);
	}
	catch(...)
	{
		// It is okay for this to fail, esp. if already stopped...
		__SUP_COUT__ << "Ignoring error on halt." << __E__;
	}

	try
	{
		theARTDAQEventBuilderInterface_->shutdown(45);
	}
	catch(...)
	{
		__SUP_COUT_ERR__ << "Error occurred during shutdown! State="
		                 << theARTDAQEventBuilderInterface_->status();
	}

	init();
	__SUP_COUT__ << "Halted." << __E__;
}  // end transitionHalting()

//========================================================================================================================
void EventBuilderApp::transitionInitializing(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Initializing..." << __E__;
	init();
	__SUP_COUT__ << "Initialized." << __E__;
}  // end transitionInitializing()

//========================================================================================================================
void EventBuilderApp::transitionPausing(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Pausing..." << __E__;
	theARTDAQEventBuilderInterface_->pause(0, 0);
	__SUP_COUT__ << "Paused." << __E__;
}  // end transitionPausing()

//========================================================================================================================
void EventBuilderApp::transitionResuming(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Resuming..." << __E__;
	theARTDAQEventBuilderInterface_->resume(0, 0);
	__SUP_COUT__ << "Resumed." << __E__;
}  // end transitionResuming()

//========================================================================================================================
void EventBuilderApp::transitionStarting(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Starting..." << __E__;

	auto runNumber = SOAPUtilities::translate(theStateMachine_.getCurrentMessage())
	                     .getParameters()
	                     .getValue("RunNumber");
	try
	{
		art::RunID runId(
		    (art::RunNumber_t)boost::lexical_cast<art::RunNumber_t>(runNumber));
		theARTDAQEventBuilderInterface_->start(runId, 0, 0);
	}
	catch(const boost::exception& e)
	{
		__SUP_SS__ << "Error parsing string to art::RunNumber_t: " << runNumber << __E__;
		__SUP_SS_THROW__;
	}
	__SUP_COUT__ << "Started." << __E__;
}  // end transitionStarting()

//========================================================================================================================
void EventBuilderApp::transitionStopping(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Stopping..." << __E__;
	theARTDAQEventBuilderInterface_->stop(45, 0);
	__SUP_COUT__ << "Stopped." << __E__;
}  // end transitionStopping()
