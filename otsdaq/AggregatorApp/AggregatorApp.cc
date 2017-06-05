#include "otsdaq/AggregatorApp/AggregatorApp.h"
#include "otsdaq/AggregatorApp/AggregatorInterface.h"
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
#include "artdaq/DAQdata/configureMessageFacility.hh"
#include "artdaq/DAQrate/quiet_mpi.hh"
#include "cetlib/exception.h"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "fhiclcpp/make_ParameterSet.h"

#include <fstream>
#include <iostream>
#include <cassert>

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(AggregatorApp)

//========================================================================================================================
AggregatorApp::AggregatorApp(xdaq::ApplicationStub * s) throw (xdaq::exception::Exception)
: xdaq::Application           (s)
, SOAPMessenger               (this)
, stateMachineWorkLoopManager_(toolbox::task::bind(this, &AggregatorApp::stateMachineThread, "StateMachine"))
, stateMachineSemaphore_      (toolbox::BSem::FULL)
, theConfigurationManager_    (new ConfigurationManager)//(Singleton<ConfigurationManager>::getInstance()) //I always load the full config but if I want to load a partial configuration (new ConfigurationManager)
, XDAQContextConfigurationName_(theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getConfigurationName())
, supervisorConfigurationPath_ ("INITIALIZED INSIDE THE CONTRUCTOR BECAUSE IT NEEDS supervisorContextUID_ and supervisorApplicationUID_")
, supervisorContextUID_        ("INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
, supervisorApplicationUID_    ("INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
{
  INIT_MF("AggregatorApp");
    xgi::bind (this, &AggregatorApp::Default,                "Default" );
    xgi::bind (this, &AggregatorApp::stateMachineXgiHandler, "StateMachineXgiHandler");

    xoap::bind(this, &AggregatorApp::stateMachineStateRequest, "StateMachineStateRequest", XDAQ_NS_URI );
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
    init();
}

//========================================================================================================================
AggregatorApp::~AggregatorApp(void)
{
    destroy();
}
//========================================================================================================================
void AggregatorApp::init(void)
{
    std::cout << __COUT_HDR_FL__ << "ARTDAQAGGREGATOR SUPERVISOR INIT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	theSupervisorDescriptorInfo_.init(getApplicationContext());
    artdaq::configureMessageFacility("aggregator");

    // initialization

    int const wanted_threading_level { MPI_THREAD_MULTIPLE };
    //int const wanted_threading_level { MPI_THREAD_FUNNELED };

    MPI_Comm local_group_comm;

    try
    {
        mpiSentry_.reset( new artdaq::MPISentry(0, 0, wanted_threading_level, artdaq::TaskType::AggregatorTask, local_group_comm) );
    }
    catch (cet::exception& errormsg)
    {
        mf::LogError("AggregatorMain") << errormsg ;
        mf::LogError("AggregatorMain") << "MPISentry error encountered in AggregatorMain; exiting...";
        throw errormsg;
    }

    std::cout << __COUT_HDR_FL__ << "ARTDAQAGGREGATOR SUPERVISOR INIT4!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::string    name = "Aggregator";
    unsigned short port = 5300;
//    artdaq::setMsgFacAppName(supervisorApplicationUID_, port);
    artdaq::setMsgFacAppName(name, port);
//    mf::LogDebug(supervisorApplicationUID_) << "artdaq version " <<
    mf::LogDebug(name + "Supervisor") << "artdaq version " <<
    artdaq::GetPackageBuildInfo::getPackageBuildInfo().getPackageVersion()
    << ", built " <<
    artdaq::GetPackageBuildInfo::getPackageBuildInfo().getBuildTimestamp();

    // create the AggregatorInterface
    theAggregatorInterface_ = new AggregatorInterface(mpiSentry_->rank(), name );
    //theAggregatorInterface_ = new AggregatorInterface(mpiSentry_->rank(), local_group_comm, supervisorApplicationUID_ );
}

//========================================================================================================================
void AggregatorApp::destroy(void)
{
	delete theAggregatorInterface_;
	//called by destructor
	mpiSentry_.reset();
}

//========================================================================================================================
void AggregatorApp::Default(xgi::Input * in, xgi::Output * out ) throw (xgi::exception::Exception)
{
    
    *out << "<!DOCTYPE HTML><html lang='en'><frameset col='100%' row='100%'><frame src='/WebPath/html/AggregatorApp.html?urn=" <<
    		this->getApplicationDescriptor()->getLocalId() << "'></frameset></html>";
}

//========================================================================================================================
void AggregatorApp::stateMachineXgiHandler(xgi::Input * in, xgi::Output * out ) throw (xgi::exception::Exception)
{}

//========================================================================================================================
void AggregatorApp::stateMachineResultXgiHandler(xgi::Input* in, xgi::Output* out ) throw (xgi::exception::Exception)
{}

//========================================================================================================================
xoap::MessageReference AggregatorApp::stateMachineXoapHandler(xoap::MessageReference message ) throw (xoap::exception::Exception)
{
    std::cout << __COUT_HDR_FL__ << "Soap Handler!" << std::endl;
    stateMachineWorkLoopManager_.removeProcessedRequests();
    stateMachineWorkLoopManager_.processRequest(message);
    std::cout << __COUT_HDR_FL__ << "Done - Soap Handler!" << std::endl;
    return message;
}

//========================================================================================================================
xoap::MessageReference AggregatorApp::stateMachineResultXoapHandler(xoap::MessageReference message ) throw (xoap::exception::Exception)
{
    std::cout << __COUT_HDR_FL__ << "Soap Handler!" << std::endl;
    //stateMachineWorkLoopManager_.removeProcessedRequests();
    //stateMachineWorkLoopManager_.processRequest(message);
    std::cout << __COUT_HDR_FL__ << "Done - Soap Handler!" << std::endl;
    return message;
}

//========================================================================================================================
bool AggregatorApp::stateMachineThread(toolbox::task::WorkLoop* workLoop)
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
xoap::MessageReference AggregatorApp::stateMachineStateRequest(xoap::MessageReference message) throw (xoap::exception::Exception)
{
    std::cout << __COUT_HDR_FL__ << theStateMachine_.getCurrentStateName() << std::endl;
    return SOAPUtilities::makeSOAPMessageReference(theStateMachine_.getCurrentStateName());
}

//========================================================================================================================
void AggregatorApp::stateInitial(toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception)
{
    
}

//========================================================================================================================
void AggregatorApp::stateHalted(toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception)
{
    
}

//========================================================================================================================
void AggregatorApp::stateRunning(toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception)
{
    
}

//========================================================================================================================
void AggregatorApp::stateConfigured(toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception)
{
    
}

//========================================================================================================================
void AggregatorApp::statePaused(toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception)
{
    
}

//========================================================================================================================
void AggregatorApp::inError (toolbox::fsm::FiniteStateMachine & fsm) throw (toolbox::fsm::exception::Exception)
{
    std::cout << __COUT_HDR_FL__ << "Fsm current state: " << theStateMachine_.getCurrentStateName()<< std::endl;
    //rcmsStateNotifier_.stateChanged("Error", "");
}

//========================================================================================================================
void AggregatorApp::enteringError (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception)
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
#define ARTDAQ_FILE_PREAMBLE	"aggregator"
//========================================================================================================================
void AggregatorApp::transitionConfiguring(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception)
{

    std::cout << __COUT_HDR_FL__ << "ARTDAQAGGREGATOR SUPERVISOR CONFIGURING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::cout << __COUT_HDR_FL__  << SOAPUtilities::translate(theStateMachine_.getCurrentMessage()) << std::endl;

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


	std::string path = "";
    char* dirMRB = getenv("MRB_BUILDDIR");
    char* dirP = getenv("OTSDAQ_DIR");

    if(dirMRB) { path = std::string(dirMRB) + "/otsdaq_demo/"; }
    else if(dirP) { path = std::string(dirP) + "/"; }

    //Now that the configuration manager has all the necessary configurations I can create all objects dependent of the configuration
    //std::string configString = "daq:{aggregator:{event_builder_count:2 event_queue_depth:20 event_queue_wait_time:5 expected_events_per_bunch:1 file_duration:0 file_event_count:0 file_size_MB:0 first_event_builder_rank:2 mpi_buffer_count:8 print_event_store_stats:true xmlrpc_client_list:\";http://localhost:5603/RPC2,3;http://localhost:5604/RPC2,3;http://localhost:5605/RPC2,4;http://localhost:5606/RPC2,4;http://localhost:5601/RPC2,5;http://localhost:5602/RPC2,5\"} max_fragment_size_words:2.097152e6} outputs:{normalOutput:{fileName:\"/data/otsdata/data/artdaqots_r%06r_sr%02s_%to.root\" module_type:\"RootOutput\"}} physics:{my_output_modules:[\"normalOutput\"] p2:[\"BuildInfo\"] producers:{BuildInfo:{instance_name:\"ArtdaqOts\" module_type:\"ArtdaqOtsBuildInfo\"}}} process_name:\"DAQAG\" services:{Timing:{summaryOnly:true} scheduler:{fileMode:\"NOMERGE\"} user:{NetMonTransportServiceInterface:{max_fragment_size_words:2.097152e6 service_provider:\"NetMonTransportService\"}}} source:{module_type:\"NetMonInput\"}";
    //ONLY 1 BOARD READER
    //    std::string configString = "daq:{aggregator:{event_builder_count:1 event_queue_depth:20 event_queue_wait_time:5 expected_events_per_bunch:1 file_duration:0 file_event_count:0 file_size_MB:0 first_event_builder_rank:1 mpi_buffer_count:8 print_event_store_stats:true xmlrpc_client_list:\";http://localhost:5100/RPC2,3;http://localhost:5101/RPC2,3;http://localhost:5200/RPC2,4;http://localhost:5201/RPC2,4;http://localhost:5300/RPC2,5;http://localhost:5301/RPC2,5\"} max_fragment_size_words:2.097152e6} outputs:{normalOutput:{fileName:\""+path+"artdaqots_r%06r_sr%02s_%to.root\" module_type:\"RootOutput\"}} physics:{my_output_modules:[\"normalOutput\"] p2:[\"BuildInfo\"] a1:[\"wf\"] producers:{BuildInfo:{instance_name:\"ArtdaqOts\" module_type:\"ArtdaqOtsBuildInfo\"}} analyzers:{wf: {module_type:\"WFViewer\" fragment_ids:[0] fragment_type_labels:[ \"DataGen\" ] prescale:60 write_to_file:true fileName:\""+path+"otsdaqdemo_onmon.root\"}}} process_name:\"DAQAG\" services:{Timing:{summaryOnly:true} scheduler:{fileMode:\"NOMERGE\" errorOnFailureToPut: false} NetMonTransportServiceInterface:{max_fragment_size_words:2097152 service_provider:\"NetMonTransportService\"}} source:{module_type:\"NetMonInput\"}";
    //2 BOARD READERS
    //std::string configString = "daq:{aggregator:{event_builder_count:1 event_queue_depth:20 event_queue_wait_time:5 expected_events_per_bunch:1 file_duration:0 file_event_count:0 file_size_MB:0 first_event_builder_rank:1 mpi_buffer_count:8 print_event_store_stats:true xmlrpc_client_list:\";http://localhost:5100/RPC2,3;http://localhost:5101/RPC2,3;http://localhost:5200/RPC2,4;http://localhost:5201/RPC2,4;http://localhost:5300/RPC2,5;http://localhost:5301/RPC2,5\"} max_fragment_size_words:2.097152e6} outputs:{normalOutput:{fileName:\""+path+"artdaqots_r%06r_sr%02s_%to.root\" module_type:\"RootOutput\"}} physics:{my_output_modules:[\"normalOutput\"] p2:[\"BuildInfo\"] a1:[\"wf\"] producers:{BuildInfo:{instance_name:\"ArtdaqOts\" module_type:\"ArtdaqOtsBuildInfo\"}} analyzers:{wf: {module_type:\"WFViewer\" fragment_ids:[0] fragment_type_labels:[ \"DataGen\" ] prescale:60 write_to_file:true fileName:\""+path+"otsdaqdemo_onmon.root\"}}} process_name:\"DAQAG\" services:{Timing:{summaryOnly:true} scheduler:{fileMode:\"NOMERGE\" errorOnFailureToPut: false} NetMonTransportServiceInterface:{max_fragment_size_words:2097152 service_provider:\"NetMonTransportService\"}} source:{module_type:\"NetMonInput\"}";

    fhicl::ParameterSet pset;
    //fhicl::make_ParameterSet(configString, pset);

	std::string filename = ARTDAQ_FCL_PATH + ARTDAQ_FILE_PREAMBLE + "-";
	std::string uid = theConfigurationManager_->getNode(XDAQContextConfigurationName_).getNode(supervisorConfigurationPath_).getValue();

	__MOUT__ << "uid: " << uid << std::endl;
	for(unsigned int i=0;i<uid.size();++i)
		if((uid[i] >= 'a' && uid[i] <= 'z') ||
				(uid[i] >= 'A' && uid[i] <= 'Z') ||
				(uid[i] >= '0' && uid[i] <= '9')) //only allow alpha numeric in file name
			filename += uid[i];
	filename += ".fcl";


	__MOUT__ << std::endl;
	__MOUT__ << std::endl;
	__MOUT__ << "filename: " << filename << std::endl;

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

	__MOUT__ << fileFclString << std::endl;
	fhicl::make_ParameterSet(fileFclString, pset);
	//fhicl::make_ParameterSet(theConfigurationManager_->getNode(XDAQContextConfigurationName_).getNode(supervisorConfigurationPath_).getNode("ConfigurationString").getValue<std::string>(), pset);


    theAggregatorInterface_->configure(pset);
    mf::LogInfo("AggregatorInterface") << "ARTDAQAGGREGATOR SUPERVISOR DONE CONFIGURING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";

}

//========================================================================================================================
void AggregatorApp::transitionHalting(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception)
{
    theAggregatorInterface_->halt();
}

//========================================================================================================================
void AggregatorApp::transitionInitializing(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception)
{
    
}

//========================================================================================================================
void AggregatorApp::transitionPausing(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception)
{
    theAggregatorInterface_->pause();
}

//========================================================================================================================
void AggregatorApp::transitionResuming(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception)
{
    theAggregatorInterface_->resume();
}

//========================================================================================================================
void AggregatorApp::transitionStarting(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception)
{
    theAggregatorInterface_->start(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getParameters().getValue("RunNumber"));
}

//========================================================================================================================
void AggregatorApp::transitionStopping(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception)
{
    theAggregatorInterface_->stop();
    theAggregatorInterface_->halt();
}
