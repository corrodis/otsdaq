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
	xoap::bind(this, &FESupervisor::macroMakerSupervisorRequest, "MacroMakerSupervisorRequest", XDAQ_NS_URI);
	xoap::bind(this, &FESupervisor::workLoopStatusRequest, "WorkLoopStatusRequest", XDAQ_NS_URI);

	CoreSupervisorBase::theStateMachineImplementation_.push_back(
		new FEVInterfacesManager(
			CoreSupervisorBase::theConfigurationManager_->getNode(CoreSupervisorBase::XDAQContextConfigurationName_),
			CoreSupervisorBase::supervisorConfigurationPath_
		)
	);

}

//========================================================================================================================
FESupervisor::~FESupervisor(void)
{
	//theStateMachineImplementation_ is reset and the object it points to deleted in ~CoreSupervisorBase()0
}


//========================================================================================================================
// macroMakerSupervisorRequest
//	 Handles all MacroMaker Requests:
//		- GetInterfaces (returns interface type and id)
//
//	Note: this code assumes a CoreSupervisorBase has only one
//		FEVInterfacesManager in its vector of state machines
xoap::MessageReference FESupervisor::macroMakerSupervisorRequest(
	xoap::MessageReference message)
	
{
	__COUT__ << "$$$$$$$$$$$$$$$$$" << std::endl;

	//locate theFEInterfacesManager in state machines vector
	FEVInterfacesManager* theFEInterfacesManager = extractFEInterfaceManager();

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

	__COUT__ << "request: " << request << std::endl;

	__COUT__ << "Address: " << addressStr << " Data: "
		<< dataStr << " InterfaceID: " << InterfaceID << std::endl;

	SOAPParameters retParameters;

	try
	{
		if (request == "GetInterfaces")
		{
			if (theFEInterfacesManager)
				retParameters.addParameter("FEList",
					theFEInterfacesManager->getFEListString(
						std::to_string(getApplicationDescriptor()->getLocalId())));
			else
				retParameters.addParameter("FEList", "");

			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "Response",
				retParameters);
		}
		else if (request == "UniversalWrite")
		{
			if (!theFEInterfacesManager)
			{
				__COUT_INFO__ << "No FE Interface Manager! (So no write occurred)" << std::endl;
				return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "DataWritten", retParameters);
			}

			// parameters interface index!
			//unsigned int index = stoi(indexStr);	// As long as the supervisor has only one interface, this index will remain 0?



			__COUT__ << "theFEInterfacesManager->getInterfaceUniversalAddressSize(index) " <<
				theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID) << std::endl;
			__COUT__ << "theFEInterfacesManager->getInterfaceUniversalDataSize(index) " <<
				theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID) << std::endl;

			//Converting std::string to char*
			//char address

			char tmpHex[3]; //for use converting hex to binary
			tmpHex[2] = '\0';


			__COUT__ << "Translating address: ";

			std::string addressTmp; addressTmp.reserve(theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID));
			char* address = &addressTmp[0];

			if (addressStr.size() % 2) //if odd, make even
				addressStr = "0" + addressStr;
			unsigned int i = 0;
			for (; i < addressStr.size() &&
				i / 2 < theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID); i += 2)
			{
				tmpHex[0] = addressStr[addressStr.size() - 1 - i - 1];
				tmpHex[1] = addressStr[addressStr.size() - 1 - i];
				sscanf(tmpHex, "%hhX", (unsigned char*)&address[i / 2]);
				printf("%2.2X", (unsigned char)address[i / 2]);
			}
			//finish and fill with 0s
			for (; i / 2 < theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID); i += 2)
			{
				address[i / 2] = 0;
				printf("%2.2X", (unsigned char)address[i / 2]);
			}

			std::cout << std::endl;

			__COUT__ << "Translating data: ";

			std::string dataTmp; dataTmp.reserve(theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID));
			char* data = &dataTmp[0];

			if (dataStr.size() % 2) //if odd, make even
				dataStr = "0" + dataStr;

			i = 0;
			for (; i < dataStr.size() &&
				i / 2 < theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID); i += 2)
			{
				tmpHex[0] = dataStr[dataStr.size() - 1 - i - 1];
				tmpHex[1] = dataStr[dataStr.size() - 1 - i];
				sscanf(tmpHex, "%hhX", (unsigned char*)&data[i / 2]);
				printf("%2.2X", (unsigned char)data[i / 2]);
			}
			//finish and fill with 0s
			for (; i / 2 < theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID); i += 2)
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

			theFEInterfacesManager->universalWrite(InterfaceID, address, data);

			//delete[] address;
			//delete[] data;

			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "DataWritten", retParameters);
		}
		else if (request == "UniversalRead")
		{
			if (!theFEInterfacesManager)
			{
				__COUT_INFO__ << "No FE Interface Manager! (So no read occurred)" << std::endl;
				return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa", retParameters);
			}

			// parameters interface index!
			// parameter address and data
			//unsigned int index = stoi(indexStr);	// As long as the supervisor has only one interface, this index will remain 0?

			__COUT__ << "theFEInterfacesManager->getInterfaceUniversalAddressSize(index) "
				<< theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID) << std::endl;
			__COUT__ << "theFEInterfacesManager->getInterfaceUniversalDataSize(index) "
				<< theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID) << std::endl;

			char tmpHex[3]; //for use converting hex to binary
			tmpHex[2] = '\0';


			__COUT__ << "Translating address: ";

			std::string addressTmp; addressTmp.reserve(theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID));
			char* address = &addressTmp[0];

			if (addressStr.size() % 2) //if odd, make even
				addressStr = "0" + addressStr;

			unsigned int i = 0;
			for (; i < addressStr.size() &&
				i / 2 < theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID); i += 2)
			{
				tmpHex[0] = addressStr[addressStr.size() - 1 - i - 1];
				tmpHex[1] = addressStr[addressStr.size() - 1 - i];
				sscanf(tmpHex, "%hhX", (unsigned char*)&address[i / 2]);
				printf("%2.2X", (unsigned char)address[i / 2]);
			}
			//finish and fill with 0s
			for (; i / 2 < theFEInterfacesManager->getInterfaceUniversalAddressSize(InterfaceID); i += 2)
			{
				address[i / 2] = 0;
				printf("%2.2X", (unsigned char)address[i / 2]);
			}

			std::cout << std::endl;

			unsigned int dataSz = theFEInterfacesManager->getInterfaceUniversalDataSize(InterfaceID);
			std::string dataStr; dataStr.resize(dataSz);
			char* data = &dataStr[0];


			//    	std::string result = theFEInterfacesManager->universalRead(index,address,data);
			//    	__COUT__<< result << std::endl << std::endl;

			try
			{
				if (theFEInterfacesManager->universalRead(InterfaceID, address, data) < 0)
				{
					retParameters.addParameter("dataResult", "Time Out Error");
					return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa", retParameters);
				}
			}
			catch (const std::runtime_error& e)
			{
				//do not allow read exception to crash everything when a macromaker command
				__MOUT_ERR__ << "Exception caught during read: " << e.what() << std::endl;
				__COUT_ERR__ << "Exception caught during read: " << e.what() << std::endl;
				retParameters.addParameter("dataResult", "Time Out Error");
				return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa", retParameters);
			}
			catch (...)
			{
				//do not allow read exception to crash everything when a macromaker command
				__MOUT_ERR__ << "Exception caught during read." << std::endl;
				__COUT_ERR__ << "Exception caught during read." << std::endl;
				retParameters.addParameter("dataResult", "Time Out Error");
				return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa", retParameters);
			}

			//if dataSz is less than 8 show what the unsigned number would be
			if (dataSz <= 8)
			{
				std::string str8(data);
				str8.resize(8);
				__COUT__ << "decResult[" << dataSz << " bytes]: " <<
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

			__COUT__ << "hexResult[" << strlen(hexResult) << " nibbles]: " << std::string(hexResult) << std::endl;



			retParameters.addParameter("dataResult", hexResult);
			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "aa", retParameters);

		}
		else if (request == "GetInterfaceMacros")
		{
			if (theFEInterfacesManager)
				retParameters.addParameter("FEMacros", theFEInterfacesManager->getFEMacrosString(
					std::to_string(getApplicationDescriptor()->getLocalId())));
			else
				retParameters.addParameter("FEMacros", "");

			return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "Response",
				retParameters);
		}
		else if (request == "RunInterfaceMacro")
		{
			if (!theFEInterfacesManager)
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
				theFEInterfacesManager->runFEMacro(InterfaceID, feMacroName, inputArgs, outputArgs);
			}
			catch (std::runtime_error &e)
			{
				__SS__ << "In Supervisor with LID=" << getApplicationDescriptor()->getLocalId()
					<< " the FE Macro named '" << feMacroName << "' with tartget FE '"
					<< InterfaceID << "' failed. Here is the error:\n\n" << e.what() << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
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
			__COUT_WARN__ << "Unrecognized request received! '" << request << "'" << std::endl;
		}
	}
	catch (const std::runtime_error& e)
	{
		__SS__ << "Error occurred handling request: " << e.what() << __E__;
		__COUT_ERR__ << ss.str();
	}
	catch (...)
	{
		__SS__ << "Error occurred handling request." << __E__;
		__COUT_ERR__ << ss.str();
	}



	return SOAPUtilities::makeSOAPMessageReference(supervisorClassNoNamespace_ + "FailRequest", retParameters);

} //end macroMakerSupervisorRequest()


//========================================================================================================================
xoap::MessageReference FESupervisor::workLoopStatusRequest(xoap::MessageReference message)

{
	//locate theFEInterfacesManager in state machines vector
	FEVInterfacesManager* theFEInterfacesManager = extractFEInterfaceManager();

	if (!theFEInterfacesManager)
	{
		__SS__ << "Invalid request for front-end workloop status from Supervisor without a FEVInterfacesManager."
			<< std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}

	return SOAPUtilities::makeSOAPMessageReference(
		(theFEInterfacesManager->allFEWorkloopsAreDone() ?
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
FEVInterfacesManager* FESupervisor::extractFEInterfaceManager()
{
	FEVInterfacesManager* theFEInterfacesManager = 0;

	for (unsigned int i = 0; i < theStateMachineImplementation_.size(); ++i)
	{
		try
		{
			theFEInterfacesManager =
				dynamic_cast<FEVInterfacesManager*>(theStateMachineImplementation_[i]);
			if (!theFEInterfacesManager)
			{
				//dynamic_cast returns null pointer on failure
				__SS__ << "Dynamic cast failure!" << std::endl;
				__COUT_ERR__ << ss.str();
				throw std::runtime_error(ss.str());
			}
			__COUT__ << "State Machine " << i << " WAS of type FEVInterfacesManager" << std::endl;

			break;
		}
		catch (...)
		{
			__COUT__ << "State Machine " << i << " was NOT of type FEVInterfacesManager" << std::endl;
		}
	}

	__COUT__ << "theFEInterfacesManager pointer = " << theFEInterfacesManager << std::endl;

	return theFEInterfacesManager;
} // end extractFEInterfaceManager()






