#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

//#include "otsdaq-core/Singleton/Singleton.h"
//#include "otsdaq-core/FECore/FEVInterfacesManager.h"



#include <iostream>

using namespace ots;

//XDAQ_INSTANTIATOR_IMPL(CoreSupervisorBase)


const std::string		CoreSupervisorBase::WORK_LOOP_DONE 			= "Done";
const std::string		CoreSupervisorBase::WORK_LOOP_WORKING 		= "Working";

//========================================================================================================================
CoreSupervisorBase::CoreSupervisorBase(xdaq::ApplicationStub * s)
throw (xdaq::exception::Exception)
: xdaq::Application             (s)
, SOAPMessenger                 (this)
, stateMachineWorkLoopManager_  (toolbox::task::bind(this, &CoreSupervisorBase::stateMachineThread, "StateMachine"))
, stateMachineSemaphore_        (toolbox::BSem::FULL)
, theConfigurationManager_      (new ConfigurationManager)//(Singleton<ConfigurationManager>::getInstance()) //I always load the full config but if I want to load a partial configuration (new ConfigurationManager)
, XDAQContextConfigurationName_ (theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getConfigurationName())
, supervisorConfigurationPath_  ("INITIALIZED INSIDE THE CONTRUCTOR BECAUSE IT NEEDS supervisorContextUID_ and supervisorApplicationUID_")
, supervisorContextUID_         ("INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
, supervisorApplicationUID_     ("INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
, supervisorClass_              (getApplicationDescriptor()->getClassName())
, supervisorClassNoNamespace_   (supervisorClass_.substr(supervisorClass_.find_last_of(":")+1, supervisorClass_.length()-supervisorClass_.find_last_of(":")))
, theRemoteWebUsers_ 			(this)
, LOCK_REQUIRED_	 			(false) 	//set default
, USER_PERMISSIONS_THRESHOLD_	(1) 		//set default
{
	INIT_MF("CoreSupervisorBase");

	__COUT__ << "Begin!" << std::endl;

	xgi::bind (this, &CoreSupervisorBase::DefaultWrapper,         "Default" );
	xgi::bind (this, &CoreSupervisorBase::requestWrapper, 		  "Request");
	xgi::bind (this, &CoreSupervisorBase::stateMachineXgiHandler, "StateMachineXgiHandler");

	xoap::bind(this, &CoreSupervisorBase::stateMachineStateRequest,    		"StateMachineStateRequest",    		XDAQ_NS_URI );
	xoap::bind(this, &CoreSupervisorBase::stateMachineErrorMessageRequest,  "StateMachineErrorMessageRequest",  XDAQ_NS_URI );
	//xoap::bind(this, &CoreSupervisorBase::macroMakerSupervisorRequest, 		"MacroMakerSupervisorRequest", 		XDAQ_NS_URI );
	xoap::bind(this, &CoreSupervisorBase::workLoopStatusRequestWrapper, 	"WorkLoopStatusRequest",    		XDAQ_NS_URI );

	try
	{
		supervisorContextUID_ = theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getContextUID(
				getApplicationContext()->getContextDescriptor()->getURL());
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

	__COUT__ << "Done!" << std::endl;

	init();
}

//========================================================================================================================
CoreSupervisorBase::~CoreSupervisorBase(void)
{
	destroy();
}

//========================================================================================================================
void CoreSupervisorBase::init(void)
{
	//This can be done in the constructor because when you start xdaq it loads the configuration that can't be changed while running!
	__COUT__ << "init" << std::endl;

	allSupervisorInfo_.init(getApplicationContext());
	__COUT__ << "Name = " << allSupervisorInfo_.getSupervisorInfo(this).getName();

	ConfigurationTree appNode = theConfigurationManager_->getSupervisorNode(
			CoreSupervisorBase::supervisorContextUID_, CoreSupervisorBase::supervisorApplicationUID_);
	//try to get security settings
	try
	{
		__COUT__ << "Looking for supervisor security settings..." << __E__;
		auto /*map<name,node>*/ children = appNode.getNode("LinkToPropertyConfiguration").getChildren();

		for(auto& child:children)
		{
			if(child.second.getNode("Status").getValue<bool>() == false) continue; //skip OFF properties

			auto propertyName = child.second.getNode("PropertyName");

			if(propertyName.getValue() ==
					supervisorProperties_.fieldRequireLock)
			{
				LOCK_REQUIRED_ = child.second.getNode("PropertyValue").getValue<bool>();
				__COUTV__(LOCK_REQUIRED_);
			}
			else if(propertyName.getValue() ==
					supervisorProperties_.fieldUserPermissionsThreshold)
			{
				USER_PERMISSIONS_THRESHOLD_ = child.second.getNode("PropertyValue").getValue<uint8_t>();
				__COUTV__(USER_PERMISSIONS_THRESHOLD_);
			}
		}
	}
	catch(...)
	{
		__COUT__ << "No supervisor security settings found, going with defaults." << __E__;
	}

	__COUT__ << "init complete!" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::destroy(void)
{
	for(auto& it: theStateMachineImplementation_)
		delete it;
	theStateMachineImplementation_.clear();
}

//========================================================================================================================
//wrapper for inheritance call
void CoreSupervisorBase::DefaultWrapper(xgi::Input * in, xgi::Output * out )
throw (xgi::exception::Exception)
{
	return Default(in,out);
}

//========================================================================================================================
void CoreSupervisorBase::Default(xgi::Input * in, xgi::Output * out )
throw (xgi::exception::Exception)
{
	__COUT__<< "Supervisor class " << supervisorClass_ << std::endl;

	std::stringstream pagess;
	pagess << "/WebPath/html/" <<
			supervisorClassNoNamespace_ << "Supervisor.html?urn=" <<
			this->getApplicationDescriptor()->getLocalId();

	__COUT__<< "Default page = " << pagess.str() << std::endl;

	*out << "<!DOCTYPE HTML><html lang='en'><frameset col='100%' row='100%'><frame src='" <<
			pagess.str() <<
			"'></frameset></html>";
}

//========================================================================================================================
//wrapper for inheritance call
void CoreSupervisorBase::requestWrapper(xgi::Input * in, xgi::Output * out )
throw (xgi::exception::Exception)
{
	return request(in,out);
}

//========================================================================================================================
void CoreSupervisorBase::request(xgi::Input * in, xgi::Output * out )
throw (xgi::exception::Exception)
{
//
//
//	cgicc::Cgicc cgi(in);
//	std::string write = CgiDataUtilities::getOrPostData(cgi,"write");
//	std::string addr = CgiDataUtilities::getOrPostData(cgi,"addr");
//	std::string data = CgiDataUtilities::getOrPostData(cgi,"data");
//
//	__COUT__<< "write " << write << " addr: " << addr << " data: " << data << std::endl;
//
//	unsigned long long int addr64,data64;
//	sscanf(addr.c_str(),"%llu",&addr64);
//	sscanf(data.c_str(),"%llu",&data64);
//	__COUT__<< "write " << write << " addr: " << addr64 << " data: " << data64 << std::endl;
//
//	*out << "done";
}

//========================================================================================================================
void CoreSupervisorBase::stateMachineXgiHandler(xgi::Input * in, xgi::Output * out )
throw (xgi::exception::Exception)
{}

//========================================================================================================================
void CoreSupervisorBase::stateMachineResultXgiHandler(xgi::Input* in, xgi::Output* out )
throw (xgi::exception::Exception)
{}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineXoapHandler(xoap::MessageReference message )
throw (xoap::exception::Exception)
{
	__COUT__<< "Soap Handler!" << std::endl;
	stateMachineWorkLoopManager_.removeProcessedRequests();
	stateMachineWorkLoopManager_.processRequest(message);
	__COUT__<< "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineResultXoapHandler(xoap::MessageReference message )
throw (xoap::exception::Exception)
{
	__COUT__<< "Soap Handler!" << std::endl;
	//stateMachineWorkLoopManager_.removeProcessedRequests();
	//stateMachineWorkLoopManager_.processRequest(message);
	__COUT__<< "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
//indirection to allow for overriding handler
xoap::MessageReference CoreSupervisorBase::workLoopStatusRequestWrapper(xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	//this should have an override for monitoring work loops being done
	return workLoopStatusRequest(message);
} //end workLoopStatusRequest()

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::workLoopStatusRequest(xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	//this should have an override for monitoring work loops being done
	return SOAPUtilities::makeSOAPMessageReference(CoreSupervisorBase::WORK_LOOP_DONE);
} //end workLoopStatusRequest()

//========================================================================================================================
bool CoreSupervisorBase::stateMachineThread(toolbox::task::WorkLoop* workLoop)
{
	stateMachineSemaphore_.take();
	__COUT__<< "Re-sending message..." << SOAPUtilities::translate(stateMachineWorkLoopManager_.getMessage(workLoop)).getCommand() << std::endl;
	std::string reply = send(this->getApplicationDescriptor(),stateMachineWorkLoopManager_.getMessage(workLoop));
	stateMachineWorkLoopManager_.report(workLoop, reply, 100, true);
	__COUT__<< "Done with message" << std::endl;
	stateMachineSemaphore_.give();
	return false;//execute once and automatically remove the workloop so in WorkLoopManager the try workLoop->remove(job_) could be commented out
	//return true;//go on and then you must do the workLoop->remove(job_) in WorkLoopManager
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineStateRequest(xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	__COUT__<< "theStateMachine_.getCurrentStateName() = " << theStateMachine_.getCurrentStateName() << std::endl;
	return SOAPUtilities::makeSOAPMessageReference(theStateMachine_.getCurrentStateName());
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineErrorMessageRequest(xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	__COUT__<< "theStateMachine_.getErrorMessage() = " << theStateMachine_.getErrorMessage() << std::endl;

	SOAPParameters retParameters;
	retParameters.addParameter("ErrorMessage",theStateMachine_.getErrorMessage());
	return SOAPUtilities::makeSOAPMessageReference("stateMachineErrorMessageRequestReply",retParameters);
}

//========================================================================================================================
void CoreSupervisorBase::stateInitial(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "CoreSupervisorBase::stateInitial" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateHalted(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "CoreSupervisorBase::stateHalted" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateRunning(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "CoreSupervisorBase::stateRunning" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateConfigured(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "CoreSupervisorBase::stateConfigured" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::statePaused(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "CoreSupervisorBase::statePaused" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::inError (toolbox::fsm::FiniteStateMachine & fsm)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__<< "Fsm current state: " << theStateMachine_.getCurrentStateName()<< std::endl;
	//rcmsStateNotifier_.stateChanged("Error", "");
}

//========================================================================================================================
void CoreSupervisorBase::enteringError (toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__<< "Fsm current state: " << theStateMachine_.getCurrentStateName()
			<< "\n\nError Message: " <<
			theStateMachine_.getErrorMessage() << std::endl;
	toolbox::fsm::FailedEvent& failedEvent = dynamic_cast<toolbox::fsm::FailedEvent&>(*e);
	std::ostringstream error;
	error << "Failure performing transition from "
			<< failedEvent.getFromState()
			<<  " to "
			<< failedEvent.getToState()
			<< " exception: " << failedEvent.getException().what();
	__COUT_ERR__<< error.str() << std::endl;
	//diagService_->reportError(errstr.str(),DIAGERROR);

}

//========================================================================================================================
void CoreSupervisorBase::transitionConfiguring(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "transitionConfiguring" << std::endl;

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


	//Now that the configuration manager has all the necessary configurations I can create all objects dependent of the configuration

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->configure();
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while configuring: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"CoreSupervisorBase::transitionConfiguring" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
				);
	}

}

//========================================================================================================================
//transitionHalting
//	Ignore errors if coming from Failed state
void CoreSupervisorBase::transitionHalting(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "transitionHalting" << std::endl;

	for(auto& it: theStateMachineImplementation_)
	{
		try
		{
			it->halt();
		}
		catch(const std::runtime_error& e)
		{
			//if halting from Failed state, then ignore errors
			if(theStateMachine_.getProvenanceStateName() ==
					RunControlStateMachine::FAILED_STATE_NAME)
			{
				__COUT_INFO__ << "Error was caught while halting (but ignoring because previous state was '" <<
						RunControlStateMachine::FAILED_STATE_NAME << "'): " << e.what() << std::endl;
			}
			else //if not previously in Failed state, then fail
			{
				__SS__ << "Error was caught while halting: " << e.what() << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				theStateMachine_.setErrorMessage(ss.str());
				throw toolbox::fsm::exception::Exception(
						"Transition Error" /*name*/,
						ss.str() /* message*/,
						"CoreSupervisorBase::transitionHalting" /*module*/,
						__LINE__ /*line*/,
						__FUNCTION__ /*function*/
						);
			}
		}
	}
}

//========================================================================================================================
void CoreSupervisorBase::transitionInitializing(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "transitionInitializing" << std::endl;

	//    for(auto& it: theStateMachineImplementation_)
	//it->initialize();
}

//========================================================================================================================
void CoreSupervisorBase::transitionPausing(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "transitionPausing" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->pause();
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while pausing: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"CoreSupervisorBase::transitionPausing" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
		);
	}
}

//========================================================================================================================
void CoreSupervisorBase::transitionResuming(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	//NOTE: I want to first start the data manager first if this is a FEDataManagerSupervisor


	__COUT__ << "transitionResuming" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->resume();
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while resuming: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"CoreSupervisorBase::transitionResuming" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
		);
	}
}

//========================================================================================================================
void CoreSupervisorBase::transitionStarting(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{

	//NOTE: I want to first start the data manager first if this is a FEDataManagerSupervisor

	__COUT__ << "transitionStarting" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->start(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getParameters().getValue("RunNumber"));
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while starting: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"CoreSupervisorBase::transitionStarting" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
		);
	}
}

//========================================================================================================================
void CoreSupervisorBase::transitionStopping(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "transitionStopping" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->stop();
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while pausing: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"CoreSupervisorBase::transitionStopping" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
		);
	}
}
