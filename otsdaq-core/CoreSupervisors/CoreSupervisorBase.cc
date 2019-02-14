#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

#include <iostream>

using namespace ots;

//XDAQ_INSTANTIATOR_IMPL(CoreSupervisorBase)

const std::string CoreSupervisorBase::WORK_LOOP_DONE    = "Done";
const std::string CoreSupervisorBase::WORK_LOOP_WORKING = "Working";

//========================================================================================================================
CoreSupervisorBase::CoreSupervisorBase (xdaq::ApplicationStub* s)
    : xdaq::Application (s)
    , SOAPMessenger (this)
    , CorePropertySupervisorBase (this)
    , RunControlStateMachine (CorePropertySupervisorBase::allSupervisorInfo_.isWizardMode () ?  //set state machine name
                                  CorePropertySupervisorBase::supervisorClassNoNamespace_
                                                                                             : CorePropertySupervisorBase::supervisorClassNoNamespace_ + ":" +
                                                                                                   CorePropertySupervisorBase::supervisorApplicationUID_)
    , stateMachineWorkLoopManager_ (toolbox::task::bind (this, &CoreSupervisorBase::stateMachineThread, "StateMachine"))
    , stateMachineSemaphore_ (toolbox::BSem::FULL)
    , theRemoteWebUsers_ (this)
{
	INIT_MF ("CoreSupervisorBase");

	__SUP_COUT__ << "Begin!" << std::endl;

	xgi::bind (this, &CoreSupervisorBase::defaultPageWrapper, "Default");
	xgi::bind (this, &CoreSupervisorBase::requestWrapper, "Request");

	xgi::bind (this, &CoreSupervisorBase::stateMachineXgiHandler, "StateMachineXgiHandler");

	xoap::bind (this, &CoreSupervisorBase::stateMachineStateRequest, "StateMachineStateRequest", XDAQ_NS_URI);
	xoap::bind (this, &CoreSupervisorBase::stateMachineErrorMessageRequest, "StateMachineErrorMessageRequest", XDAQ_NS_URI);
	xoap::bind (this, &CoreSupervisorBase::workLoopStatusRequestWrapper, "WorkLoopStatusRequest", XDAQ_NS_URI);

	return;
}

//========================================================================================================================
CoreSupervisorBase::~CoreSupervisorBase (void)
{
	destroy ();
}

//========================================================================================================================
void CoreSupervisorBase::destroy (void)
{
	__SUP_COUT__ << "Destroying..." << std::endl;
	for (auto& it : theStateMachineImplementation_)
		delete it;
	theStateMachineImplementation_.clear ();
}

//========================================================================================================================
//wrapper for inheritance call
void CoreSupervisorBase::defaultPageWrapper (xgi::Input* in, xgi::Output* out)
{
	return defaultPage (in, out);
}

//========================================================================================================================
void CoreSupervisorBase::defaultPage (xgi::Input* in, xgi::Output* out)
{
	__SUP_COUT__ << "Supervisor class " << supervisorClass_ << std::endl;

	std::stringstream pagess;
	pagess << "/WebPath/html/" << supervisorClassNoNamespace_ << ".html?urn=" << this->getApplicationDescriptor ()->getLocalId ();

	__SUP_COUT__ << "Default page = " << pagess.str () << std::endl;

	*out << "<!DOCTYPE HTML><html lang='en'><frameset col='100%' row='100%'><frame src='" << pagess.str () << "'></frameset></html>";
}

//========================================================================================================================
//requestWrapper ~
//	wrapper for inheritance Supervisor request call
void CoreSupervisorBase::requestWrapper (xgi::Input* in, xgi::Output* out)

{
	//checkSupervisorPropertySetup();

	cgicc::Cgicc cgiIn (in);
	std::string  requestType = CgiDataUtilities::getData (cgiIn, "RequestType");

	//__SUP_COUT__ << "requestType " << requestType << " files: " << cgiIn.getFiles().size() << std::endl;

	HttpXmlDocument           xmlOut;
	WebUsers::RequestUserInfo userInfo (requestType,
	                                    CgiDataUtilities::getOrPostData (cgiIn, "CookieCode"));

	CorePropertySupervisorBase::getRequestUserInfo (userInfo);

	if (!theRemoteWebUsers_.xmlRequestToGateway (
	        cgiIn,
	        out,
	        &xmlOut,
	        CorePropertySupervisorBase::allSupervisorInfo_,
	        userInfo))
		return;  //access failed

	//done checking cookieCode, sequence, userWithLock, and permissions access all in one shot!
	//**** end LOGIN GATEWAY CODE ***//

	if (!userInfo.automatedCommand_)
		__SUP_COUT__ << "requestType: " << requestType << __E__;

	if (userInfo.NonXMLRequestType_)
	{
		try
		{
			nonXmlRequest (requestType, cgiIn, *out, userInfo);
		}
		catch (const std::runtime_error& e)
		{
			__SUP_SS__ << "An error was encountered handling requestType '" << requestType << "':" << e.what () << __E__;
			__SUP_COUT_ERR__ << "\n"
			                 << ss.str ();
			__SUP_MOUT_ERR__ << "\n"
			                 << ss.str ();
		}
		catch (...)
		{
			__SUP_SS__ << "An unknown error was encountered handling requestType '" << requestType << ".' "
			           << "Please check the printouts to debug." << __E__;
			__SUP_COUT_ERR__ << "\n"
			                 << ss.str ();
			__SUP_MOUT_ERR__ << "\n"
			                 << ss.str ();
		}
		return;
	}
	//else xml request type

	try
	{
		//call derived class' request()
		request (requestType, cgiIn, xmlOut, userInfo);
	}
	catch (const std::runtime_error& e)
	{
		__SUP_SS__ << "An error was encountered handling requestType '" << requestType << "':" << e.what () << __E__;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		xmlOut.addTextElementToData ("Error", ss.str ());
	}
	catch (...)
	{
		__SUP_SS__ << "An unknown error was encountered handling requestType '" << requestType << ".' "
		           << "Please check the printouts to debug." << __E__;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		xmlOut.addTextElementToData ("Error", ss.str ());
	}

	//report any errors encountered
	{
		unsigned int occurance = 0;
		std::string  err       = xmlOut.getMatchingValue ("Error", occurance++);
		while (err != "")
		{
			__SUP_COUT_ERR__ << "'" << requestType << "' ERROR encountered: " << err << std::endl;
			__SUP_MOUT_ERR__ << "'" << requestType << "' ERROR encountered: " << err << std::endl;
			err = xmlOut.getMatchingValue ("Error", occurance++);
		}
	}

	//return xml doc holding server response
	xmlOut.outputXmlDocument ((std::ostringstream*)out, false /*print to cout*/, !userInfo.NoXmlWhiteSpace_ /*allow whitespace*/);
}

//========================================================================================================================
//request
//		Supervisors should override this function. It will be called after user access has been verified
//		according to the Supervisor Property settings. The CoreSupervisorBase class provides consistent
//		access, responses, and error handling across all inheriting supervisors that use ::request.
void CoreSupervisorBase::request (const std::string& requestType, cgicc::Cgicc& cgiIn, HttpXmlDocument& xmlOut, const WebUsers::RequestUserInfo& userInfo)
{
	__SUP_COUT__ << "This is the empty Core Supervisor request. Supervisors should override this function." << __E__;

	// KEEP:
	//	here are some possibly interesting example lines of code for overriding supervisors
	//
	//try
	//{
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
	//		__SUP_SS_THROW__;
	//	}
	//	xmlOut.addTextElementToData("Error",
	//			"request encountered an error!");
	//}
	//	catch(const std::runtime_error& e)
	//	{
	//		__SUP_SS__ << "An error was encountered handling requestType '" << requestType << "':" <<
	//				e.what() << __E__;
	//		__SUP_COUT_ERR__ << "\n" << ss.str();
	//		xmlOut.addTextElementToData("Error", ss.str());
	//	}
	//	catch(...)
	//	{
	//		__SUP_SS__ << "An unknown error was encountered handling requestType '" << requestType << ".' " <<
	//				"Please check the printouts to debug." << __E__;
	//		__SUP_COUT_ERR__ << "\n" << ss.str();
	//		xmlOut.addTextElementToData("Error", ss.str());
	//	}
	// END KEEP.
}

//========================================================================================================================
//nonXmlRequest
//		Supervisors should override this function. It will be called after user access has been verified
//		according to the Supervisor Property settings. The CoreSupervisorBase class provides consistent
//		access, responses, and error handling across all inheriting supervisors that use ::request.
void CoreSupervisorBase::nonXmlRequest (const std::string& requestType, cgicc::Cgicc& cgiIn, std::ostream& out, const WebUsers::RequestUserInfo& userInfo)
{
	__SUP_COUT__ << "This is the empty Core Supervisor non-xml request. Supervisors should override this function." << __E__;
	out << "This is the empty Core Supervisor non-xml request. Supervisors should override this function." << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::stateMachineXgiHandler (xgi::Input* in, xgi::Output* out)

{
}

//========================================================================================================================
void CoreSupervisorBase::stateMachineResultXgiHandler (xgi::Input* in, xgi::Output* out)

{
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineXoapHandler (xoap::MessageReference message)

{
	__SUP_COUT__ << "Soap Handler!" << std::endl;
	stateMachineWorkLoopManager_.removeProcessedRequests ();
	stateMachineWorkLoopManager_.processRequest (message);
	__SUP_COUT__ << "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineResultXoapHandler (xoap::MessageReference message)

{
	__SUP_COUT__ << "Soap Handler!" << std::endl;
	//stateMachineWorkLoopManager_.removeProcessedRequests();
	//stateMachineWorkLoopManager_.processRequest(message);
	__SUP_COUT__ << "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
//indirection to allow for overriding handler
xoap::MessageReference CoreSupervisorBase::workLoopStatusRequestWrapper (xoap::MessageReference message)

{
	//this should have an override for monitoring work loops being done
	return workLoopStatusRequest (message);
}  //end workLoopStatusRequest()

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::workLoopStatusRequest (xoap::MessageReference message)

{
	//this should have an override for monitoring work loops being done
	return SOAPUtilities::makeSOAPMessageReference (CoreSupervisorBase::WORK_LOOP_DONE);
}  //end workLoopStatusRequest()

//========================================================================================================================
bool CoreSupervisorBase::stateMachineThread (toolbox::task::WorkLoop* workLoop)
{
	stateMachineSemaphore_.take ();
	__SUP_COUT__ << "Re-sending message..." << SOAPUtilities::translate (stateMachineWorkLoopManager_.getMessage (workLoop)).getCommand () << std::endl;
	std::string reply = send (this->getApplicationDescriptor (), stateMachineWorkLoopManager_.getMessage (workLoop));
	stateMachineWorkLoopManager_.report (workLoop, reply, 100, true);
	__SUP_COUT__ << "Done with message" << std::endl;
	stateMachineSemaphore_.give ();
	return false;  //execute once and automatically remove the workloop so in WorkLoopManager the try workLoop->remove(job_) could be commented out
	               //return true;//go on and then you must do the workLoop->remove(job_) in WorkLoopManager
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineStateRequest (xoap::MessageReference message)

{
	__SUP_COUT__ << "theStateMachine_.getCurrentStateName() = " << theStateMachine_.getCurrentStateName () << std::endl;
	return SOAPUtilities::makeSOAPMessageReference (theStateMachine_.getCurrentStateName ());
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineErrorMessageRequest (xoap::MessageReference message)

{
	__SUP_COUT__ << "theStateMachine_.getErrorMessage() = " << theStateMachine_.getErrorMessage () << std::endl;

	SOAPParameters retParameters;
	retParameters.addParameter ("ErrorMessage", theStateMachine_.getErrorMessage ());
	return SOAPUtilities::makeSOAPMessageReference ("stateMachineErrorMessageRequestReply", retParameters);
}

//========================================================================================================================
void CoreSupervisorBase::stateInitial (toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateInitial" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateHalted (toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateHalted" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateRunning (toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateRunning" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateConfigured (toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::stateConfigured" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::statePaused (toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "CoreSupervisorBase::statePaused" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::inError (toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName () << std::endl;
	//rcmsStateNotifier_.stateChanged("Error", "");
}

//========================================================================================================================
void CoreSupervisorBase::enteringError (toolbox::Event::Reference e)

{
	//	__SUP_COUT__<< "Fsm current state: " << theStateMachine_.getCurrentStateName()
	//			<< "\n\nError Message: " <<
	//			theStateMachine_.getErrorMessage() << std::endl;
	toolbox::fsm::FailedEvent& failedEvent = dynamic_cast<toolbox::fsm::FailedEvent&> (*e);
	std::ostringstream         error;
	error << "Failure performing transition from "
	      << failedEvent.getFromState ()
	      << " to "
	      << failedEvent.getToState ()
	      << " exception: " << failedEvent.getException ().what ();
	__SUP_COUT_ERR__ << error.str () << std::endl;
	//diagService_->reportError(errstr.str(),DIAGERROR);
}

//========================================================================================================================
void CoreSupervisorBase::preStateMachineExecutionLoop (void)
{
	RunControlStateMachine::clearIterationWork ();
	RunControlStateMachine::clearSubIterationWork ();

	stateMachinesIterationWorkCount_ = 0;

	if (RunControlStateMachine::getIterationIndex () == 0 &&
	    RunControlStateMachine::getSubIterationIndex () == 0)
	{
		//reset vector for iterations done on first iteration

		subIterationWorkStateMachineIndex_ = -1;  //clear sub iteration work index

		stateMachinesIterationDone_.resize (theStateMachineImplementation_.size ());
		for (unsigned int i = 0; i < stateMachinesIterationDone_.size (); ++i)
			stateMachinesIterationDone_[i] = false;
	}
	else
		__SUP_COUT__ << "Iteration " << RunControlStateMachine::getIterationIndex () << "." << RunControlStateMachine::getSubIterationIndex () << "(" << subIterationWorkStateMachineIndex_ << ")" << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::preStateMachineExecution (unsigned int i)
{
	if (i >= theStateMachineImplementation_.size ())
	{
		__SUP_SS__ << "State Machine " << i << " not found!" << __E__;
		__SUP_SS_THROW__;
	}

	theStateMachineImplementation_[i]->VStateMachine::setIterationIndex (
	    RunControlStateMachine::getIterationIndex ());
	theStateMachineImplementation_[i]->VStateMachine::setSubIterationIndex (
	    RunControlStateMachine::getSubIterationIndex ());

	theStateMachineImplementation_[i]->VStateMachine::clearIterationWork ();
	theStateMachineImplementation_[i]->VStateMachine::clearSubIterationWork ();

	__SUP_COUT__ << "theStateMachineImplementation Iteration " << theStateMachineImplementation_[i]->VStateMachine::getIterationIndex () << "." << theStateMachineImplementation_[i]->VStateMachine::getSubIterationIndex () << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::postStateMachineExecution (unsigned int i)
{
	if (i >= theStateMachineImplementation_.size ())
	{
		__SUP_SS__ << "State Machine " << i << " not found!" << __E__;
		__SUP_SS_THROW__;
	}

	//sub-iteration has priority
	if (theStateMachineImplementation_[i]->VStateMachine::getSubIterationWork ())
	{
		subIterationWorkStateMachineIndex_ = i;
		RunControlStateMachine::indicateSubIterationWork ();

		__SUP_COUT__ << "State machine " << i << " is flagged for another sub-iteration..." << __E__;
	}
	else
	{
		stateMachinesIterationDone_[i] =
		    !theStateMachineImplementation_[i]->VStateMachine::getIterationWork ();

		if (!stateMachinesIterationDone_[i])
		{
			__SUP_COUT__ << "State machine " << i << " is flagged for another iteration..." << __E__;
			RunControlStateMachine::indicateIterationWork ();  //mark not done at CoreSupervisorBase level
			++stateMachinesIterationWorkCount_;                //increment still working count
		}
	}
}

//========================================================================================================================
void CoreSupervisorBase::postStateMachineExecutionLoop (void)
{
	if (RunControlStateMachine::subIterationWorkFlag_)
		__SUP_COUT__ << "State machine implementation " << subIterationWorkStateMachineIndex_ << " is flagged for another sub-iteration..." << __E__;
	else if (RunControlStateMachine::iterationWorkFlag_)
		__SUP_COUT__ << stateMachinesIterationWorkCount_ << " state machine implementation(s) flagged for another iteration..." << __E__;
	else
		__SUP_COUT__ << "Done configuration all state machine implementations..." << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::transitionConfiguring (toolbox::Event::Reference e)
{
	__SUP_COUT__ << "transitionConfiguring" << std::endl;

	//activate the configuration tree (the first iteration)
	if (RunControlStateMachine::getIterationIndex () == 0 &&
	    RunControlStateMachine::getSubIterationIndex () == 0)
	{
		std::pair<std::string /*group name*/, ConfigurationGroupKey> theGroup (
		    SOAPUtilities::translate (theStateMachine_.getCurrentMessage ()).getParameters ().getValue ("ConfigurationGroupName"),
		    ConfigurationGroupKey (SOAPUtilities::translate (theStateMachine_.getCurrentMessage ()).getParameters ().getValue ("ConfigurationGroupKey")));

		__SUP_COUT__ << "Configuration group name: " << theGroup.first << " key: " << theGroup.second << std::endl;

		theConfigurationManager_->loadConfigurationGroup (
		    theGroup.first,
		    theGroup.second,
		    true);
	}

	//Now that the configuration manager has all the necessary configurations,
	//	create all objects that depend on the configuration (the first iteration)

	try
	{
		__SUP_COUT__ << "Configuring all state machine implementations..." << __E__;
		preStateMachineExecutionLoop ();
		for (unsigned int i = 0; i < theStateMachineImplementation_.size (); ++i)
		{
			//if one state machine is doing a sub-iteration, then target that one
			if (subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			    i != subIterationWorkStateMachineIndex_) continue;  //skip those not in the sub-iteration

			if (stateMachinesIterationDone_[i]) continue;  //skip state machines already done

			preStateMachineExecution (i);
			theStateMachineImplementation_[i]->parentSupervisor_ = this;  //for backwards compatibility, kept out of configure parameters
			theStateMachineImplementation_[i]->configure ();              //e.g. for FESupervisor, this is configure of FEVInterfacesManager
			postStateMachineExecution (i);
		}
		postStateMachineExecutionLoop ();
	}
	catch (const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while configuring: " << e.what () << std::endl;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		theStateMachine_.setErrorMessage (ss.str ());
		throw toolbox::fsm::exception::Exception (
		    "Transition Error" /*name*/,
		    ss.str () /* message*/,
		    "CoreSupervisorBase::transitionConfiguring" /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
	catch (...)
	{
		__SUP_SS__ << "Unknown error was caught while configuring. Please checked the logs." << __E__;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		theStateMachine_.setErrorMessage (ss.str ());
		throw toolbox::fsm::exception::Exception (
		    "Transition Error" /*name*/,
		    ss.str () /* message*/,
		    "CoreSupervisorBase::transitionConfiguring" /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
}  //end transitionConfiguring()

//========================================================================================================================
//transitionHalting
//	Ignore errors if coming from Failed state
void CoreSupervisorBase::transitionHalting (toolbox::Event::Reference e)
{
	const std::string transitionName = "Halting";
	try
	{
		__SUP_COUT__ << transitionName << " all state machine implementations..." << __E__;
		preStateMachineExecutionLoop ();
		for (unsigned int i = 0; i < theStateMachineImplementation_.size (); ++i)
		{
			//if one state machine is doing a sub-iteration, then target that one
			if (subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			    i != subIterationWorkStateMachineIndex_) continue;  //skip those not in the sub-iteration

			if (stateMachinesIterationDone_[i]) continue;  //skip state machines already done

			preStateMachineExecution (i);
			theStateMachineImplementation_[i]->halt ();  //e.g. for FESupervisor, this is transition of FEVInterfacesManager
			postStateMachineExecution (i);
		}
		postStateMachineExecutionLoop ();
	}
	catch (const std::runtime_error& e)
	{
		//if halting from Failed state, then ignore errors
		if (theStateMachine_.getProvenanceStateName () ==
		    RunControlStateMachine::FAILED_STATE_NAME)
		{
			__SUP_COUT_INFO__ << "Error was caught while halting (but ignoring because previous state was '" << RunControlStateMachine::FAILED_STATE_NAME << "'): " << e.what () << std::endl;
		}
		else  //if not previously in Failed state, then fail
		{
			__SUP_SS__ << "Error was caught while " << transitionName << ": " << e.what () << std::endl;
			__SUP_COUT_ERR__ << "\n"
			                 << ss.str ();
			theStateMachine_.setErrorMessage (ss.str ());
			throw toolbox::fsm::exception::Exception (
			    "Transition Error" /*name*/,
			    ss.str () /* message*/,
			    "CoreSupervisorBase::transition" + transitionName /*module*/,
			    __LINE__ /*line*/,
			    __FUNCTION__ /*function*/
			);
		}
	}
	catch (...)
	{
		//if halting from Failed state, then ignore errors
		if (theStateMachine_.getProvenanceStateName () ==
		    RunControlStateMachine::FAILED_STATE_NAME)
		{
			__SUP_COUT_INFO__ << "Unknown error was caught while halting (but ignoring because previous state was '" << RunControlStateMachine::FAILED_STATE_NAME << "')." << std::endl;
		}
		else  //if not previously in Failed state, then fail
		{
			__SUP_SS__ << "Unknown error was caught while " << transitionName << ". Please checked the logs." << __E__;
			__SUP_COUT_ERR__ << "\n"
			                 << ss.str ();
			theStateMachine_.setErrorMessage (ss.str ());
			throw toolbox::fsm::exception::Exception (
			    "Transition Error" /*name*/,
			    ss.str () /* message*/,
			    "CoreSupervisorBase::transition" + transitionName /*module*/,
			    __LINE__ /*line*/,
			    __FUNCTION__ /*function*/
			);
		}
	}
}  //end transitionHalting()

//========================================================================================================================
//Inheriting supervisor classes should not override this function, or should at least also call it in the override
//	to maintain property functionality.
void CoreSupervisorBase::transitionInitializing (toolbox::Event::Reference e)
{
	__SUP_COUT__ << "transitionInitializing" << std::endl;

	CorePropertySupervisorBase::resetPropertiesAreSetup ();  //indicate need to re-load user properties

	//Note: Do not initialize the state machine implementations...
	//	do any initializing in configure
	//	This allows re-instantiation at each configure time.
	//for(auto& it: theStateMachineImplementation_)
	//it->initialize();
}  //end transitionInitializing()

//========================================================================================================================
void CoreSupervisorBase::transitionPausing (toolbox::Event::Reference e)
{
	const std::string transitionName = "Pausing";
	try
	{
		__SUP_COUT__ << "Configuring all state machine implementations..." << __E__;
		preStateMachineExecutionLoop ();
		for (unsigned int i = 0; i < theStateMachineImplementation_.size (); ++i)
		{
			//if one state machine is doing a sub-iteration, then target that one
			if (subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			    i != subIterationWorkStateMachineIndex_) continue;  //skip those not in the sub-iteration

			if (stateMachinesIterationDone_[i]) continue;  //skip state machines already done

			preStateMachineExecution (i);
			theStateMachineImplementation_[i]->pause ();  //e.g. for FESupervisor, this is transition of FEVInterfacesManager
			postStateMachineExecution (i);
		}
		postStateMachineExecutionLoop ();
	}
	catch (const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while " << transitionName << ": " << e.what () << std::endl;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		theStateMachine_.setErrorMessage (ss.str ());
		throw toolbox::fsm::exception::Exception (
		    "Transition Error" /*name*/,
		    ss.str () /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
	catch (...)
	{
		__SUP_SS__ << "Unknown error was caught while " << transitionName << ". Please checked the logs." << __E__;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		theStateMachine_.setErrorMessage (ss.str ());
		throw toolbox::fsm::exception::Exception (
		    "Transition Error" /*name*/,
		    ss.str () /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
}  //end transitionPausing()

//========================================================================================================================
void CoreSupervisorBase::transitionResuming (toolbox::Event::Reference e)
{
	const std::string transitionName = "Resuming";
	try
	{
		__SUP_COUT__ << "Configuring all state machine implementations..." << __E__;
		preStateMachineExecutionLoop ();
		for (unsigned int i = 0; i < theStateMachineImplementation_.size (); ++i)
		{
			//if one state machine is doing a sub-iteration, then target that one
			if (subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			    i != subIterationWorkStateMachineIndex_) continue;  //skip those not in the sub-iteration

			if (stateMachinesIterationDone_[i]) continue;  //skip state machines already done

			preStateMachineExecution (i);
			theStateMachineImplementation_[i]->resume ();  //e.g. for FESupervisor, this is transition of FEVInterfacesManager
			postStateMachineExecution (i);
		}
		postStateMachineExecutionLoop ();
	}
	catch (const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while " << transitionName << ": " << e.what () << std::endl;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		theStateMachine_.setErrorMessage (ss.str ());
		throw toolbox::fsm::exception::Exception (
		    "Transition Error" /*name*/,
		    ss.str () /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
	catch (...)
	{
		__SUP_SS__ << "Unknown error was caught while " << transitionName << ". Please checked the logs." << __E__;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		theStateMachine_.setErrorMessage (ss.str ());
		throw toolbox::fsm::exception::Exception (
		    "Transition Error" /*name*/,
		    ss.str () /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
}  // end transitionResuming()

//========================================================================================================================
void CoreSupervisorBase::transitionStarting (toolbox::Event::Reference e)
{
	const std::string transitionName = "Starting";
	const std::string runNumber      = SOAPUtilities::translate (
                                      theStateMachine_.getCurrentMessage ())
	                                  .getParameters ()
	                                  .getValue ("RunNumber");
	try
	{
		__SUP_COUT__ << "Configuring all state machine implementations..." << __E__;
		preStateMachineExecutionLoop ();
		for (unsigned int i = 0; i < theStateMachineImplementation_.size (); ++i)
		{
			//if one state machine is doing a sub-iteration, then target that one
			if (subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			    i != subIterationWorkStateMachineIndex_) continue;  //skip those not in the sub-iteration

			if (stateMachinesIterationDone_[i]) continue;  //skip state machines already done

			preStateMachineExecution (i);
			theStateMachineImplementation_[i]->start (runNumber);  //e.g. for FESupervisor, this is transition of FEVInterfacesManager
			postStateMachineExecution (i);
		}
		postStateMachineExecutionLoop ();
	}
	catch (const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while " << transitionName << ": " << e.what () << std::endl;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		theStateMachine_.setErrorMessage (ss.str ());
		throw toolbox::fsm::exception::Exception (
		    "Transition Error" /*name*/,
		    ss.str () /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
	catch (...)
	{
		__SUP_SS__ << "Unknown error was caught while " << transitionName << ". Please checked the logs." << __E__;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		theStateMachine_.setErrorMessage (ss.str ());
		throw toolbox::fsm::exception::Exception (
		    "Transition Error" /*name*/,
		    ss.str () /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
}  //end transitionStarting()

//========================================================================================================================
void CoreSupervisorBase::transitionStopping (toolbox::Event::Reference e)
{
	const std::string transitionName = "Stopping";
	try
	{
		__SUP_COUT__ << "Configuring all state machine implementations..." << __E__;
		preStateMachineExecutionLoop ();
		for (unsigned int i = 0; i < theStateMachineImplementation_.size (); ++i)
		{
			//if one state machine is doing a sub-iteration, then target that one
			if (subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
			    i != subIterationWorkStateMachineIndex_) continue;  //skip those not in the sub-iteration

			if (stateMachinesIterationDone_[i]) continue;  //skip state machines already done

			preStateMachineExecution (i);
			theStateMachineImplementation_[i]->stop ();  //e.g. for FESupervisor, this is transition of FEVInterfacesManager
			postStateMachineExecution (i);
		}
		postStateMachineExecutionLoop ();
	}
	catch (const std::runtime_error& e)
	{
		__SUP_SS__ << "Error was caught while " << transitionName << ": " << e.what () << std::endl;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		theStateMachine_.setErrorMessage (ss.str ());
		throw toolbox::fsm::exception::Exception (
		    "Transition Error" /*name*/,
		    ss.str () /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
	catch (...)
	{
		__SUP_SS__ << "Unknown error was caught while " << transitionName << ". Please checked the logs." << __E__;
		__SUP_COUT_ERR__ << "\n"
		                 << ss.str ();
		theStateMachine_.setErrorMessage (ss.str ());
		throw toolbox::fsm::exception::Exception (
		    "Transition Error" /*name*/,
		    ss.str () /* message*/,
		    "CoreSupervisorBase::transition" + transitionName /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
}  //end transitionStopping()
