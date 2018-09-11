#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

//#include "otsdaq-core/Singleton/Singleton.h"
//#include "otsdaq-core/FECore/FEVInterfacesManager.h"



#include <iostream>

using namespace ots;

//XDAQ_INSTANTIATOR_IMPL(CoreSupervisorBase)


const std::string								CoreSupervisorBase::WORK_LOOP_DONE 			= "Done";
const std::string								CoreSupervisorBase::WORK_LOOP_WORKING 		= "Working";

//========================================================================================================================
CoreSupervisorBase::CoreSupervisorBase(xdaq::ApplicationStub * s)
: xdaq::Application             (s)
, SOAPMessenger                 (this)
, CorePropertySupervisorBase	(this)
, RunControlStateMachine		(CorePropertySupervisorBase::allSupervisorInfo_.isWizardMode()? //set state machine name
		CorePropertySupervisorBase::supervisorClassNoNamespace_:
		CorePropertySupervisorBase::supervisorClassNoNamespace_ + ":" + CorePropertySupervisorBase::supervisorApplicationUID_)
, stateMachineWorkLoopManager_  (toolbox::task::bind(this, &CoreSupervisorBase::stateMachineThread, "StateMachine"))
, stateMachineSemaphore_        (toolbox::BSem::FULL)
//, theConfigurationManager_      (new ConfigurationManager)//(Singleton<ConfigurationManager>::getInstance()) //I always load the full config but if I want to load a partial configuration (new ConfigurationManager)
//, supervisorClass_              (getApplicationDescriptor()->getClassName())
//, supervisorClassNoNamespace_   (supervisorClass_.substr(supervisorClass_.find_last_of(":")+1, supervisorClass_.length()-supervisorClass_.find_last_of(":")))
, theRemoteWebUsers_ 			(this)
//, propertiesAreSetup_			(false)
{
	INIT_MF("CoreSupervisorBase");

	__SUP_COUT__ << "Begin!" << std::endl;

	xgi::bind (this, &CoreSupervisorBase::defaultPageWrapper,     "Default" );
	xgi::bind (this, &CoreSupervisorBase::requestWrapper, 		  "Request");

	xgi::bind (this, &CoreSupervisorBase::stateMachineXgiHandler, "StateMachineXgiHandler");

	xoap::bind(this, &CoreSupervisorBase::stateMachineStateRequest,    		"StateMachineStateRequest",    		XDAQ_NS_URI );
	xoap::bind(this, &CoreSupervisorBase::stateMachineErrorMessageRequest,  "StateMachineErrorMessageRequest",  XDAQ_NS_URI );
	//xoap::bind(this, &CoreSupervisorBase::macroMakerSupervisorRequest, 		"MacroMakerSupervisorRequest", 		XDAQ_NS_URI ); //moved to only FESupervisor!
	xoap::bind(this, &CoreSupervisorBase::workLoopStatusRequestWrapper, 	"WorkLoopStatusRequest",    		XDAQ_NS_URI );

	return;
}

//========================================================================================================================
CoreSupervisorBase::~CoreSupervisorBase(void)
{
	destroy();
}

//========================================================================================================================
void CoreSupervisorBase::destroy(void)
{
	__SUP_COUT__ << "Destroying..." << std::endl;
	for(auto& it: theStateMachineImplementation_)
		delete it;
	theStateMachineImplementation_.clear();
}

//========================================================================================================================
//wrapper for inheritance call
void CoreSupervisorBase::defaultPageWrapper(xgi::Input * in, xgi::Output * out )
{
	return defaultPage(in,out);
}

//========================================================================================================================
void CoreSupervisorBase::defaultPage(xgi::Input * in, xgi::Output * out )
{
	__SUP_COUT__<< "Supervisor class " << supervisorClass_ << std::endl;

	std::stringstream pagess;
	pagess << "/WebPath/html/" <<
			supervisorClassNoNamespace_ << ".html?urn=" <<
			this->getApplicationDescriptor()->getLocalId();

	__SUP_COUT__<< "Default page = " << pagess.str() << std::endl;

	*out << "<!DOCTYPE HTML><html lang='en'><frameset col='100%' row='100%'><frame src='" <<
			pagess.str() <<
			"'></frameset></html>";
}

//========================================================================================================================
//requestWrapper ~
//	wrapper for inheritance Supervisor request call
void CoreSupervisorBase::requestWrapper(xgi::Input * in, xgi::Output * out )

{
	//checkSupervisorPropertySetup();

	cgicc::Cgicc cgiIn(in);
	std::string requestType = CgiDataUtilities::getData(cgiIn,"RequestType");

	//__SUP_COUT__ << "requestType " << requestType << " files: " << cgiIn.getFiles().size() << std::endl;

	HttpXmlDocument xmlOut;
	WebUsers::RequestUserInfo userInfo(requestType,
			CgiDataUtilities::getOrPostData(cgiIn,"CookieCode"));

	CorePropertySupervisorBase::getRequestUserInfo(userInfo);

	if(!theRemoteWebUsers_.xmlRequestToGateway(
			cgiIn,
			out,
			&xmlOut,
			CorePropertySupervisorBase::allSupervisorInfo_,
			userInfo))
		return; //access failed


	//done checking cookieCode, sequence, userWithLock, and permissions access all in one shot!
	//**** end LOGIN GATEWAY CODE ***//

	if(!userInfo.automatedCommand_)
		__SUP_COUT__ << "requestType: " << requestType << __E__;

	if(userInfo.NonXMLRequestType_)
	{
		nonXmlRequest(requestType,cgiIn,*out,userInfo);
		return;
	}
	//else xml request type

	request(requestType,cgiIn,xmlOut,userInfo);

	//report any errors encountered
	{
		unsigned int occurance = 0;
		std::string err = xmlOut.getMatchingValue("Error",occurance++);
		while(err != "")
		{
			__SUP_COUT_ERR__ << "'" << requestType << "' ERROR encountered: " << err << std::endl;
			__MOUT_ERR__ << "'" << requestType << "' ERROR encountered: " << err << std::endl;
			err = xmlOut.getMatchingValue("Error",occurance++);
		}
	}


	//return xml doc holding server response
	xmlOut.outputXmlDocument((std::ostringstream*) out, false /*print to cout*/,
			!userInfo.NoXmlWhiteSpace_/*allow whitespace*/);

}

//========================================================================================================================
//request
//		Supervisors should override this function. It will be called after user access has been verified
//		according to the Supervisor Property settings. The CoreSupervisorBase class provides consistent
//		access, responses, and error handling across all inheriting supervisors that use ::request.
void CoreSupervisorBase::request(const std::string& requestType, cgicc::Cgicc& cgiIn, HttpXmlDocument& xmlOut,
		const WebUsers::RequestUserInfo& userInfo)
{
	__SUP_COUT__ << "This is the empty Core Supervisor request. Supervisors should override this function." << __E__;

// KEEP:
//	here are some possibly interesting example lines of code for overriding supervisors
//
//
//  if(requestType == "savePlanCommandSequence")
//	{
//		std::string 	planName 		= CgiDataUtilities::getData(cgiIn,"planName"); //from GET
//		std::string 	commands 		= CgiDataUtilities::postData(cgiIn,"commands"); //from POST
//
//		cgiIn.getFiles()
//		__SUP_COUT__ << "planName: " << planName << __E__;
//		__SUP_COUTV__(commands);
//
//
//	}
//	else
//	{
//		__SUP_SS__ << "requestType '" << requestType << "' request not recognized." << std::endl;
//		__SUP_COUT__ << "\n" << ss.str();
//		xmlOut.addTextElementToData("Error", ss.str());
//	}
//	xmlOut.addTextElementToData("Error",
//			"request encountered an error!");
}

//========================================================================================================================
//nonXmlRequest
//		Supervisors should override this function. It will be called after user access has been verified
//		according to the Supervisor Property settings. The CoreSupervisorBase class provides consistent
//		access, responses, and error handling across all inheriting supervisors that use ::request.
void CoreSupervisorBase::nonXmlRequest(const std::string& requestType, cgicc::Cgicc& cgiIn, std::ostream& out,
		const WebUsers::RequestUserInfo& userInfo)
{
	__SUP_COUT__ << "This is the empty Core Supervisor non-xml request. Supervisors should override this function." << __E__;
	out << "This is the empty Core Supervisor non-xml request. Supervisors should override this function." << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::stateMachineXgiHandler(xgi::Input * in, xgi::Output * out )

{}

//========================================================================================================================
void CoreSupervisorBase::stateMachineResultXgiHandler(xgi::Input* in, xgi::Output* out )

{}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineXoapHandler(xoap::MessageReference message )

{
	__SUP_COUT__<< "Soap Handler!" << std::endl;
	stateMachineWorkLoopManager_.removeProcessedRequests();
	stateMachineWorkLoopManager_.processRequest(message);
	__SUP_COUT__<< "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineResultXoapHandler(xoap::MessageReference message )

{
	__SUP_COUT__<< "Soap Handler!" << std::endl;
	//stateMachineWorkLoopManager_.removeProcessedRequests();
	//stateMachineWorkLoopManager_.processRequest(message);
	__SUP_COUT__<< "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
//indirection to allow for overriding handler
xoap::MessageReference CoreSupervisorBase::workLoopStatusRequestWrapper(xoap::MessageReference message)

{
	//this should have an override for monitoring work loops being done
	return workLoopStatusRequest(message);
} //end workLoopStatusRequest()

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::workLoopStatusRequest(xoap::MessageReference message)

{
	//this should have an override for monitoring work loops being done
	return SOAPUtilities::makeSOAPMessageReference(CoreSupervisorBase::WORK_LOOP_DONE);
} //end workLoopStatusRequest()

//========================================================================================================================
bool CoreSupervisorBase::stateMachineThread(toolbox::task::WorkLoop* workLoop)
{
	stateMachineSemaphore_.take();
	__SUP_COUT__<< "Re-sending message..." << SOAPUtilities::translate(stateMachineWorkLoopManager_.getMessage(workLoop)).getCommand() << std::endl;
	std::string reply = send(this->getApplicationDescriptor(),stateMachineWorkLoopManager_.getMessage(workLoop));
	stateMachineWorkLoopManager_.report(workLoop, reply, 100, true);
	__SUP_COUT__<< "Done with message" << std::endl;
	stateMachineSemaphore_.give();
	return false;//execute once and automatically remove the workloop so in WorkLoopManager the try workLoop->remove(job_) could be commented out
	//return true;//go on and then you must do the workLoop->remove(job_) in WorkLoopManager
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineStateRequest(xoap::MessageReference message)

{
	__SUP_COUT__<< "theStateMachine_.getCurrentStateName() = " << theStateMachine_.getCurrentStateName() << std::endl;
	return SOAPUtilities::makeSOAPMessageReference(theStateMachine_.getCurrentStateName());
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineErrorMessageRequest(xoap::MessageReference message)

{
	__SUP_COUT__<< "theStateMachine_.getErrorMessage() = " << theStateMachine_.getErrorMessage() << std::endl;

	SOAPParameters retParameters;
	retParameters.addParameter("ErrorMessage",theStateMachine_.getErrorMessage());
	return SOAPUtilities::makeSOAPMessageReference("stateMachineErrorMessageRequestReply",retParameters);
}

//========================================================================================================================
void CoreSupervisorBase::stateInitial(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateInitial" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateHalted(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateHalted" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateRunning(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateRunning" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateConfigured(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateConfigured" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::statePaused(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::statePaused" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::inError (toolbox::fsm::FiniteStateMachine & fsm)

{
	__SUP_COUT__<< "Fsm current state: " << theStateMachine_.getCurrentStateName()<< std::endl;
	//rcmsStateNotifier_.stateChanged("Error", "");
}

//========================================================================================================================
void CoreSupervisorBase::enteringError (toolbox::Event::Reference e)

{
//	__SUP_COUT__<< "Fsm current state: " << theStateMachine_.getCurrentStateName()
//			<< "\n\nError Message: " <<
//			theStateMachine_.getErrorMessage() << std::endl;
	toolbox::fsm::FailedEvent& failedEvent = dynamic_cast<toolbox::fsm::FailedEvent&>(*e);
	std::ostringstream error;
	error << "Failure performing transition from "
			<< failedEvent.getFromState()
			<<  " to "
			<< failedEvent.getToState()
			<< " exception: " << failedEvent.getException().what();
	__SUP_COUT_ERR__<< error.str() << std::endl;
	//diagService_->reportError(errstr.str(),DIAGERROR);

}

//========================================================================================================================
void CoreSupervisorBase::transitionConfiguring(toolbox::Event::Reference e)

{
	__SUP_COUT__ << "transitionConfiguring" << std::endl;

	std::pair<std::string /*group name*/, ConfigurationGroupKey> theGroup(
			SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).
			getParameters().getValue("ConfigurationGroupName"),
			ConfigurationGroupKey(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).
					getParameters().getValue("ConfigurationGroupKey")));

	__SUP_COUT__ << "Configuration group name: " << theGroup.first << " key: " <<
			theGroup.second << std::endl;

	theConfigurationManager_->loadConfigurationGroup(
			theGroup.first,
			theGroup.second, true);

	//Now that the configuration manager has all the necessary configurations, create all objects that depend on the configuration

	try
	{
		__SUP_COUT__ << "Configuring all state machine implementations..." << __E__;
		for(auto& it: theStateMachineImplementation_)
			it->configure();
		__SUP_COUT__ << "Done configuration all state machine implementations..." << __E__;
	}
	catch(const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while configuring: " << e.what() << std::endl;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"CoreSupervisorBase::transitionConfiguring" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
				);
	}
	catch(...)//const toolbox::fsm::exception::Exception& e)
	{
//		__SUP_SS__ << "Error was caught while configuring: " << e.what() << std::endl;
//		__SUP_COUT_ERR__ << "\n" << ss.str();
//		theStateMachine_.setErrorMessage(ss.str());
//		throw toolbox::fsm::exception::Exception(
//				"Transition Error" /*name*/,
//				ss.str() /* message*/,
//				"CoreSupervisorBase::transitionConfiguring" /*module*/,
//				__LINE__ /*line*/,
//				__FUNCTION__ /*function*/
//		);

		__SUP_SS__ << "Unknown error was caught while configuring. Please checked the logs." << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
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

{
	__SUP_COUT__ << "transitionHalting" << std::endl;

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
				__SUP_COUT_INFO__ << "Error was caught while halting (but ignoring because previous state was '" <<
						RunControlStateMachine::FAILED_STATE_NAME << "'): " << e.what() << std::endl;
			}
			else //if not previously in Failed state, then fail
			{
				__SUP_SS__ << "Error was caught while halting: " << e.what() << std::endl;
				__SUP_COUT_ERR__ << "\n" << ss.str();
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
//Inheriting supervisor classes should not override this function, or should at least also call it in the override
//	to maintain property functionality.
void CoreSupervisorBase::transitionInitializing(toolbox::Event::Reference e)

{
	__SUP_COUT__ << "transitionInitializing" << std::endl;

	CorePropertySupervisorBase::resetPropertiesAreSetup(); //indicate need to re-load user properties


	//Note: Do not initialize the state machine implementations... do any initializing in configure
	//	This allows re-instantiation at each configure time.
	//for(auto& it: theStateMachineImplementation_)
	//it->initialize();
}

//========================================================================================================================
void CoreSupervisorBase::transitionPausing(toolbox::Event::Reference e)

{
	__SUP_COUT__ << "transitionPausing" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->pause();
	}
	catch(const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while pausing: " << e.what() << std::endl;
		__SUP_COUT_ERR__ << "\n" << ss.str();
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

{
	//NOTE: I want to first start the data manager first if this is a FEDataManagerSupervisor


	__SUP_COUT__ << "transitionResuming" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->resume();
	}
	catch(const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while resuming: " << e.what() << std::endl;
		__SUP_COUT_ERR__ << "\n" << ss.str();
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

{

	//NOTE: I want to first start the data manager first if this is a FEDataManagerSupervisor

	__SUP_COUT__ << "transitionStarting" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->start(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getParameters().getValue("RunNumber"));
	}
	catch(const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while starting: " << e.what() << std::endl;
		__SUP_COUT_ERR__ << "\n" << ss.str();
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

{
	__SUP_COUT__ << "transitionStopping" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->stop();
	}
	catch(const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while pausing: " << e.what() << std::endl;
		__SUP_COUT_ERR__ << "\n" << ss.str();
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
