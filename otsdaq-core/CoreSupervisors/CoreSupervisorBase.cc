#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

#include <iostream>

using namespace ots;

// XDAQ_INSTANTIATOR_IMPL(CoreSupervisorBase)

const std::string CoreSupervisorBase::WORK_LOOP_DONE    = "Done";
const std::string CoreSupervisorBase::WORK_LOOP_WORKING = "Working";

//========================================================================================================================
CoreSupervisorBase::CoreSupervisorBase(xdaq::ApplicationStub* stub)
    : xdaq::Application(stub)
    , SOAPMessenger(this)
    , CorePropertySupervisorBase(this)
    , RunControlStateMachine(
          CorePropertySupervisorBase::allSupervisorInfo_.isWizardMode()
              ?  // set state machine name
              CorePropertySupervisorBase::supervisorClassNoNamespace_
              : CorePropertySupervisorBase::supervisorClassNoNamespace_ + ":" +
                    CorePropertySupervisorBase::getSupervisorUID())
    , stateMachineWorkLoopManager_(toolbox::task::bind(
          this, &CoreSupervisorBase::stateMachineThread, "StateMachine"))
    , stateMachineSemaphore_(toolbox::BSem::FULL)
    , theRemoteWebUsers_(this)
{
	__SUP_COUT__ << "Constructor." << __E__;

	INIT_MF("CoreSupervisorBase");

	xgi::bind(this, &CoreSupervisorBase::defaultPageWrapper, "Default");
	xgi::bind(this, &CoreSupervisorBase::requestWrapper, "Request");

	xgi::bind(
	    this, &CoreSupervisorBase::stateMachineXgiHandler, "StateMachineXgiHandler");

	xoap::bind(this,
	           &CoreSupervisorBase::stateMachineStateRequest,
	           "StateMachineStateRequest",
	           XDAQ_NS_URI);
	xoap::bind(this,
	           &CoreSupervisorBase::stateMachineErrorMessageRequest,
	           "StateMachineErrorMessageRequest",
	           XDAQ_NS_URI);
	xoap::bind(this,
	           &CoreSupervisorBase::workLoopStatusRequestWrapper,
	           "WorkLoopStatusRequest",
	           XDAQ_NS_URI);
	xoap::bind(this,
	           &CoreSupervisorBase::applicationStatusRequest,
	           "ApplicationStatusRequest",
	           XDAQ_NS_URI);

	__SUP_COUT__ << "Constructed." << __E__;
}  // end constructor

//========================================================================================================================
CoreSupervisorBase::~CoreSupervisorBase(void)
{
	__SUP_COUT__ << "Destructor." << __E__;
	destroy();
	__SUP_COUT__ << "Destructed." << __E__;
}  // end destructor()

//========================================================================================================================
void CoreSupervisorBase::destroy(void)
{
	__SUP_COUT__ << "Destroying..." << __E__;
	for(auto& it : theStateMachineImplementation_)
		delete it;
	theStateMachineImplementation_.clear();
}  // end destroy()

//========================================================================================================================
// wrapper for inheritance call
void CoreSupervisorBase::defaultPageWrapper(xgi::Input* in, xgi::Output* out)
{
	return defaultPage(in, out);
}

//========================================================================================================================
void CoreSupervisorBase::defaultPage(xgi::Input* in, xgi::Output* out)
{
	__SUP_COUT__ << "Supervisor class " << supervisorClass_ << __E__;

	std::stringstream pagess;
	pagess << "/WebPath/html/" << supervisorClassNoNamespace_
	       << ".html?urn=" << this->getApplicationDescriptor()->getLocalId();

	__SUP_COUT__ << "Default page = " << pagess.str() << __E__;

	*out << "<!DOCTYPE HTML><html lang='en'><frameset col='100%' row='100%'><frame src='"
	     << pagess.str() << "'></frameset></html>";
}

//========================================================================================================================
// requestWrapper ~
//	wrapper for inheritance Supervisor request call
void CoreSupervisorBase::requestWrapper(xgi::Input* in, xgi::Output* out)
{
	// checkSupervisorPropertySetup();

	cgicc::Cgicc cgiIn(in);
	std::string  requestType = CgiDataUtilities::getData(cgiIn, "RequestType");

	//	__SUP_COUT__ << "requestType " << requestType << " files: " <<
	//			cgiIn.getFiles().size() << __E__;

	HttpXmlDocument           xmlOut;
	WebUsers::RequestUserInfo userInfo(
	    requestType, CgiDataUtilities::getOrPostData(cgiIn, "CookieCode"));

	CorePropertySupervisorBase::getRequestUserInfo(userInfo);

	if(!theRemoteWebUsers_.xmlRequestToGateway(
	       cgiIn, out, &xmlOut, CorePropertySupervisorBase::allSupervisorInfo_, userInfo))
		return;  // access failed

	// done checking cookieCode, sequence, userWithLock, and permissions access all in one
	// shot!
	//**** end LOGIN GATEWAY CODE ***//

	if(!userInfo.automatedCommand_)
		__SUP_COUT__ << "requestType: " << requestType << __E__;

	if(userInfo.NonXMLRequestType_)
	{
		try
		{
			nonXmlRequest(requestType, cgiIn, *out, userInfo);
		}
		catch(const std::runtime_error& e)
		{
			__SUP_SS__ << "An error was encountered handling requestType '" << requestType
			           << "':" << e.what() << __E__;
			__SUP_COUT_ERR__ << "\n" << ss.str();
			__SUP_MOUT_ERR__ << "\n" << ss.str();
		}
		catch(...)
		{
			__SUP_SS__ << "An unknown error was encountered handling requestType '"
			           << requestType << ".' "
			           << "Please check the printouts to debug." << __E__;
			__SUP_COUT_ERR__ << "\n" << ss.str();
			__SUP_MOUT_ERR__ << "\n" << ss.str();
		}
		return;
	}
	// else xml request type

	try
	{
		// call derived class' request()
		request(requestType, cgiIn, xmlOut, userInfo);
	}
	catch(const std::runtime_error& e)
	{
		__SUP_SS__ << "An error was encountered handling requestType '" << requestType
		           << "':" << e.what() << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		xmlOut.addTextElementToData("Error", ss.str());
	}
	catch(...)
	{
		__SUP_SS__ << "An unknown error was encountered handling requestType '"
		           << requestType << ".' "
		           << "Please check the printouts to debug." << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		xmlOut.addTextElementToData("Error", ss.str());
	}

	// report any errors encountered
	{
		unsigned int occurance = 0;
		std::string  err       = xmlOut.getMatchingValue("Error", occurance++);
		while(err != "")
		{
			__SUP_COUT_ERR__ << "'" << requestType << "' ERROR encountered: " << err
			                 << __E__;
			__SUP_MOUT_ERR__ << "'" << requestType << "' ERROR encountered: " << err
			                 << __E__;
			err = xmlOut.getMatchingValue("Error", occurance++);
		}
	}

	// return xml doc holding server response
	xmlOut.outputXmlDocument((std::ostringstream*)out,
	                         false /*print to cout*/,
	                         !userInfo.NoXmlWhiteSpace_ /*allow whitespace*/);
}

//========================================================================================================================
// request
//		Supervisors should override this function. It will be called after user access has
// been verified 		according to the Supervisor Property settings. The
// CoreSupervisorBase class provides consistent 		access, responses, and error
// handling  across all inheriting supervisors that use ::request.
void CoreSupervisorBase::request(const std::string&               requestType,
                                 cgicc::Cgicc&                    cgiIn,
                                 HttpXmlDocument&                 xmlOut,
                                 const WebUsers::RequestUserInfo& userInfo)
{
	__SUP_SS__ << "This is the empty Core Supervisor request. Supervisors should "
	              "override this function."
	           << __E__;
	__SUP_COUT__ << ss.str();
	xmlOut.addTextElementToData("Error", ss.str());
	return;

	// KEEP:
	//	here are some possibly interesting example lines of code for overriding
	// supervisors
	//
	// try
	//{
	//
	//  if(requestType == "savePlanCommandSequence")
	//	{
	//		std::string 	planName 		= CgiDataUtilities::getData(cgiIn,"planName");
	////from  GET 		std::string 	commands 		=
	// CgiDataUtilities::postData(cgiIn,"commands");
	////from POST
	//
	//		cgiIn.getFiles()
	//		__SUP_COUT__ << "planName: " << planName << __E__;
	//		__SUP_COUTV__(commands);
	//
	//
	//	}
	//	else
	//	{
	//		__SUP_SS__ << "requestType '" << requestType << "' request not recognized." <<
	// __E__;
	//		__SUP_SS_THROW__;
	//	}
	//	xmlOut.addTextElementToData("Error",
	//			"request encountered an error!");
	//}
	//	catch(const std::runtime_error& e)
	//	{
	//		__SUP_SS__ << "An error was encountered handling requestType '" << requestType
	//<< "':" << 				e.what() << __E__;
	//		__SUP_COUT_ERR__ << "\n" << ss.str();
	//		xmlOut.addTextElementToData("Error", ss.str());
	//	}
	//	catch(...)
	//	{
	//		__SUP_SS__ << "An unknown error was encountered handling requestType '" <<
	// requestType << ".' " << 				"Please check the printouts to debug." <<
	//__E__;
	//		__SUP_COUT_ERR__ << "\n" << ss.str();
	//		xmlOut.addTextElementToData("Error", ss.str());
	//	}
	// END KEEP.
}  // end request()

//========================================================================================================================
// nonXmlRequest
//		Supervisors should override this function. It will be called after user access has
// been verified 		according to the Supervisor Property settings. The
// CoreSupervisorBase class provides consistent 		access, responses, and error
// handling  across all inheriting supervisors that use ::request.
void CoreSupervisorBase::nonXmlRequest(const std::string&               requestType,
                                       cgicc::Cgicc&                    cgiIn,
                                       std::ostream&                    out,
                                       const WebUsers::RequestUserInfo& userInfo)
{
	__SUP_COUT__ << "This is the empty Core Supervisor non-xml request. Supervisors "
	                "should override this function."
	             << __E__;
	out << "This is the empty Core Supervisor non-xml request. Supervisors should "
	       "override this function."
	    << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::stateMachineXgiHandler(xgi::Input* in, xgi::Output* out) {}

//========================================================================================================================
void CoreSupervisorBase::stateMachineResultXgiHandler(xgi::Input* in, xgi::Output* out) {}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineXoapHandler(
    xoap::MessageReference message)

{
	__SUP_COUT__ << "Soap Handler!" << __E__;
	stateMachineWorkLoopManager_.removeProcessedRequests();
	stateMachineWorkLoopManager_.processRequest(message);
	__SUP_COUT__ << "Done - Soap Handler!" << __E__;
	return message;
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineResultXoapHandler(
    xoap::MessageReference message)

{
	__SUP_COUT__ << "Soap Handler!" << __E__;
	// stateMachineWorkLoopManager_.removeProcessedRequests();
	// stateMachineWorkLoopManager_.processRequest(message);
	__SUP_COUT__ << "Done - Soap Handler!" << __E__;
	return message;
}

//========================================================================================================================
// indirection to allow for overriding handler
xoap::MessageReference CoreSupervisorBase::workLoopStatusRequestWrapper(
    xoap::MessageReference message)

{
	// this should have an override for monitoring work loops being done
	return workLoopStatusRequest(message);
}  // end workLoopStatusRequest()

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::workLoopStatusRequest(
    xoap::MessageReference message)

{
	// this should have an override for monitoring work loops being done
	return SOAPUtilities::makeSOAPMessageReference(
		CoreSupervisorBase::WORK_LOOP_DONE);
}  // end workLoopStatusRequest()

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::applicationStatusRequest(
    xoap::MessageReference message)

{
	// send back status and progress parameters
	std::string status = theStateMachine_.getCurrentStateName();
	std::string progress = RunControlStateMachine::theProgressBar_.readPercentageString();

	if(theStateMachine_.isInTransition())
	{
		// return the ProvenanceStateName
		status = theStateMachine_.getProvenanceStateName();
		// std::string transition = theStateMachine_.getTransitionName(theStateMachine_.getCurrentStateName(), //getProvenanceStateName
		// 	SOAPUtilities::translate(theStateMachine_.theMessage_).getCommand());
		// __COUTV__(transition);
	}

	else
	{
		status = theStateMachine_.getCurrentStateName();
	}

	SOAPParameters retParameters;
	retParameters.addParameter("Status", status); 
	retParameters.addParameter("Progress", progress);


	return SOAPUtilities::makeSOAPMessageReference("applicationStatusRequestReply", retParameters); 
}  // end applicationStatusRequest()

//========================================================================================================================
bool CoreSupervisorBase::stateMachineThread(toolbox::task::WorkLoop* workLoop)
{
	stateMachineSemaphore_.take();
	__SUP_COUT__ << "Re-sending message..."
	             << SOAPUtilities::translate(
	                    stateMachineWorkLoopManager_.getMessage(workLoop))
	                    .getCommand()
	             << __E__;
	std::string reply = send(this->getApplicationDescriptor(),
	                         stateMachineWorkLoopManager_.getMessage(workLoop));
	stateMachineWorkLoopManager_.report(workLoop, reply, 100, true);
	__SUP_COUT__ << "Done with message" << __E__;
	stateMachineSemaphore_.give();
	return false;  // execute once and automatically remove the workloop so in
	               // WorkLoopManager the try workLoop->remove(job_) could be commented
	               // out return true;//go on and then you must do the
	               // workLoop->remove(job_) in WorkLoopManager
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineStateRequest(
    xoap::MessageReference message)

{
	__SUP_COUT__ << "theStateMachine_.getCurrentStateName() = "
	             << theStateMachine_.getCurrentStateName() << __E__;
	return SOAPUtilities::makeSOAPMessageReference(
	    theStateMachine_.getCurrentStateName());
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineErrorMessageRequest(
    xoap::MessageReference message)

{
	__SUP_COUT__ << "theStateMachine_.getErrorMessage() = "
	             << theStateMachine_.getErrorMessage() << __E__;

	SOAPParameters retParameters;
	retParameters.addParameter("ErrorMessage", theStateMachine_.getErrorMessage());
	return SOAPUtilities::makeSOAPMessageReference("stateMachineErrorMessageRequestReply",
	                                               retParameters);
}

//========================================================================================================================
void CoreSupervisorBase::stateInitial(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateInitial" << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::stateHalted(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateHalted" << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::stateRunning(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateRunning" << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::stateConfigured(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateConfigured" << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::statePaused(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::statePaused" << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::inError(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName()
	             << __E__;
	// rcmsStateNotifier_.stateChanged("Error", "");
}

//========================================================================================================================
void CoreSupervisorBase::enteringError(toolbox::Event::Reference e)

{
	//	__SUP_COUT__<< "Fsm current state: " << theStateMachine_.getCurrentStateName()
	//			<< "\n\nError Message: " <<
	//			theStateMachine_.getErrorMessage() << __E__;
	toolbox::fsm::FailedEvent& failedEvent = dynamic_cast<toolbox::fsm::FailedEvent&>(*e);
	std::ostringstream         error;
	error << "Failure performing transition from " << failedEvent.getFromState() << " to "
	      << failedEvent.getToState()
	      << " exception: " << failedEvent.getException().what();
	__SUP_COUT_ERR__ << error.str() << __E__;
	// diagService_->reportError(errstr.str(),DIAGERROR);
}

//========================================================================================================================
void CoreSupervisorBase::preStateMachineExecutionLoop(void)
{
	RunControlStateMachine::clearIterationWork();
	RunControlStateMachine::clearSubIterationWork();

	stateMachinesIterationWorkCount_ = 0;

	if(RunControlStateMachine::getIterationIndex() == 0 &&
	   RunControlStateMachine::getSubIterationIndex() == 0)
	{
		// reset vector for iterations done on first iteration

		subIterationWorkStateMachineIndex_ = -1;  // clear sub iteration work index

		stateMachinesIterationDone_.resize(theStateMachineImplementation_.size());
		for(unsigned int i = 0; i < stateMachinesIterationDone_.size(); ++i)
			stateMachinesIterationDone_[i] = false;
	}
	else
		__SUP_COUT__ << "Iteration " << RunControlStateMachine::getIterationIndex() << "."
		             << RunControlStateMachine::getSubIterationIndex() << "("
		             << subIterationWorkStateMachineIndex_ << ")" << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::preStateMachineExecution(unsigned int i)
{
	if(i >= theStateMachineImplementation_.size())
	{
		__SUP_SS__ << "State Machine " << i << " not found!" << __E__;
		__SUP_SS_THROW__;
	}

	theStateMachineImplementation_[i]->VStateMachine::setIterationIndex(
	    RunControlStateMachine::getIterationIndex());
	theStateMachineImplementation_[i]->VStateMachine::setSubIterationIndex(
	    RunControlStateMachine::getSubIterationIndex());

	theStateMachineImplementation_[i]->VStateMachine::clearIterationWork();
	theStateMachineImplementation_[i]->VStateMachine::clearSubIterationWork();

	__SUP_COUT__
	    << "theStateMachineImplementation Iteration "
	    << theStateMachineImplementation_[i]->VStateMachine::getIterationIndex() << "."
	    << theStateMachineImplementation_[i]->VStateMachine::getSubIterationIndex()
	    << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::postStateMachineExecution(unsigned int i)
{
	if(i >= theStateMachineImplementation_.size())
	{
		__SUP_SS__ << "State Machine " << i << " not found!" << __E__;
		__SUP_SS_THROW__;
	}

	// sub-iteration has priority
	if(theStateMachineImplementation_[i]->VStateMachine::getSubIterationWork())
	{
		subIterationWorkStateMachineIndex_ = i;
		RunControlStateMachine::indicateSubIterationWork();

		__SUP_COUT__ << "State machine " << i
		             << " is flagged for another sub-iteration..." << __E__;
	}
	else
	{
		stateMachinesIterationDone_[i] =
		    !theStateMachineImplementation_[i]->VStateMachine::getIterationWork();

		if(!stateMachinesIterationDone_[i])
		{
			__SUP_COUT__ << "State machine " << i
			             << " is flagged for another iteration..." << __E__;
			RunControlStateMachine::indicateIterationWork();  // mark not done at
			                                                  // CoreSupervisorBase level
			++stateMachinesIterationWorkCount_;  // increment still working count
		}
	}
}

//========================================================================================================================
void CoreSupervisorBase::postStateMachineExecutionLoop(void)
{
	if(RunControlStateMachine::subIterationWorkFlag_)
		__SUP_COUT__ << "State machine implementation "
		             << subIterationWorkStateMachineIndex_
		             << " is flagged for another sub-iteration..." << __E__;
	else if(RunControlStateMachine::iterationWorkFlag_)
		__SUP_COUT__
		    << stateMachinesIterationWorkCount_
		    << " state machine implementation(s) flagged for another iteration..."
		    << __E__;
	else
		__SUP_COUT__ << "Done configuration all state machine implementations..."
		             << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::transitionConfiguring(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "transitionConfiguring" << __E__;

	// activate the configuration tree (the first iteration)
	if(RunControlStateMachine::getIterationIndex() == 0 &&
	   RunControlStateMachine::getSubIterationIndex() == 0)
	{
		std::pair<std::string /*group name*/, TableGroupKey> theGroup(
		    SOAPUtilities::translate(theStateMachine_.getCurrentMessage())
		        .getParameters()
		        .getValue("ConfigurationTableGroupName"),
		    TableGroupKey(SOAPUtilities::translate(theStateMachine_.getCurrentMessage())
		                      .getParameters()
		                      .getValue("ConfigurationTableGroupKey")));

		__SUP_COUT__ << "Configuration table group name: " << theGroup.first
		             << " key: " << theGroup.second << __E__;

		theConfigurationManager_->loadTableGroup(
		    theGroup.first, theGroup.second, true /*doActivate*/);
	}

	// Now that the configuration manager has all the necessary configurations,
	//	create all objects that depend on the configuration (the first iteration)

	try
	{
		__SUP_COUT__ << "Configuring all state machine implementations..." << __E__;
		preStateMachineExecutionLoop();
		for(unsigned int i = 0; i < theStateMachineImplementation_.size(); ++i)
		{
			// if one state machine is doing a sub-iteration, then target that one
			if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			   i != subIterationWorkStateMachineIndex_)
				continue;  // skip those not in the sub-iteration

			if(stateMachinesIterationDone_[i])
				continue;  // skip state machines already done

			preStateMachineExecution(i);
			theStateMachineImplementation_[i]->parentSupervisor_ =
			    this;  // for backwards compatibility, kept out of configure parameters
			theStateMachineImplementation_[i]->configure();  // e.g. for FESupervisor,
			                                                 // this is configure of
			                                                 // FEVInterfacesManager
			postStateMachineExecution(i);
		}
		postStateMachineExecutionLoop();
	}
	catch(const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while configuring: " << e.what() << __E__;
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
	catch(...)
	{
		__SUP_SS__
		    << "Unknown error was caught while configuring. Please checked the logs."
		    << __E__;
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
}  // end transitionConfiguring()

//========================================================================================================================
// transitionHalting
//	Ignore errors if coming from Failed state
void CoreSupervisorBase::transitionHalting(toolbox::Event::Reference e)
{
	const std::string transitionName = "Halting";
	try
	{
		__SUP_COUT__ << transitionName << " all state machine implementations..."
		             << __E__;
		preStateMachineExecutionLoop();
		for(unsigned int i = 0; i < theStateMachineImplementation_.size(); ++i)
		{
			// if one state machine is doing a sub-iteration, then target that one
			if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			   i != subIterationWorkStateMachineIndex_)
				continue;  // skip those not in the sub-iteration

			if(stateMachinesIterationDone_[i])
				continue;  // skip state machines already done

			preStateMachineExecution(i);
			theStateMachineImplementation_[i]->halt();  // e.g. for FESupervisor, this is
			                                            // transition of
			                                            // FEVInterfacesManager
			postStateMachineExecution(i);
		}
		postStateMachineExecutionLoop();
	}
	catch(const std::runtime_error& e)
	{
		// if halting from Failed state, then ignore errors
		if(theStateMachine_.getProvenanceStateName() ==
		   RunControlStateMachine::FAILED_STATE_NAME)
		{
			__SUP_COUT_INFO__ << "Error was caught while halting (but ignoring because "
			                     "previous state was '"
			                  << RunControlStateMachine::FAILED_STATE_NAME
			                  << "'): " << e.what() << __E__;
		}
		else  // if not previously in Failed state, then fail
		{
			__SUP_SS__ << "Error was caught while " << transitionName << ": " << e.what()
			           << __E__;
			__SUP_COUT_ERR__ << "\n" << ss.str();
			theStateMachine_.setErrorMessage(ss.str());
			throw toolbox::fsm::exception::Exception(
			    "Transition Error" /*name*/,
			    ss.str() /* message*/,
			    "CoreSupervisorBase::transition" + transitionName /*module*/,
			    __LINE__ /*line*/,
			    __FUNCTION__ /*function*/
			);
		}
	}
	catch(...)
	{
		// if halting from Failed state, then ignore errors
		if(theStateMachine_.getProvenanceStateName() ==
		   RunControlStateMachine::FAILED_STATE_NAME)
		{
			__SUP_COUT_INFO__ << "Unknown error was caught while halting (but ignoring "
			                     "because previous state was '"
			                  << RunControlStateMachine::FAILED_STATE_NAME << "')."
			                  << __E__;
		}
		else  // if not previously in Failed state, then fail
		{
			__SUP_SS__ << "Unknown error was caught while " << transitionName
			           << ". Please checked the logs." << __E__;
			__SUP_COUT_ERR__ << "\n" << ss.str();
			theStateMachine_.setErrorMessage(ss.str());
			throw toolbox::fsm::exception::Exception(
			    "Transition Error" /*name*/,
			    ss.str() /* message*/,
			    "CoreSupervisorBase::transition" + transitionName /*module*/,
			    __LINE__ /*line*/,
			    __FUNCTION__ /*function*/
			);
		}
	}
}  // end transitionHalting()

//========================================================================================================================
// Inheriting supervisor classes should not override this function, or should at least
// also call it in the override 	to maintain property functionality.
void CoreSupervisorBase::transitionInitializing(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "transitionInitializing" << __E__;

	CorePropertySupervisorBase::resetPropertiesAreSetup();  // indicate need to re-load
	                                                        // user properties

	// Note: Do not initialize the state machine implementations...
	//	do any initializing in configure
	//	This allows re-instantiation at each configure time.
	// for(auto& it: theStateMachineImplementation_)
	// it->initialize();
}  // end transitionInitializing()

//========================================================================================================================
void CoreSupervisorBase::transitionPausing(toolbox::Event::Reference e)
{
	const std::string transitionName = "Pausing";
	try
	{
		__SUP_COUT__ << "Configuring all state machine implementations..." << __E__;
		preStateMachineExecutionLoop();
		for(unsigned int i = 0; i < theStateMachineImplementation_.size(); ++i)
		{
			// if one state machine is doing a sub-iteration, then target that one
			if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			   i != subIterationWorkStateMachineIndex_)
				continue;  // skip those not in the sub-iteration

			if(stateMachinesIterationDone_[i])
				continue;  // skip state machines already done

			preStateMachineExecution(i);
			theStateMachineImplementation_[i]->pause();  // e.g. for FESupervisor, this is
			                                             // transition of
			                                             // FEVInterfacesManager
			postStateMachineExecution(i);
		}
		postStateMachineExecutionLoop();
	}
	catch(const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while " << transitionName << ": " << e.what()
		           << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
	catch(...)
	{
		__SUP_SS__ << "Unknown error was caught while " << transitionName
		           << ". Please checked the logs." << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
}  // end transitionPausing()

//========================================================================================================================
void CoreSupervisorBase::transitionResuming(toolbox::Event::Reference e)
{
	const std::string transitionName = "Resuming";
	try
	{
		__SUP_COUT__ << "Configuring all state machine implementations..." << __E__;
		preStateMachineExecutionLoop();
		for(unsigned int i = 0; i < theStateMachineImplementation_.size(); ++i)
		{
			// if one state machine is doing a sub-iteration, then target that one
			if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			   i != subIterationWorkStateMachineIndex_)
				continue;  // skip those not in the sub-iteration

			if(stateMachinesIterationDone_[i])
				continue;  // skip state machines already done

			preStateMachineExecution(i);
			theStateMachineImplementation_[i]->resume();  // e.g. for FESupervisor, this
			                                              // is transition of
			                                              // FEVInterfacesManager
			postStateMachineExecution(i);
		}
		postStateMachineExecutionLoop();
	}
	catch(const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while " << transitionName << ": " << e.what()
		           << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
	catch(...)
	{
		__SUP_SS__ << "Unknown error was caught while " << transitionName
		           << ". Please checked the logs." << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
}  // end transitionResuming()

//========================================================================================================================
void CoreSupervisorBase::transitionStarting(toolbox::Event::Reference e)
{
	const std::string transitionName = "Starting";
	const std::string runNumber =
	    SOAPUtilities::translate(theStateMachine_.getCurrentMessage())
	        .getParameters()
	        .getValue("RunNumber");
	try
	{
		__SUP_COUT__ << "Configuring all state machine implementations..." << __E__;
		preStateMachineExecutionLoop();
		for(unsigned int i = 0; i < theStateMachineImplementation_.size(); ++i)
		{
			// if one state machine is doing a sub-iteration, then target that one
			if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			   i != subIterationWorkStateMachineIndex_)
				continue;  // skip those not in the sub-iteration

			if(stateMachinesIterationDone_[i])
				continue;  // skip state machines already done

			preStateMachineExecution(i);
			theStateMachineImplementation_[i]->start(runNumber);  // e.g. for
			                                                      // FESupervisor, this is
			                                                      // transition of
			                                                      // FEVInterfacesManager
			postStateMachineExecution(i);
		}
		postStateMachineExecutionLoop();
	}
	catch(const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while " << transitionName << ": " << e.what()
		           << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
	catch(...)
	{
		__SUP_SS__ << "Unknown error was caught while " << transitionName
		           << ". Please checked the logs." << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
}  // end transitionStarting()

//========================================================================================================================
void CoreSupervisorBase::transitionStopping(toolbox::Event::Reference e)
{
	const std::string transitionName = "Stopping";
	try
	{
		__SUP_COUT__ << "Configuring all state machine implementations..." << __E__;
		preStateMachineExecutionLoop();
		for(unsigned int i = 0; i < theStateMachineImplementation_.size(); ++i)
		{
			// if one state machine is doing a sub-iteration, then target that one
			if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			   i != subIterationWorkStateMachineIndex_)
				continue;  // skip those not in the sub-iteration

			if(stateMachinesIterationDone_[i])
				continue;  // skip state machines already done

			preStateMachineExecution(i);
			theStateMachineImplementation_[i]->stop();  // e.g. for FESupervisor, this is
			                                            // transition of
			                                            // FEVInterfacesManager
			postStateMachineExecution(i);
		}
		postStateMachineExecutionLoop();
	}
	catch(const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while " << transitionName << ": " << e.what()
		           << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
	catch(...)
	{
		__SUP_SS__ << "Unknown error was caught while " << transitionName
		           << ". Please checked the logs." << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
}  // end transitionStopping()


//========================================================================================================================
// SendAsyncErrorToGateway
//	Static -- thread
//	Send async error or soft error to gateway
//	Call this as thread so that parent calling function (workloop) can end.
void CoreSupervisorBase::sendAsyncErrorToGateway(
                                           const std::string& errorMessage,
                                           bool               isSoftError) try
{

	if(isSoftError)
		__SUP_COUT_ERR__ << "Sending Supervisor Async SOFT Running Error... \n"
		             << errorMessage << __E__;
	else
		__SUP_COUT_ERR__ << "Sending Supervisor Async Running Error... \n"
		             << errorMessage << __E__;


	theStateMachine_.setErrorMessage(errorMessage);

	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* gatewaySupervisor =
	    allSupervisorInfo_.getGatewayInfo().getDescriptor();

	SOAPParameters parameters;
	parameters.addParameter("ErrorMessage", errorMessage);

	xoap::MessageReference replyMessage =
	    SOAPMessenger::sendWithSOAPReply(
	        gatewaySupervisor, isSoftError ? "AsyncSoftError" : "AsyncError", parameters);

	std::stringstream replyMessageSStream;
	replyMessageSStream << SOAPUtilities::translate(replyMessage);
	__SUP_COUT__ << "Received... " << replyMessageSStream.str() << std::endl;

	if(replyMessageSStream.str().find("Fault") != std::string::npos)
	{
		__SUP_COUT_ERR__ << "Failure to indicate fault to Gateway..." << __E__;
		throw;
	}
}
catch(const xdaq::exception::Exception& e)
{
	if(isSoftError)
		__SUP_COUT__ << "SOAP message failure indicating Supervisor asynchronous running SOFT "
		            "error back to Gateway: "
		         << e.what() << __E__;
	else
		__SUP_COUT__ << "SOAP message failure indicating Supervisor asynchronous running "
		            "error back to Gateway: "
		         << e.what() << __E__;
	throw; //rethrow and hope error is noticed
}
catch(...)
{
	if(isSoftError)
		__SUP_COUT__ << "Unknown error encounter indicating Supervisor asynchronous running "
		            "SOFT error back to Gateway."
		         << __E__;
	else
		__SUP_COUT__ << "Unknown error encounter indicating Supervisor asynchronous running "
		            "error back to Gateway."
		         << __E__;
	throw; //rethrow and hope error is noticed
}  // end SendAsyncErrorToGateway()
