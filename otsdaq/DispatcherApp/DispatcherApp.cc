#include "otsdaq/DispatcherApp/DispatcherApp.h"

#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/make_ParameterSet.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(DispatcherApp)

#define ARTDAQ_FCL_PATH std::string(__ENV__("USER_DATA")) + "/" + "ARTDAQConfigurations/"
#define ARTDAQ_FILE_PREAMBLE "dispatcher"

//========================================================================================================================
DispatcherApp::DispatcherApp(xdaq::ApplicationStub* s) : CoreSupervisorBase(s)
{
	__SUP_COUT__ << "Constructor." << __E__;

	INIT_MF("DispatcherApp");

	__SUP_COUT__ << "Constructed." << __E__;
}  // end constructor()

//========================================================================================================================
DispatcherApp::~DispatcherApp(void)
{
	__SUP_COUT__ << "Destructor." << __E__;
	destroy();
	__SUP_COUT__ << "Destructed." << __E__;
}  // end destructor()

//========================================================================================================================
void DispatcherApp::init(void)
{
	__SUP_COUT__ << "Initializing..." << __E__;

	// allSupervisorInfo_.init(getApplicationContext());

	artdaq::configureMessageFacility("Dispatcher");
	__SUP_COUT__ << "artdaq MF configured." << __E__;

	// initialization

	std::string    name = "Dispatcher";
	unsigned short port = 5300;
	//    artdaq::setMsgFacAppName(supervisorApplicationUID_, port);
	artdaq::setMsgFacAppName(name, port);
	//    mf::LogDebug(supervisorApplicationUID_) << "artdaq version " <<
	TLOG(TLVL_DEBUG, name + "Supervisor")
	    << "artdaq version "
	    << artdaq::GetPackageBuildInfo::getPackageBuildInfo().getPackageVersion()
	    << ", built "
	    << artdaq::GetPackageBuildInfo::getPackageBuildInfo().getBuildTimestamp();

	// create the DispatcherInterface
	app_name = name;
	my_rank  = this->getApplicationDescriptor()->getLocalId();
	theDispatcherInterface_.reset(new artdaq::DispatcherApp());
	// theDispatcherInterface_ = new DispatcherInterface(mpiSentry_->rank(),
	// local_group_comm, supervisorApplicationUID_ );

	__SUP_COUT__ << "Initialized." << __E__;
}  // end init()

//========================================================================================================================
void DispatcherApp::destroy(void)
{
	__SUP_COUT__ << "Destroying..." << __E__;
	theDispatcherInterface_.reset(nullptr);
	__SUP_COUT__ << "Destroyed." << __E__;
}  // end destroy()

//========================================================================================================================
void DispatcherApp::transitionConfiguring(toolbox::Event::Reference e)
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
	const std::string& uid =
	    theConfigurationManager_
	        ->getNode(ConfigurationManager::XDAQ_APPLICATION_TABLE_NAME + "/" +
	                  CorePropertySupervisorBase::getSupervisorUID() + "/" +
	                  "LinkToSupervisorTable")
	        .getValueAsString();

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
		theDispatcherInterface_->initialize(pset, 0, 0);
	}
	catch(const cet::coded_exception<fhicl::error, &fhicl::detail::translate>& e)
	{
		__SUP_SS__ << "Error was caught while configuring: " << e.what() << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "DispatcherApp::transitionConfiguring" /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
	catch(const cet::exception& e)
	{
		__SUP_SS__ << "Error was caught while configuring: " << e.explain_self() << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "DispatcherApp::transitionConfiguring" /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}
	catch(...)
	{
		__SUP_SS__ << "Unknown error was caught while configuring." << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
		    "Transition Error" /*name*/,
		    ss.str() /* message*/,
		    "DispatcherApp::transitionConfiguring" /*module*/,
		    __LINE__ /*line*/,
		    __FUNCTION__ /*function*/
		);
	}

	__SUP_COUT__ << "Configured." << __E__;
}  // end transitionConfiguring()

//========================================================================================================================
void DispatcherApp::transitionHalting(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Halting..." << __E__;
	try
	{
		theDispatcherInterface_->stop(45, 0);
	}
	catch(...)
	{
		// It is okay for this to fail, esp. if already stopped...
		__SUP_COUT__ << "Ignoring error on halt." << __E__;
	}

	try
	{
		theDispatcherInterface_->shutdown(45);
	}
	catch(...)
	{
		__SUP_COUT_ERR__ << "Error occurred during shutdown! State="
		                 << theDispatcherInterface_->status();
	}

	init();

	__SUP_COUT__ << "Halted." << __E__;
}  // end transitionHalting()

//========================================================================================================================
void DispatcherApp::transitionInitializing(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Initializing..." << __E__;
	init();
	__SUP_COUT__ << "Initialized." << __E__;
}  // end transitionInitializing()

//========================================================================================================================
void DispatcherApp::transitionPausing(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Pausing..." << __E__;
	theDispatcherInterface_->pause(0, 0);
	__SUP_COUT__ << "Paused." << __E__;
}  // end transitionPausing()

//========================================================================================================================
void DispatcherApp::transitionResuming(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Resuming..." << __E__;
	theDispatcherInterface_->resume(0, 0);
	__SUP_COUT__ << "Resumed." << __E__;
}  // end transitionResuming()

//========================================================================================================================
void DispatcherApp::transitionStarting(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Starting..." << __E__;

	auto runNumber = SOAPUtilities::translate(theStateMachine_.getCurrentMessage())
	                     .getParameters()
	                     .getValue("RunNumber");
	try
	{
		art::RunID runId(
		    (art::RunNumber_t)boost::lexical_cast<art::RunNumber_t>(runNumber));
		theDispatcherInterface_->start(runId, 0, 0);
	}
	catch(const boost::exception& e)
	{
		__SUP_SS__ << "Error parsing string to art::RunNumber_t: " << runNumber << __E__;
		__SUP_SS_THROW__;
	}
	__SUP_COUT__ << "Started." << __E__;

	// register configured monitors
	for(const auto& monitor : monitors_)
		registerMonitor(monitor.second /*fcl*/);

}  // end transitionStarting()

//========================================================================================================================
void DispatcherApp::transitionStopping(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Stopping..." << __E__;
	theDispatcherInterface_->stop(45, 0);
	__SUP_COUT__ << "Stopped." << __E__;
}  // end transitionStopping()

//========================================================================================================================
//	request
void DispatcherApp::request(const std::string&               requestType,
                            cgicc::Cgicc&                    cgiIn,
                            HttpXmlDocument&                 xmlOut,
                            const WebUsers::RequestUserInfo& userInfo)
{
	// Commands:
	//		 RegisterMonitor
	//		 UnregisterMonitor

	if(theStateMachine_.getCurrentStateName() != "Configured")
	{
		__SUP_SS__ << "Monitor requests can only take place while Dispatcher"
		              " is in Running state. The current state is '"
		           << theStateMachine_.getCurrentStateName() << ".'" << __E__;
		__SUP_SS_THROW__;
	}

	std::string fcl = StringMacros::decodeURIComponent(
	    CgiDataUtilities::postData(cgiIn, "fcl"));  // from POST

	__SUP_COUTV__(requestType);
	__SUP_COUTV__(fcl);

	if(requestType == "RegisterMonitor")
		registerMonitor(fcl);
	else if(requestType == "UnregisterMonitor")
		unregisterMonitor(fcl);
	else
	{
		__SUP_SS__ << "Unknown Dispatcher request '" << requestType << "' received."
		           << __E__;
		__SUP_SS_THROW__;
	}

}  // end request

//========================================================================================================================
void DispatcherApp::registerMonitor(const std::string& fcl)
{
	__SUP_COUTV__(fcl);

	// load fcl string from dedicated file
	fhicl::ParameterSet pset;
	try
	{
		fhicl::make_ParameterSet(fcl, pset);
		theDispatcherInterface_->register_monitor(pset);
	}
	catch(const cet::coded_exception<fhicl::error, &fhicl::detail::translate>& e)
	{
		__SUP_SS__ << "Error was caught while configuring: " << e.what() << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();

		sendAsyncErrorToGateway(ss.str(), false /*isSoftError*/);
	}
	catch(const cet::exception& e)
	{
		__SUP_SS__ << "Error was caught while configuring: " << e.explain_self() << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();

		sendAsyncErrorToGateway(ss.str(), false /*isSoftError*/);
	}
	catch(...)
	{
		__SUP_SS__ << "Unknown error was caught while configuring." << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();

		sendAsyncErrorToGateway(ss.str(), false /*isSoftError*/);
	}

}  // end registerMonitor()

//========================================================================================================================
void DispatcherApp::unregisterMonitor(const std::string& label)
{
	try
	{
		theDispatcherInterface_->unregister_monitor(label);
	}
	catch(const cet::coded_exception<fhicl::error, &fhicl::detail::translate>& e)
	{
		__SUP_SS__ << "Error was caught while configuring: " << e.what() << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();

		sendAsyncErrorToGateway(ss.str(), false /*isSoftError*/);
	}
	catch(const cet::exception& e)
	{
		__SUP_SS__ << "Error was caught while configuring: " << e.explain_self() << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();

		sendAsyncErrorToGateway(ss.str(), false /*isSoftError*/);
	}
	catch(...)
	{
		__SUP_SS__ << "Unknown error was caught while configuring." << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();

		sendAsyncErrorToGateway(ss.str(), false /*isSoftError*/);
	}

}  // end unregisterMonitor()
