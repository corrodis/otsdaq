#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"
#include "otsdaq-core/CgiDataUtilities/CgiDataUtilities.h"
//#include "otsdaq-core/Singleton/Singleton.h"
//#include "otsdaq-core/WorkLoopManager/WorkLoopManager.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationTree.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/FiniteStateMachine/VStateMachine.h"
#include "otsdaq-core/FECore/FEVInterfacesManager.h"


#include <toolbox/fsm/FailedEvent.h>

#include <xdaq/NamespaceURI.h>
#include <xoap/Method.h>

#include <iostream>

using namespace ots;

//XDAQ_INSTANTIATOR_IMPL(CoreSupervisorBase)

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
{
	__MOUT__ << "Begin!" << std::endl;
	__MOUT__ << "Begin!" << std::endl;
	__MOUT__ << "Begin!" << std::endl;
	__MOUT__ << "Begin!" << std::endl;
	__MOUT__ << "Begin!" << std::endl;
	xgi::bind (this, &CoreSupervisorBase::Default,                "Default" );
	xgi::bind (this, &CoreSupervisorBase::stateMachineXgiHandler, "StateMachineXgiHandler");
	xgi::bind (this, &CoreSupervisorBase::request, 				  "Request");

	xoap::bind(this, &CoreSupervisorBase::stateMachineStateRequest,    "StateMachineStateRequest",    XDAQ_NS_URI );
	xoap::bind(this, &CoreSupervisorBase::macroMakerSupervisorRequest, "MacroMakerSupervisorRequest", XDAQ_NS_URI );

	try
	{
		supervisorContextUID_ = theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getContextUID(getApplicationContext()->getContextDescriptor()->getURL());
	}
	catch(...)
	{
		__MOUT_ERR__ << "XDAQ Supervisor could not access it's configuration through the Configuration Context Group." <<
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
		__MOUT_ERR__ << "XDAQ Supervisor could not access it's configuration through the Configuration Application Group."
				<< " The supervisorApplicationUID = " << supervisorApplicationUID_ << std::endl;
		throw;
	}
	supervisorConfigurationPath_  = "/" + supervisorContextUID_ + "/LinkToApplicationConfiguration/" + supervisorApplicationUID_ + "/LinkToSupervisorConfiguration";

	setStateMachineName(supervisorApplicationUID_);
	__MOUT__ << "Done!" << std::endl;
	__MOUT__ << "Done!" << std::endl;
	__MOUT__ << "Done!" << std::endl;
	__MOUT__ << "Done!" << std::endl;

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
	__MOUT__ << "CONTEXT!" << std::endl;
	__MOUT__ << "CONTEXT!" << std::endl;
	__MOUT__ << "CONTEXT!" << std::endl;
	supervisorDescriptorInfo_.init(getApplicationContext());
	__MOUT__ << "Done!" << std::endl;
	__MOUT__ << "Done!" << std::endl;
	__MOUT__ << "Done!" << std::endl;
	//RunControlStateMachine::reset();
}

//========================================================================================================================
void CoreSupervisorBase::destroy(void)
{
	for(auto& it: theStateMachineImplementation_)
		delete it;
	theStateMachineImplementation_.clear();
}

//========================================================================================================================
void CoreSupervisorBase::Default(xgi::Input * in, xgi::Output * out )
throw (xgi::exception::Exception)
{


	__MOUT__<< "Supervisor class " << supervisorClass_ << std::endl;

	*out << "<!DOCTYPE HTML><html lang='en'><frameset col='100%' row='100%'><frame src='/WebPath/html/" << supervisorClassNoNamespace_ << "Supervisor.html?urn=" <<
			this->getApplicationDescriptor()->getLocalId() << //getenv((theSupervisorClassNoNamespace_ + "_SUPERVISOR_ID").c_str()) <<
			"'></frameset></html>";
}

//========================================================================================================================
void CoreSupervisorBase::request(xgi::Input * in, xgi::Output * out )
throw (xgi::exception::Exception)
{


	cgicc::Cgicc cgi(in);
	std::string write = CgiDataUtilities::getOrPostData(cgi,"write");
	std::string addr = CgiDataUtilities::getOrPostData(cgi,"addr");
	std::string data = CgiDataUtilities::getOrPostData(cgi,"data");

	__MOUT__<< "write " << write << " addr: " << addr << " data: " << data << std::endl;

	unsigned long long int addr64,data64;
	sscanf(addr.c_str(),"%llu",&addr64);
	sscanf(data.c_str(),"%llu",&data64);
	__MOUT__<< "write " << write << " addr: " << addr64 << " data: " << data64 << std::endl;

	*out << "done";
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
	__MOUT__<< "Soap Handler!" << std::endl;
	stateMachineWorkLoopManager_.removeProcessedRequests();
	stateMachineWorkLoopManager_.processRequest(message);
	__MOUT__<< "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineResultXoapHandler(xoap::MessageReference message )
throw (xoap::exception::Exception)
{
	__MOUT__<< "Soap Handler!" << std::endl;
	//stateMachineWorkLoopManager_.removeProcessedRequests();
	//stateMachineWorkLoopManager_.processRequest(message);
	__MOUT__<< "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
bool CoreSupervisorBase::stateMachineThread(toolbox::task::WorkLoop* workLoop)
{
	stateMachineSemaphore_.take();
	__MOUT__<< "Re-sending message..." << SOAPUtilities::translate(stateMachineWorkLoopManager_.getMessage(workLoop)).getCommand() << std::endl;
	std::string reply = send(this->getApplicationDescriptor(),stateMachineWorkLoopManager_.getMessage(workLoop));
	stateMachineWorkLoopManager_.report(workLoop, reply, 100, true);
	__MOUT__<< "Done with message" << std::endl;
	stateMachineSemaphore_.give();
	return false;//execute once and automatically remove the workloop so in WorkLoopManager the try workLoop->remove(job_) could be commented out
	//return true;//go on and then you must do the workLoop->remove(job_) in WorkLoopManager
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineStateRequest(xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	__MOUT__<< theStateMachine_.getCurrentStateName() << std::endl;
	return SOAPUtilities::makeSOAPMessageReference(theStateMachine_.getCurrentStateName());
}

//========================================================================================================================
// macroMakerSupervisorRequest
//	 Handles all MacroMaker Requests:
//		- GetInterfaces (returns interface type and id)
//
//	Note: this code assumes a CoreSupervisorBase has only one
//		FEVInterfacesManager in its vector of state machines
xoap::MessageReference CoreSupervisorBase::macroMakerSupervisorRequest(
		xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	__MOUT__<< "$$$$$$$$$$$$$$$$$" << std::endl;

	//locate theFEInterfacesManager in state machines vector
	FEVInterfacesManager*          theFEInterfacesManager = 0;
	for(unsigned int i = 0; i<theStateMachineImplementation_.size();++i)
	{
		try
		{
			theFEInterfacesManager =
					dynamic_cast<FEVInterfacesManager*>(theStateMachineImplementation_[i]);
			if(!theFEInterfacesManager) throw std::runtime_error(""); //dynamic_cast returns null pointer on failure
			__MOUT__ << "State Machine " << i << " WAS of type FEVInterfacesManager" << std::endl;

			break;
		}
		catch(...)
		{
			__MOUT__ << "State Machine " << i << " was NOT of type FEVInterfacesManager" << std::endl;
		}
	}


	__MOUT__ << "theFEInterfacesManager pointer = " << theFEInterfacesManager << std::endl;

	//receive request parameters
	SOAPParameters parameters;
	parameters.addParameter("Request");
	parameters.addParameter("InterfaceID");

	//params for universal commands
	parameters.addParameter("Address");
	parameters.addParameter("Data");

	//params for running macros
	parameters.addParameter("feMacroName");
	parameters.addParameter("inputArgs");
	parameters.addParameter("outputArgs");

	SOAPMessenger::receive(message, parameters);
	std::string request = parameters.getValue("Request");
	std::string addressStr = parameters.getValue("Address");
	std::string dataStr = parameters.getValue("Data");
	std::string InterfaceID = parameters.getValue("InterfaceID");

	__MOUT__<< "request: " << request << std::endl;

	__MOUT__<<  "Address: " << addressStr << " Data: "
			<< dataStr << " InterfaceID: " << InterfaceID << std::endl;

	SOAPParameters retParameters;

	if(request == "GetInterfaces")
	{
		if(theFEInterfacesManager)
			retParameters.addParameter("FEList",theFEInterfacesManager->getFEListString(
								std::to_string(getApplicationDescriptor()->getLocalId())));
		else
			retParameters.addParameter("FEList","");

		return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "Response",
				retParameters);
	}
	else if(request == "UniversalWrite")
	{
		if(!theFEInterfacesManager)
		{
			__MOUT_INFO__ << "No FE Interface Manager! (So no write occurred)" << std::endl;
			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "DataWritten",retParameters);
		}

		// parameters interface index!
		//unsigned int index = stoi(indexStr);	// As long as the supervisor has only one interface, this index will remain 0?



		__MOUT__<< "theFEInterfacesManager->getInterfaceUniversalAddressSize(index) " <<
				theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID) << std::endl;
		__MOUT__<< "theFEInterfacesManager->getInterfaceUniversalDataSize(index) " <<
				theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID) << std::endl;

		//Converting std::string to char*
		//char address

		char tmpHex[3]; //for use converting hex to binary
		tmpHex[2] = '\0';


		__MOUT__<< "Translating address: ";

		char address[theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID)];

		if(addressStr.size()%2) //if odd, make even
			addressStr = "0" + addressStr;
		unsigned int i=0;
		for(;i<addressStr.size() &&
		i/2 < theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID) ; i+=2)
		{
			tmpHex[0] = addressStr[addressStr.size()-1-i-1];
			tmpHex[1] = addressStr[addressStr.size()-1-i];
			sscanf(tmpHex,"%hhX",(unsigned char*)&address[i/2]);
			printf("%2.2X",(unsigned char)address[i/2]);
		}
		//finish and fill with 0s
		for(;i/2 < theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID) ; i+=2)
		{
			address[i/2] = 0;
			printf("%2.2X",(unsigned char)address[i/2]);
		}

		std::cout << std::endl;

		__MOUT__<< "Translating data: ";

		char data[theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID)];

		if(dataStr.size()%2) //if odd, make even
			dataStr = "0" + dataStr;

		i=0;
		for(;i<dataStr.size() &&
		i/2 < theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID) ; i+=2)
		{
			tmpHex[0] = dataStr[dataStr.size()-1-i-1];
			tmpHex[1] = dataStr[dataStr.size()-1-i];
			sscanf(tmpHex,"%hhX",(unsigned char*)&data[i/2]);
			printf("%2.2X",(unsigned char)data[i/2]);
		}
		//finish and fill with 0s
		for(;i/2 < theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID) ; i+=2)
		{
			data[i/2] = 0;
			printf("%2.2X",(unsigned char)data[i/2]);
		}

		std::cout << std::endl;

		//char* address = new char[addressStr.size() + 1];
		//std::copy(addressStr.begin(), addressStr.end(), address);
		//    	address[addressStr.size()] = '\0';
		//    	char* data = new char[dataStr.size() + 1];
		//		std::copy(dataStr.begin(), dataStr.end(), data);
		//		data[dataStr.size()] = '\0';

		theFEInterfacesManager->universalWrite(InterfaceID,address,data);

		//delete[] address;
		//delete[] data;

		return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "DataWritten",retParameters);
	}
	else if(request == "UniversalRead")
	{
		if(!theFEInterfacesManager)
		{
			__MOUT_INFO__ << "No FE Interface Manager! (So no read occurred)" << std::endl;
			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa",retParameters);
		}

		// parameters interface index!
		// parameter address and data
		//unsigned int index = stoi(indexStr);	// As long as the supervisor has only one interface, this index will remain 0?

		__MOUT__<< "theFEInterfacesManager->getInterfaceUniversalAddressSize(index) "
				<< theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID) << std::endl;
		__MOUT__<< "theFEInterfacesManager->getInterfaceUniversalDataSize(index) "
				<<theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID) << std::endl;

		char tmpHex[3]; //for use converting hex to binary
		tmpHex[2] = '\0';


		__MOUT__<< "Translating address: ";

		char address[theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID)];

		if(addressStr.size()%2) //if odd, make even
			addressStr = "0" + addressStr;

		unsigned int i=0;
		for(;i<addressStr.size() &&
		i/2 < theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID) ; i+=2)
		{
			tmpHex[0] = addressStr[addressStr.size()-1-i-1];
			tmpHex[1] = addressStr[addressStr.size()-1-i];
			sscanf(tmpHex,"%hhX",(unsigned char*)&address[i/2]);
			printf("%2.2X",(unsigned char)address[i/2]);
		}
		//finish and fill with 0s
		for(;i/2 < theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID) ; i+=2)
		{
			address[i/2] = 0;
			printf("%2.2X",(unsigned char)address[i/2]);
		}

		std::cout << std::endl;

		unsigned int dataSz = theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID);
		char data[dataSz];


		//    	std::string result = theFEInterfacesManager->universalRead(index,address,data);
		//    	__MOUT__<< result << std::endl << std::endl;

		if(theFEInterfacesManager->universalRead(InterfaceID, address, data) < 0)
		{
			retParameters.addParameter("dataResult","Time Out Error");
			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa",retParameters);
		}

		//if dataSz is less than 8 show what the unsigned number would be
		if(dataSz <= 8)
		{
			std::string str8(data);
			str8.resize(8);
			__MOUT__<< "decResult[" << dataSz << " bytes]: " <<
					*((unsigned long long *)(&str8[0])) << std::endl;

		}

		char hexResult[dataSz*2+1];
		//go through each byte and convert it to hex value (i.e. 2 0-F chars)
		//go backwards through source data since should be provided in host order
		//	(i.e. a cast to unsigned long long should acquire expected value)
		for(unsigned int i=0;i<dataSz;++i)
		{
			sprintf(&hexResult[i*2],"%2.2X", (unsigned char)data[dataSz-1-i]);
		}

		__MOUT__<< "hexResult[" << strlen(hexResult) << " nibbles]: " << std::string(hexResult) << std::endl;



		retParameters.addParameter("dataResult",hexResult);
		return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa",retParameters);

	}
	else if(request == "GetInterfaceMacros")
	{
		if(theFEInterfacesManager)
			retParameters.addParameter("FEMacros",theFEInterfacesManager->getFEMacrosString(
					std::to_string(getApplicationDescriptor()->getLocalId())));
		else
			retParameters.addParameter("FEMacros","");

		return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "Response",
				retParameters);
	}
	else if(request == "RunInterfaceMacro")
	{
		if(!theFEInterfacesManager)
		{
			retParameters.addParameter("success","0");
			retParameters.addParameter("outputArgs","");
			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "Response",
							retParameters);
		}

		std::string feMacroName = parameters.getValue("feMacroName");
		std::string inputArgs = parameters.getValue("inputArgs");
		std::string outputArgs = parameters.getValue("outputArgs");

		//outputArgs must be filled with the proper argument names
		//	and then the response output values will be returned in the string.
		bool success = true;
		try
		{
			theFEInterfacesManager->runFEMacro(InterfaceID,feMacroName,inputArgs,outputArgs);
		}
		catch(std::runtime_error &e)
		{
			__SS__ << "In Supervisor with LID=" << getApplicationDescriptor()->getLocalId()
					<< " the FE Macro named '" << feMacroName << "' with tartget FE '"
					<< InterfaceID << "' failed. Here is the error:\n\n" << e.what() << std::endl;
			__MOUT_ERR__ << "\n" << ss.str();
			success = false;
			outputArgs = ss.str();
		}


		retParameters.addParameter("success",success?"1":"0");
		retParameters.addParameter("outputArgs",outputArgs);

		return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "Response",
						retParameters);
	}
	else
	{
		__MOUT_WARN__ << "Unrecognized request received! '" << request << "'" << std::endl;
	}


	return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "FailRequest",retParameters);
}

//========================================================================================================================
void CoreSupervisorBase::stateInitial(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{
	__MOUT__ << "CoreSupervisorBase::stateInitial" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateHalted(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{
	__MOUT__ << "CoreSupervisorBase::stateHalted" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateRunning(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{
	__MOUT__ << "CoreSupervisorBase::stateRunning" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateConfigured(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{
	__MOUT__ << "CoreSupervisorBase::stateConfigured" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::statePaused(toolbox::fsm::FiniteStateMachine& fsm)
throw (toolbox::fsm::exception::Exception)
{
	__MOUT__ << "CoreSupervisorBase::statePaused" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::inError (toolbox::fsm::FiniteStateMachine & fsm)
throw (toolbox::fsm::exception::Exception)
{
	__MOUT__<< "Fsm current state: " << theStateMachine_.getCurrentStateName()<< std::endl;
	//rcmsStateNotifier_.stateChanged("Error", "");
}

//========================================================================================================================
void CoreSupervisorBase::enteringError (toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__MOUT__<< "Fsm current state: " << theStateMachine_.getCurrentStateName()<< std::endl;
	toolbox::fsm::FailedEvent& failedEvent = dynamic_cast<toolbox::fsm::FailedEvent&>(*e);
	std::ostringstream error;
	error << "Failure performing transition from "
			<< failedEvent.getFromState()
			<<  " to "
			<< failedEvent.getToState()
			<< " exception: " << failedEvent.getException().what();
	__MOUT__<< error.str() << std::endl;
	//diagService_->reportError(errstr.str(),DIAGERROR);

}

//========================================================================================================================
void CoreSupervisorBase::transitionConfiguring(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__MOUT__ << "transitionConfiguring" << std::endl;

	std::pair<std::string /*group name*/, ConfigurationGroupKey> theGroup(
			SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).
			getParameters().getValue("ConfigurationGroupName"),
			ConfigurationGroupKey(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).
					getParameters().getValue("ConfigurationGroupKey")));

	__MOUT__ << "Configuration group name: " << theGroup.first << " key: " <<
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
		__MOUT_ERR__ << "\n" << ss.str();
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
	__MOUT__ << "transitionHalting" << std::endl;

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
				__MOUT_INFO__ << "Error was caught while halting (but ignoring because previous state was '" <<
						RunControlStateMachine::FAILED_STATE_NAME << "'): " << e.what() << std::endl;
			}
			else //if not previously in Failed state, then fail
			{
				__SS__ << "Error was caught while halting: " << e.what() << std::endl;
				__MOUT_ERR__ << "\n" << ss.str();
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
	__MOUT__ << "transitionInitializing" << std::endl;

	//    for(auto& it: theStateMachineImplementation_)
	//it->initialize();
}

//========================================================================================================================
void CoreSupervisorBase::transitionPausing(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__MOUT__ << "transitionPausing" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->pause();
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while pausing: " << e.what() << std::endl;
		__MOUT_ERR__ << "\n" << ss.str();
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


	__MOUT__ << "transitionResuming" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->resume();
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while resuming: " << e.what() << std::endl;
		__MOUT_ERR__ << "\n" << ss.str();
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

	__MOUT__ << "transitionStarting" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->start(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getParameters().getValue("RunNumber"));
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while starting: " << e.what() << std::endl;
		__MOUT_ERR__ << "\n" << ss.str();
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
	__MOUT__ << "transitionStopping" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->stop();
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while pausing: " << e.what() << std::endl;
		__MOUT_ERR__ << "\n" << ss.str();
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
