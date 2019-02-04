#include "otsdaq-core/CoreSupervisors/FESupervisor.h"
#include "otsdaq-core/FECore/FEVInterfacesManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/FECore/FEVInterfacesManager.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(FESupervisor)



//========================================================================================================================
FESupervisor::FESupervisor(xdaq::ApplicationStub * s) 
: CoreSupervisorBase(s)
{
	xoap::bind(this, &FESupervisor::macroMakerSupervisorRequest,
			"MacroMakerSupervisorRequest", 	XDAQ_NS_URI);
	xoap::bind(this, &FESupervisor::workLoopStatusRequest,
			"WorkLoopStatusRequest", 		XDAQ_NS_URI);
	xoap::bind(this, &FESupervisor::frontEndCommunicationRequest,
			"FECommunication",    			XDAQ_NS_URI );

	CoreSupervisorBase::theStateMachineImplementation_.push_back(
		new FEVInterfacesManager(
				CorePropertySupervisorBase::getContextTreeNode(),
				CorePropertySupervisorBase::supervisorConfigurationPath_
		)
	);

	extractFEInterfacesManager();

} //end constructor

//========================================================================================================================
FESupervisor::~FESupervisor(void)
{
	__SUP_COUT__ << "Destroying..." << std::endl;
	//theStateMachineImplementation_ is reset and the object it points to deleted in ~CoreSupervisorBase()0
} //end destructor

//========================================================================================================================
xoap::MessageReference FESupervisor::frontEndCommunicationRequest(xoap::MessageReference message)
try
{
	__SUP_COUT__<< "FE Request received: " << SOAPUtilities::translate(message) << __E__;

	if (!theFEInterfacesManager_)
	{
		__SUP_SS__ << "No FE Interface Manager!" << std::endl;
		__SUP_SS_THROW__;
	}
	SOAPParameters typeParameter, rxParameters;  //params for xoap to recv
	typeParameter.addParameter("type");
	SOAPUtilities::receive(message, typeParameter);

	std::string type = typeParameter.getValue("type");

	rxParameters.addParameter("requester");
	rxParameters.addParameter("targetInterfaceID");

	if(type == "feSend")
	{
		__SUP_COUTV__(type);

		rxParameters.addParameter("value");
		SOAPUtilities::receive(message, rxParameters);

		std::string requester = rxParameters.getValue("requester");
		std::string targetInterfaceID = rxParameters.getValue("targetInterfaceID");
		std::string value = rxParameters.getValue("value");

		__SUP_COUTV__(requester);
		__SUP_COUTV__(targetInterfaceID);
		__SUP_COUTV__(value);

		//test that the interface exists
		theFEInterfacesManager_->getFEInterface(targetInterfaceID);

		//mutex scope
		{
			std::lock_guard<std::mutex> lock(
					theFEInterfacesManager_->frontEndCommunicationReceiveMutex_);

			theFEInterfacesManager_->frontEndCommunicationReceiveBuffer_
			[targetInterfaceID][requester].emplace(value);

			__SUP_COUT__ << "Number of target interface ID '" << targetInterfaceID <<
					"' buffers: " << theFEInterfacesManager_->frontEndCommunicationReceiveBuffer_
					[targetInterfaceID].size() << __E__;
			__SUP_COUT__ << "Number of source interface ID '" << requester <<
					"' values received: " << theFEInterfacesManager_->frontEndCommunicationReceiveBuffer_
					[targetInterfaceID][requester].size() << __E__;
		}
		return SOAPUtilities::makeSOAPMessageReference("Received");
	} //end type feSend
	else if(type == "feMacro")
	{
		__SUP_COUTV__(type);

		rxParameters.addParameter("feMacroName");
		rxParameters.addParameter("inputArgs");

		SOAPUtilities::receive(message, rxParameters);

		std::string requester = rxParameters.getValue("requester");
		std::string targetInterfaceID = rxParameters.getValue("targetInterfaceID");
		std::string feMacroName = rxParameters.getValue("feMacroName");
		std::string inputArgs = rxParameters.getValue("inputArgs");

		__SUP_COUTV__(requester);
		__SUP_COUTV__(targetInterfaceID);
		__SUP_COUTV__(feMacroName);
		__SUP_COUTV__(inputArgs);

		std::string outputArgs;
		try
		{
			theFEInterfacesManager_->runFEMacroByFE(
					requester,
					targetInterfaceID, feMacroName,
					inputArgs, outputArgs);
		}
		catch(std::runtime_error &e)
		{
			__SUP_SS__ << "In Supervisor with LID=" << getApplicationDescriptor()->getLocalId() <<
					" the FE Macro named '" << feMacroName << "' with target FE '" <<
					targetInterfaceID << "' failed. Here is the error:\n\n" << e.what() << std::endl;
			__SUP_SS_THROW__;
		}
		catch(...)
		{
			__SUP_SS__ << "In Supervisor with LID=" << getApplicationDescriptor()->getLocalId() <<
					" the FE Macro named '" << feMacroName << "' with target FE '" <<
					targetInterfaceID << "' failed due to an unknown error." << __E__;
			__SUP_SS_THROW__;
		}

		__SUP_COUTV__(outputArgs);

		xoap::MessageReference replyMessage =
			SOAPUtilities::makeSOAPMessageReference("feMacrosResponse");
		SOAPParameters txParameters;
		txParameters.addParameter("requester", requester);
		txParameters.addParameter("targetInterfaceID", targetInterfaceID);
		txParameters.addParameter("feMacroName", feMacroName);
		txParameters.addParameter("outputArgs", outputArgs);
		SOAPUtilities::addParameters(replyMessage, txParameters);


		__SUP_COUT__ << "Sending FE macro result: " <<
				SOAPUtilities::translate(replyMessage) << __E__;

		return replyMessage;
	} //end type feMacro
	else if(type == "feMacroMultiDimensionalStart") //from iterator
	{
		__SUP_COUTV__(type);

		rxParameters.addParameter("feMacroName");
		rxParameters.addParameter("inputArgs");

		SOAPUtilities::receive(message, rxParameters);

		std::string requester = rxParameters.getValue("requester");
		std::string targetInterfaceID = rxParameters.getValue("targetInterfaceID");
		std::string feMacroName = rxParameters.getValue("feMacroName");
		std::string inputArgs = rxParameters.getValue("inputArgs");

		__SUP_COUTV__(requester);
		__SUP_COUTV__(targetInterfaceID);
		__SUP_COUTV__(feMacroName);
		__SUP_COUTV__(inputArgs);


		try
		{
			theFEInterfacesManager_->startFEMacroMultiDimensional(
					requester,
					targetInterfaceID, feMacroName,
					inputArgs);
		}
		catch(std::runtime_error &e)
		{
			__SUP_SS__ << "In Supervisor with LID=" << getApplicationDescriptor()->getLocalId() <<
					" the FE Macro named '" << feMacroName << "' with target FE '" <<
					targetInterfaceID << "' failed to start multi-dimensional launch. " <<
					"Here is the error:\n\n" << e.what() << std::endl;
			__SUP_SS_THROW__;
		}
		catch(...)
		{
			__SUP_SS__ << "In Supervisor with LID=" << getApplicationDescriptor()->getLocalId() <<
					" the FE Macro named '" << feMacroName << "' with target FE '" <<
					targetInterfaceID << "' failed to start multi-dimensional launch " <<
					"due to an unknown error." << __E__;
			__SUP_SS_THROW__;
		}

		xoap::MessageReference replyMessage =
			SOAPUtilities::makeSOAPMessageReference(type + "Done");
		SOAPParameters txParameters;
		//txParameters.addParameter("started", "1");
		SOAPUtilities::addParameters(replyMessage, txParameters);


		__SUP_COUT__ << "Sending FE macro result: " <<
				SOAPUtilities::translate(replyMessage) << __E__;

		return replyMessage;
	} //end type feMacroMultiDimensionalStart
	else if(type == "feMacroMultiDimensionalCheck")
	{
		__SUP_COUTV__(type);

		rxParameters.addParameter("feMacroName");
		rxParameters.addParameter("targetInterfaceID");

		SOAPUtilities::receive(message, rxParameters);

		std::string targetInterfaceID = rxParameters.getValue("targetInterfaceID");
		std::string feMacroName = rxParameters.getValue("feMacroName");

		__SUP_COUTV__(targetInterfaceID);
		__SUP_COUTV__(feMacroName);


		std::string outputArgs;
		bool done = false;
		try
		{
			done = theFEInterfacesManager_->checkFEMacroMultiDimensional(
					targetInterfaceID, feMacroName);
		}
		catch(std::runtime_error &e)
		{
			__SUP_SS__ << "In Supervisor with LID=" << getApplicationDescriptor()->getLocalId() <<
					" the FE Macro named '" << feMacroName << "' with target FE '" <<
					targetInterfaceID << "' failed to check multi-dimensional launch. " <<
					"Here is the error:\n\n" << e.what() << std::endl;
			__SUP_SS_THROW__;
		}
		catch(...)
		{
			__SUP_SS__ << "In Supervisor with LID=" << getApplicationDescriptor()->getLocalId() <<
					" the FE Macro named '" << feMacroName << "' with target FE '" <<
					targetInterfaceID << "' failed to check multi-dimensional launch " <<
					"due to an unknown error." << __E__;
			__SUP_SS_THROW__;
		}

		__SUP_COUTV__(outputArgs);

		xoap::MessageReference replyMessage =
			SOAPUtilities::makeSOAPMessageReference(type + "Done");
		SOAPParameters txParameters;
		txParameters.addParameter("Done", done?"1":"0");
		SOAPUtilities::addParameters(replyMessage, txParameters);


		__SUP_COUT__ << "Sending FE macro result: " <<
				SOAPUtilities::translate(replyMessage) << __E__;

		return replyMessage;
	} //end type feMacroMultiDimensionalCheck
	else
	{
		__SUP_SS__ << "Unrecognized FE Communication type: " << type << __E__;
		__SUP_SS_THROW__;
	}
}
catch(const std::runtime_error& e)
{
	__SUP_SS__ << "Error encountered processing FE communication request: " <<
			e.what() << __E__;
	__SUP_COUT_ERR__ << ss.str();

	SOAPParameters parameters;
	parameters.addParameter("Error", ss.str());
	return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "FailFECommunicationRequest",
			parameters);
}
catch(...)
{
	__SUP_SS__ << "Unknown error encountered processing FE communication request." << __E__;
	__SUP_COUT_ERR__ << ss.str();

	SOAPParameters parameters;
	parameters.addParameter("Error", ss.str());
	return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "FailFECommunicationRequest",
				parameters);
} //end frontEndCommunicationRequest()

//========================================================================================================================
// macroMakerSupervisorRequest
//	 Handles all MacroMaker Requests:
//		- GetInterfaces (returns interface type and id)
//
//	Note: this code assumes a CoreSupervisorBase has only one
//		FEVInterfacesManager in its vector of state machines
xoap::MessageReference FESupervisor::macroMakerSupervisorRequest(xoap::MessageReference message)
{
	__SUP_COUT__ << "$$$$$$$$$$$$$$$$$" << std::endl;

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

	SOAPUtilities::receive(message, parameters);
	std::string request = parameters.getValue("Request");
	std::string addressStr = parameters.getValue("Address");
	std::string dataStr = parameters.getValue("Data");
	std::string InterfaceID = parameters.getValue("InterfaceID");

	__SUP_COUT__ << "request: " << request << std::endl;

	__SUP_COUT__ << "Address: " << addressStr << " Data: "
		<< dataStr << " InterfaceID: " << InterfaceID << std::endl;

	SOAPParameters retParameters;

	try
	{
		if (request == "GetInterfaces")
		{
			if (theFEInterfacesManager_)
				retParameters.addParameter("FEList",
					theFEInterfacesManager_->getFEListString(
						std::to_string(getApplicationDescriptor()->getLocalId())));
			else //if no FE interfaces, return empty string
				retParameters.addParameter("FEList", "");

			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "Response",
				retParameters);
		}
		else if (request == "UniversalWrite")
		{
			if (!theFEInterfacesManager_)
			{
				__SUP_COUT_INFO__ << "No FE Interface Manager! (So no write occurred)" << std::endl;
				return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "DataWritten", retParameters);
			}

			// parameters interface index!
			//unsigned int index = stoi(indexStr);	// As long as the supervisor has only one interface, this index will remain 0?



			__SUP_COUT__ << "theFEInterfacesManager_->getInterfaceUniversalAddressSize(index) " <<
				theFEInterfacesManager_->getInterfaceUniversalAddressSize(InterfaceID) << std::endl;
			__SUP_COUT__ << "theFEInterfacesManager_->getInterfaceUniversalDataSize(index) " <<
				theFEInterfacesManager_->getInterfaceUniversalDataSize(InterfaceID) << std::endl;

			//Converting std::string to char*
			//char address

			char tmpHex[3]; //for use converting hex to binary
			tmpHex[2] = '\0';


			__SUP_COUT__ << "Translating address: ";

			std::string addressTmp; addressTmp.reserve(theFEInterfacesManager_->getInterfaceUniversalAddressSize(InterfaceID));
			char* address = &addressTmp[0];

			if (addressStr.size() % 2) //if odd, make even
				addressStr = "0" + addressStr;
			unsigned int i = 0;
			for (; i < addressStr.size() &&
				i / 2 < theFEInterfacesManager_->getInterfaceUniversalAddressSize(InterfaceID); i += 2)
			{
				tmpHex[0] = addressStr[addressStr.size() - 1 - i - 1];
				tmpHex[1] = addressStr[addressStr.size() - 1 - i];
				sscanf(tmpHex, "%hhX", (unsigned char*)&address[i / 2]);
				printf("%2.2X", (unsigned char)address[i / 2]);
			}
			//finish and fill with 0s
			for (; i / 2 < theFEInterfacesManager_->getInterfaceUniversalAddressSize(InterfaceID); i += 2)
			{
				address[i / 2] = 0;
				printf("%2.2X", (unsigned char)address[i / 2]);
			}

			std::cout << std::endl;

			__SUP_COUT__ << "Translating data: ";

			std::string dataTmp; dataTmp.reserve(theFEInterfacesManager_->getInterfaceUniversalDataSize(InterfaceID));
			char* data = &dataTmp[0];

			if (dataStr.size() % 2) //if odd, make even
				dataStr = "0" + dataStr;

			i = 0;
			for (; i < dataStr.size() &&
				i / 2 < theFEInterfacesManager_->getInterfaceUniversalDataSize(InterfaceID); i += 2)
			{
				tmpHex[0] = dataStr[dataStr.size() - 1 - i - 1];
				tmpHex[1] = dataStr[dataStr.size() - 1 - i];
				sscanf(tmpHex, "%hhX", (unsigned char*)&data[i / 2]);
				printf("%2.2X", (unsigned char)data[i / 2]);
			}
			//finish and fill with 0s
			for (; i / 2 < theFEInterfacesManager_->getInterfaceUniversalDataSize(InterfaceID); i += 2)
			{
				data[i / 2] = 0;
				printf("%2.2X", (unsigned char)data[i / 2]);
			}

			std::cout << std::endl;

			//char* address = new char[addressStr.size() + 1];
			//std::copy(addressStr.begin(), addressStr.end(), address);
			//    	address[addressStr.size()] = '\0';
			//    	char* data = new char[dataStr.size() + 1];
			//		std::copy(dataStr.begin(), dataStr.end(), data);
			//		data[dataStr.size()] = '\0';

			theFEInterfacesManager_->universalWrite(InterfaceID, address, data);

			//delete[] address;
			//delete[] data;

			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "DataWritten", retParameters);
		}
		else if (request == "UniversalRead")
		{
			if (!theFEInterfacesManager_)
			{
				__SUP_COUT_INFO__ << "No FE Interface Manager! (So no read occurred)" << std::endl;
				return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa", retParameters);
			}

			// parameters interface index!
			// parameter address and data
			//unsigned int index = stoi(indexStr);	// As long as the supervisor has only one interface, this index will remain 0?

			__SUP_COUT__ << "theFEInterfacesManager_->getInterfaceUniversalAddressSize(index) "
				<< theFEInterfacesManager_->getInterfaceUniversalAddressSize(InterfaceID) << std::endl;
			__SUP_COUT__ << "theFEInterfacesManager_->getInterfaceUniversalDataSize(index) "
				<< theFEInterfacesManager_->getInterfaceUniversalDataSize(InterfaceID) << std::endl;

			char tmpHex[3]; //for use converting hex to binary
			tmpHex[2] = '\0';


			__SUP_COUT__ << "Translating address: ";

			std::string addressTmp; addressTmp.reserve(theFEInterfacesManager_->getInterfaceUniversalAddressSize(InterfaceID));
			char* address = &addressTmp[0];

			if (addressStr.size() % 2) //if odd, make even
				addressStr = "0" + addressStr;

			unsigned int i = 0;
			for (; i < addressStr.size() &&
				i / 2 < theFEInterfacesManager_->getInterfaceUniversalAddressSize(InterfaceID); i += 2)
			{
				tmpHex[0] = addressStr[addressStr.size() - 1 - i - 1];
				tmpHex[1] = addressStr[addressStr.size() - 1 - i];
				sscanf(tmpHex, "%hhX", (unsigned char*)&address[i / 2]);
				printf("%2.2X", (unsigned char)address[i / 2]);
			}
			//finish and fill with 0s
			for (; i / 2 < theFEInterfacesManager_->getInterfaceUniversalAddressSize(InterfaceID); i += 2)
			{
				address[i / 2] = 0;
				printf("%2.2X", (unsigned char)address[i / 2]);
			}

			std::cout << std::endl;

			unsigned int dataSz = theFEInterfacesManager_->getInterfaceUniversalDataSize(InterfaceID);
			std::string dataStr; dataStr.resize(dataSz);
			char* data = &dataStr[0];


			//    	std::string result = theFEInterfacesManager_->universalRead(index,address,data);
			//    	__SUP_COUT__<< result << std::endl << std::endl;

			try
			{
				theFEInterfacesManager_->universalRead(InterfaceID, address, data);
			}
			catch (const std::runtime_error& e)
			{
				//do not allow read exception to crash everything when a macromaker command
				__MOUT_ERR__ << "Exception caught during read: " << e.what() << std::endl;
				__SUP_COUT_ERR__ << "Exception caught during read: " << e.what() << std::endl;
				retParameters.addParameter("dataResult", "Time Out Error");
				return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa", retParameters);
			}
			catch (...)
			{
				//do not allow read exception to crash everything when a macromaker command
				__MOUT_ERR__ << "Exception caught during read." << std::endl;
				__SUP_COUT_ERR__ << "Exception caught during read." << std::endl;
				retParameters.addParameter("dataResult", "Time Out Error");
				return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa", retParameters);
			}

			//if dataSz is less than 8 show what the unsigned number would be
			if (dataSz <= 8)
			{
				std::string str8(data);
				str8.resize(8);
				__SUP_COUT__ << "decResult[" << dataSz << " bytes]: " <<
					*((unsigned long long *)(&str8[0])) << std::endl;

			}

			std::string hexResultStr;
			hexResultStr.reserve(dataSz * 2 + 1);
			char* hexResult = &hexResultStr[0];
			//go through each byte and convert it to hex value (i.e. 2 0-F chars)
			//go backwards through source data since should be provided in host order
			//	(i.e. a cast to unsigned long long should acquire expected value)
			for (unsigned int i = 0; i < dataSz; ++i)
			{
				sprintf(&hexResult[i * 2], "%2.2X", (unsigned char)data[dataSz - 1 - i]);
			}

			__SUP_COUT__ << "hexResult[" << strlen(hexResult) << " nibbles]: " << std::string(hexResult) << std::endl;



			retParameters.addParameter("dataResult", hexResult);
			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa", retParameters);

		}
		else if (request == "GetInterfaceMacros")
		{
			if (theFEInterfacesManager_)
				retParameters.addParameter("FEMacros",
						theFEInterfacesManager_->getFEMacrosString(
								supervisorApplicationUID_,
								std::to_string(getApplicationDescriptor()->getLocalId())));
			else
				retParameters.addParameter("FEMacros", "");

			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "Response",
				retParameters);
		}
		else if (request == "RunInterfaceMacro")
		{
			if (!theFEInterfacesManager_)
			{
				retParameters.addParameter("success", "0");
				retParameters.addParameter("outputArgs", "");
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
				theFEInterfacesManager_->runFEMacro(InterfaceID, feMacroName, inputArgs, outputArgs);
			}
			catch (std::runtime_error &e)
			{
				__SUP_SS__ << "In Supervisor with LID=" << getApplicationDescriptor()->getLocalId()
					<< " the FE Macro named '" << feMacroName << "' with target FE '"
					<< InterfaceID << "' failed. Here is the error:\n\n" << e.what() << std::endl;
				__SUP_COUT_ERR__ << "\n" << ss.str();
				success = false;
				outputArgs = ss.str();
			}
			catch (...)
			{
				__SUP_SS__ << "In Supervisor with LID=" << getApplicationDescriptor()->getLocalId()
					<< " the FE Macro named '" << feMacroName << "' with target FE '"
					<< InterfaceID << "' failed due to an unknown error." << __E__;
				__SUP_COUT_ERR__ << "\n" << ss.str();
				success = false;
				outputArgs = ss.str();
			}


			retParameters.addParameter("success", success ? "1" : "0");
			retParameters.addParameter("outputArgs", outputArgs);

			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "Response",
				retParameters);
		}
		else
		{
			__SUP_COUT_WARN__ << "Unrecognized request received! '" << request << "'" << std::endl;
		}
	}
	catch (const std::runtime_error& e)
	{
		__SUP_SS__ << "Error occurred handling request: " << e.what() << __E__;
		__SUP_COUT_ERR__ << ss.str();
		retParameters.addParameter("Error", ss.str());
	}
	catch (...)
	{
		__SUP_SS__ << "Error occurred handling request." << __E__;
		__SUP_COUT_ERR__ << ss.str();
		retParameters.addParameter("Error", ss.str());
	}



	return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "FailRequest", retParameters);

} //end macroMakerSupervisorRequest()


//========================================================================================================================
xoap::MessageReference FESupervisor::workLoopStatusRequest(xoap::MessageReference message)
{
	if (!theFEInterfacesManager_)
	{
		__SUP_SS__ << "Invalid request for front-end workloop status from Supervisor without a FEVInterfacesManager."
			<< std::endl;
		__SUP_COUT_ERR__ << ss.str();
		__SUP_SS_THROW__;
	}

	return SOAPUtilities::makeSOAPMessageReference(
		(theFEInterfacesManager_->allFEWorkloopsAreDone() ?
			CoreSupervisorBase::WORK_LOOP_DONE :
			CoreSupervisorBase::WORK_LOOP_WORKING));
} //end workLoopStatusRequest()

//========================================================================================================================
//extractFEInterfaceManager
//
//	locates theFEInterfacesManager in state machines vector and
//		returns 0 if not found.
//
//	Note: this code assumes a CoreSupervisorBase has only one
//		FEVInterfacesManager in its vector of state machines
FEVInterfacesManager* FESupervisor::extractFEInterfacesManager()
{
	theFEInterfacesManager_ = 0;

	for (unsigned int i = 0; i < theStateMachineImplementation_.size(); ++i)
	{
		try
		{
			theFEInterfacesManager_ =
				dynamic_cast<FEVInterfacesManager*>(theStateMachineImplementation_[i]);
			if (!theFEInterfacesManager_)
			{
				//dynamic_cast returns null pointer on failure
				__SUP_SS__ << "Dynamic cast failure!" << std::endl;
				__SUP_COUT_ERR__ << ss.str();
				__SUP_SS_THROW__;
			}
			__SUP_COUT__ << "State Machine " << i << " WAS of type FEVInterfacesManager" << std::endl;

			break;
		}
		catch (...)
		{
			__SUP_COUT__ << "State Machine " << i << " was NOT of type FEVInterfacesManager" << std::endl;
		}
	}

	__SUP_COUT__ << "theFEInterfacesManager pointer = " << theFEInterfacesManager_ << std::endl;

	return theFEInterfacesManager_;
} // end extractFEInterfaceManager()






