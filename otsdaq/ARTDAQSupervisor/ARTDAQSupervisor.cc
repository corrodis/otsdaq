
#include "otsdaq/ARTDAQSupervisor/ARTDAQSupervisor.hh"

#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/make_ParameterSet.h"

//#include "otsdaq/TablePlugins/ARTDAQBoardReaderTable.h"
//#include "otsdaq/TablePlugins/ARTDAQBuilderTable.h"
//#include "otsdaq/TablePlugins/ARTDAQDataLoggerTable.h"
//#include "otsdaq/TablePlugins/ARTDAQDispatcherTable.h"

#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>

#include <signal.h>

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(ARTDAQSupervisor)

#define FAKE_CONFIG_NAME "ots_config"
#define DAQINTERFACE_PORT std::atoi(__ENV__("ARTDAQ_BASE_PORT")) + (partition_ * std::atoi(__ENV__("ARTDAQ_PORTS_PER_PARTITION")))

static ARTDAQSupervisor*                         instance        = nullptr;
static std::unordered_map<int, struct sigaction> old_actions     = std::unordered_map<int, struct sigaction>();
static bool                                      sighandler_init = false;
static void                                      signal_handler(int signum)
{
	// Messagefacility may already be gone at this point, TRACE ONLY!
	TRACE_STREAMER(TLVL_ERROR, &("ARTDAQsupervisor")[0], 0, 0, 0) << "A signal of type " << signum
	                                                              << " was caught by ARTDAQSupervisor. Shutting down DAQInterface, "
	                                                                 "then proceeding with default handlers!";

	if(instance)
		instance->destroy();

	sigset_t set;
	pthread_sigmask(SIG_UNBLOCK, NULL, &set);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	TRACE_STREAMER(TLVL_ERROR, &("SharedMemoryManager")[0], 0, 0, 0) << "Calling default signal handler";
	if(signum != SIGUSR2)
	{
		sigaction(signum, &old_actions[signum], NULL);
		kill(getpid(), signum);  // Only send signal to self
	}
	else
	{
		// Send Interrupt signal if parsing SIGUSR2 (i.e. user-defined exception that
		// should tear down ARTDAQ)
		sigaction(SIGINT, &old_actions[SIGINT], NULL);
		kill(getpid(), SIGINT);  // Only send signal to self
	}
}

static void init_sighandler(ARTDAQSupervisor* inst)
{
	static std::mutex            sighandler_mutex;
	std::unique_lock<std::mutex> lk(sighandler_mutex);

	if(!sighandler_init)
	{
		sighandler_init          = true;
		instance                 = inst;
		std::vector<int> signals = {
		    SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGPIPE, SIGALRM, SIGTERM, SIGUSR2, SIGHUP};  // SIGQUIT is used by art in normal operation
		for(auto signal : signals)
		{
			struct sigaction old_action;
			sigaction(signal, NULL, &old_action);

			// If the old handler wasn't SIG_IGN (it's a handler that just
			// "ignore" the signal)
			if(old_action.sa_handler != SIG_IGN)
			{
				struct sigaction action;
				action.sa_handler = signal_handler;
				sigemptyset(&action.sa_mask);
				for(auto sigblk : signals)
				{
					sigaddset(&action.sa_mask, sigblk);
				}
				action.sa_flags = 0;

				// Replace the signal handler of SIGINT with the one described by
				// new_action
				sigaction(signal, &action, NULL);
				old_actions[signal] = old_action;
			}
		}
	}
}

//========================================================================================================================
ARTDAQSupervisor::ARTDAQSupervisor(xdaq::ApplicationStub* stub)
    : CoreSupervisorBase(stub)
    , daqinterface_ptr_(NULL)
    , partition_(getSupervisorProperty("partition", 0))
    , daqinterface_state_("notrunning")
    , runner_thread_(nullptr)
{
	__SUP_COUT__ << "Constructor." << __E__;

	INIT_MF("ARTDAQSupervisor");
	init_sighandler(this);

	// Write out settings file
	auto          settings_file = __ENV__("DAQINTERFACE_SETTINGS");
	std::ofstream o(settings_file, std::ios::trunc);

	setenv("DAQINTERFACE_PARTITION_NUMBER", std::to_string(partition_).c_str(), 1);
	auto logfileName = std::string(__ENV__("OTSDAQ_LOG_DIR")) + "/DAQInteface/DAQInterface_partition" + std::to_string(partition_) + ".log";
	setenv("DAQINTERFACE_LOGFILE", logfileName.c_str(), 1);

	o << "log_directory: " << getSupervisorProperty("log_directory", std::string(__ENV__("OTSDAQ_LOG_DIR"))) << std::endl;
	o << "record_directory: " << getSupervisorProperty("record_directory", ARTDAQTableBase::ARTDAQ_FCL_PATH + "/run_records/") << std::endl;
	o << "package_hashes_to_save: " << getSupervisorProperty("package_hashes_to_save", "[artdaq]") << std::endl;
	// Note that productsdir_for_bash_scripts is REQUIRED!
	o << "productsdir_for_bash_scripts: " << getSupervisorProperty("productsdir_for_bash_scripts", std::string(__ENV__("OTS_PRODUCTS"))) << std::endl;
	o << "boardreader timeout: " << getSupervisorProperty("boardreader_timeout", 30) << std::endl;
	o << "eventbuilder timeout: " << getSupervisorProperty("eventbuilder_timeout", 30) << std::endl;
	o << "datalogger timeout: " << getSupervisorProperty("datalogger_timeout", 30) << std::endl;
	o << "dispatcher timeout: " << getSupervisorProperty("dispatcher_timeout", 30) << std::endl;
	o << "max_fragment_size_bytes: " << getSupervisorProperty("max_fragment_size_bytes", 1048576) << std::endl;
	o << "transfer_plugin_to_use: " << getSupervisorProperty("transfer_plugin_to_use", "Autodetect") << std::endl;
	o << "all_events_to_all_dispatchers: " << std::boolalpha << getSupervisorProperty("all_events_to_all_dispatchers", true) << std::endl;
	o << "data_directory_override: " << getSupervisorProperty("data_directory_override", std::string(__ENV__("ARTDAQ_OUTPUT_DIR"))) << std::endl;
	o << "max_configurations_to_list: " << getSupervisorProperty("max_configurations_to_list", 10) << std::endl;
	o << "disable_unique_rootfile_labels: " << getSupervisorProperty("disable_unique_rootfile_labels", false) << std::endl;
	o << "use_messageviewer: " << std::boolalpha << getSupervisorProperty("use_messageviewer", false) << std::endl;
	o << "fake_messagefacility: " << std::boolalpha << getSupervisorProperty("fake_messagefacility", false) << std::endl;
	o << "advanced_memory_usage: " << std::boolalpha << getSupervisorProperty("advanced_memory_usage", false) << std::endl;
	o << "disable_private_network_bookkeeping: " << std::boolalpha << getSupervisorProperty("disable_private_network_bookkeeping", false) << std::endl;

	o.close();
	__SUP_COUT__ << "Constructed." << __E__;
}  // end constructor()

//========================================================================================================================
ARTDAQSupervisor::~ARTDAQSupervisor(void)
{
	__SUP_COUT__ << "Destructor." << __E__;
	destroy();
	__SUP_COUT__ << "Destructed." << __E__;
}  // end destructor()

//========================================================================================================================
void ARTDAQSupervisor::init(void)
{
	stop_runner_();

	__SUP_COUT__ << "Initializing..." << __E__;
	{
		std::lock_guard<std::mutex> lk(daqinterface_mutex_);

		// allSupervisorInfo_.init(getApplicationContext());
		artdaq::configureMessageFacility("ARTDAQSupervisor");
		__SUP_COUT__ << "artdaq MF configured." << __E__;

		// initialization
		char* daqinterface_dir = getenv("ARTDAQ_DAQINTERFACE_DIR");
		if(daqinterface_dir == NULL)
		{
			__SS__ << "ARTDAQ_DAQINTERFACE_DIR environment variable not set! This "
			          "means that DAQInterface has not been setup!"
			       << __E__;
			__SUP_SS_THROW__;
		}
		else
		{
			__SUP_COUT__ << "Initializing Python" << __E__;
			Py_Initialize();

			__SUP_COUT__ << "Adding DAQInterface directory to PYTHON_PATH" << __E__;
			PyObject* sysPath     = PySys_GetObject((char*)"path");
			PyObject* programName = PyString_FromString(daqinterface_dir);
			PyList_Append(sysPath, programName);
			Py_DECREF(programName);

			__SUP_COUT__ << "Creating Module name" << __E__;
			PyObject* pName = PyString_FromString("rc.control.daqinterface");
			/* Error checking of pName left out */

			__SUP_COUT__ << "Importing module" << __E__;
			PyObject* pModule = PyImport_Import(pName);
			Py_DECREF(pName);

			if(pModule == NULL)
			{
				PyErr_Print();
				__SS__ << "Failed to load rc.control.daqinterface" << __E__;
				__SUP_SS_THROW__;
			}
			else
			{
				__SUP_COUT__ << "Loading python module dictionary" << __E__;
				PyObject* pDict = PyModule_GetDict(pModule);
				if(pDict == NULL)
				{
					PyErr_Print();
					__SS__ << "Unable to load module dictionary" << __E__;
					__SUP_SS_THROW__;
				}
				else
				{
					Py_DECREF(pModule);

					__SUP_COUT__ << "Getting DAQInterface object pointer" << __E__;
					PyObject* di_obj_ptr = PyDict_GetItemString(pDict, "DAQInterface");

					__SUP_COUT__ << "Filling out DAQInterface args struct" << __E__;
					PyObject* pArgs = PyTuple_New(0);

					PyObject* kwargs = Py_BuildValue("{s:s, s:s, s:i, s:i, s:s, s:s}",
					                                 "logpath",
					                                 ".daqint.log",
					                                 "name",
					                                 "DAQInterface",
					                                 "partition_number",
					                                 partition_,
					                                 "rpc_port",
					                                 DAQINTERFACE_PORT,
					                                 "rpc_host",
					                                 "localhost",
					                                 "control_host",
					                                 "localhost");

					__SUP_COUT__ << "Calling DAQInterface Object Constructor" << __E__;
					daqinterface_ptr_ = PyObject_Call(di_obj_ptr, pArgs, kwargs);

					Py_DECREF(di_obj_ptr);
				}
			}
		}

		getDAQState_();
	}
	start_runner_();
	__SUP_COUT__ << "Initialized." << __E__;
}  // end init()

//========================================================================================================================
void ARTDAQSupervisor::destroy(void)
{
	__SUP_COUT__ << "Destroying..." << __E__;

	if(daqinterface_ptr_ != NULL)
	{
		__SUP_COUT__ << "Calling recover transition" << __E__;
		std::lock_guard<std::mutex> lk(daqinterface_mutex_);
		PyObject*                   pName = PyString_FromString("do_recover");
		PyObject*                   res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, NULL);

		__SUP_COUT__ << "Making sure that correct state has been reached" << __E__;
		getDAQState_();
		while(daqinterface_state_ != "stopped")
		{
			getDAQState_();
			__SUP_COUT__ << "State is " << daqinterface_state_ << ", waiting 1s and retrying..." << __E__;
			usleep(1000000);
		}

		Py_XDECREF(daqinterface_ptr_);
		daqinterface_ptr_ = NULL;
	}

	Py_Finalize();
	__SUP_COUT__ << "Destroyed." << __E__;
}  // end destroy()

//========================================================================================================================
void ARTDAQSupervisor::transitionConfiguring(toolbox::Event::Reference event)
{
	__SUP_COUT__ << "transitionConfiguring" << __E__;

	// activate the configuration tree (the first iteration)
	if(RunControlStateMachine::getIterationIndex() == 0 && RunControlStateMachine::getSubIterationIndex() == 0)
	{
		thread_error_message_ = "";
		thread_progress_bar_.resetProgressBar(0);
		last_thread_progress_update_ = time(0);  // initialize timeout timer

		std::pair<std::string /*group name*/, TableGroupKey> theGroup(
		    SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getParameters().getValue("ConfigurationTableGroupName"),
		    TableGroupKey(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getParameters().getValue("ConfigurationTableGroupKey")));

		__SUP_COUT__ << "Configuration table group name: " << theGroup.first << " key: " << theGroup.second << __E__;

		theConfigurationManager_->loadTableGroup(theGroup.first, theGroup.second, true /*doActivate*/);

		// start configuring thread
		std::thread([](ARTDAQSupervisor* as) { ARTDAQSupervisor::configuringThread(as); }, this).detach();

		__SUP_COUT__ << "Configuring thread started." << __E__;

		RunControlStateMachine::indicateSubIterationWork();
	}
	else  // not first time
	{
		std::string errorMessage;
		{
			std::lock_guard<std::mutex> lock(thread_mutex_);  // lock out for remainder of scope
			errorMessage = thread_error_message_;             // theStateMachine_.getErrorMessage();
		}
		int progress = thread_progress_bar_.read();
		__SUP_COUTV__(errorMessage);
		__SUP_COUTV__(progress);
		__SUP_COUTV__(thread_progress_bar_.isComplete());

		// check for done and error messages
		if(errorMessage == "" &&  // if no update in 600 seconds, give up
		   time(0) - last_thread_progress_update_ > 600)
		{
			__SUP_SS__ << "There has been no update from the configuration thread for " << (time(0) - last_thread_progress_update_)
			           << " seconds, assuming something is wrong and giving up! "
			           << "Last progress received was " << progress << __E__;
			errorMessage = ss.str();
		}

		if(errorMessage != "")
		{
			__SUP_SS__ << "Error was caught in configuring thread: " << errorMessage << __E__;
			__SUP_COUT_ERR__ << "\n" << ss.str();

			theStateMachine_.setErrorMessage(ss.str());
			throw toolbox::fsm::exception::Exception("Transition Error" /*name*/,
			                                         ss.str() /* message*/,
			                                         "CoreSupervisorBase::transitionConfiguring" /*module*/,
			                                         __LINE__ /*line*/,
			                                         __FUNCTION__ /*function*/
			);
		}

		if(!thread_progress_bar_.isComplete())
		{
			RunControlStateMachine::indicateSubIterationWork();

			if(last_thread_progress_read_ != progress)
			{
				last_thread_progress_read_   = progress;
				last_thread_progress_update_ = time(0);
			}

			sleep(1 /*seconds*/);
		}
		else
			__SUP_COUT_INFO__ << "Complete configuring transition!" << __E__;
	}

	return;
}  // end transitionConfiguring()

//========================================================================================================================
void ARTDAQSupervisor::configuringThread(ARTDAQSupervisor* theArtdaqSupervisor) try
{
	ProgressBar& progressBar = theArtdaqSupervisor->thread_progress_bar_;

	const std::string& uid = theArtdaqSupervisor->theConfigurationManager_
	                             ->getNode(ConfigurationManager::XDAQ_APPLICATION_TABLE_NAME + "/" +
	                                       theArtdaqSupervisor->CorePropertySupervisorBase::getSupervisorUID() + "/" + "LinkToSupervisorTable")
	                             .getValueAsString();

	__COUT__ << "Supervisor uid is " << uid << ", getting supervisor table node" << __E__;

	const std::string mfSubject_ = theArtdaqSupervisor->supervisorClassNoNamespace_ + "-" + uid;

	ConfigurationTree theSupervisorNode = theArtdaqSupervisor->getSupervisorTableNode();

	progressBar.step();

	const ARTDAQTableBase::ARTDAQInfo& info = ARTDAQTableBase::extractARTDAQInfo(
	    theSupervisorNode,
	    true /*doWriteFHiCL*/,
	    theArtdaqSupervisor->CorePropertySupervisorBase::getSupervisorProperty<size_t>("max_fragment_size_bytes", ARTDAQTableBase::DEFAULT_MAX_FRAGMENT_SIZE),
	    theArtdaqSupervisor->CorePropertySupervisorBase::getSupervisorProperty<size_t>("routing_timeout_ms", ARTDAQTableBase::DEFAULT_ROUTING_TIMEOUT_MS),
	    theArtdaqSupervisor->CorePropertySupervisorBase::getSupervisorProperty<size_t>("routing_retry_count", ARTDAQTableBase::DEFAULT_ROUTING_RETRY_COUNT),
	    &progressBar);

	const std::list<ARTDAQTableBase::ProcessInfo>& readerInfo        = info.processes.at(ARTDAQTableBase::ARTDAQAppType::BoardReader);
	const std::list<ARTDAQTableBase::ProcessInfo>& builderInfo       = info.processes.at(ARTDAQTableBase::ARTDAQAppType::EventBuilder);
	const std::list<ARTDAQTableBase::ProcessInfo>& loggerInfo        = info.processes.at(ARTDAQTableBase::ARTDAQAppType::DataLogger);
	const std::list<ARTDAQTableBase::ProcessInfo>& dispatcherInfo    = info.processes.at(ARTDAQTableBase::ARTDAQAppType::Dispatcher);
	const std::list<ARTDAQTableBase::ProcessInfo>& routingMasterInfo = info.processes.at(ARTDAQTableBase::ARTDAQAppType::RoutingMaster);

	// Check lists
	if(readerInfo.size() == 0)
	{
		__GEN_SS__ << "There must be at least one enabled BoardReader!" << __E__;
		__GEN_SS_THROW__;
		return;
	}
	if(builderInfo.size() == 0)
	{
		__GEN_SS__ << "There must be at least one enabled EventBuilder!" << __E__;
		__GEN_SS_THROW__;
		return;
	}

	progressBar.step();

	__GEN_COUT__ << "Writing boot.txt" << __E__;

	int         debugLevel  = theSupervisorNode.getNode("DAQInterfaceDebugLevel").getValue<int>();
	std::string setupScript = theSupervisorNode.getNode("DAQSetupScript").getValue();

	std::ofstream o(ARTDAQTableBase::ARTDAQ_FCL_PATH + "/boot.txt", std::ios::trunc);
	o << "DAQ setup script: " << setupScript << std::endl;
	o << "debug level: " << debugLevel << std::endl;
	o << std::endl;

	if(info.subsystems.size() > 1)
	{
		for(auto& ss : info.subsystems)
		{
			if(ss.first == 0)
				continue;
			o << "Subsystem id: " << ss.first << std::endl;
			if(ss.second.destination != 0)
			{
				o << "Subsystem destination: " << ss.second.destination << std::endl;
			}
			for(auto& sss : ss.second.sources)
			{
				o << "Subsystem source: " << sss << std::endl;
			}
			o << std::endl;
		}
	}

	for(auto& builder : builderInfo)
	{
		o << "EventBuilder host: " << builder.hostname << std::endl;
		o << "EventBuilder label: " << builder.label << std::endl;
		if(builder.subsystem != 1)
		{
			o << "EventBuilder subsystem: " << builder.subsystem << std::endl;
		}
		o << std::endl;
	}
	for(auto& logger : loggerInfo)
	{
		o << "DataLogger host: " << logger.hostname << std::endl;
		o << "DataLogger label: " << logger.label << std::endl;
		if(logger.subsystem != 1)
		{
			o << "DataLogger subsystem: " << logger.subsystem << std::endl;
		}
		o << std::endl;
	}
	for(auto& dispatcher : dispatcherInfo)
	{
		o << "Dispatcher host: " << dispatcher.hostname << std::endl;
		o << "Dispatcher label: " << dispatcher.label << std::endl;
		if(dispatcher.subsystem != 1)
		{
			o << "Dispatcher subsystem: " << dispatcher.subsystem << std::endl;
		}
		o << std::endl;
	}
	for(auto& rmaster : routingMasterInfo)
	{
		o << "RoutingMaster host: " << rmaster.hostname << std::endl;
		o << "RoutingMaster label: " << rmaster.label << std::endl;
		if(rmaster.subsystem != 1)
		{
			o << "RoutingMaster subsystem: " << rmaster.subsystem << std::endl;
		}
		o << std::endl;
	}
	o.close();

	progressBar.step();

	__GEN_COUT__ << "Building configuration directory" << __E__;

	boost::system::error_code ignored;
	boost::filesystem::remove_all(ARTDAQTableBase::ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME, ignored);
	mkdir((ARTDAQTableBase::ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME).c_str(), 0755);

	for(auto& reader : readerInfo)
	{
		symlink(ARTDAQTableBase::getFlatFHICLFilename(ARTDAQTableBase::ARTDAQAppType::BoardReader, reader.label).c_str(),
		        (ARTDAQTableBase::ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME + "/" + reader.label + ".fcl").c_str());
	}
	for(auto& builder : builderInfo)
	{
		symlink(ARTDAQTableBase::getFlatFHICLFilename(ARTDAQTableBase::ARTDAQAppType::EventBuilder, builder.label).c_str(),
		        (ARTDAQTableBase::ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME + "/" + builder.label + ".fcl").c_str());
	}
	for(auto& logger : loggerInfo)
	{
		symlink(ARTDAQTableBase::getFlatFHICLFilename(ARTDAQTableBase::ARTDAQAppType::DataLogger, logger.label).c_str(),
		        (ARTDAQTableBase::ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME + "/" + logger.label + ".fcl").c_str());
	}
	for(auto& dispatcher : dispatcherInfo)
	{
		symlink(ARTDAQTableBase::getFlatFHICLFilename(ARTDAQTableBase::ARTDAQAppType::Dispatcher, dispatcher.label).c_str(),
		        (ARTDAQTableBase::ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME + "/" + dispatcher.label + ".fcl").c_str());
	}
	for(auto& rmaster : routingMasterInfo)
	{
		symlink(ARTDAQTableBase::getFlatFHICLFilename(ARTDAQTableBase::ARTDAQAppType::RoutingMaster, rmaster.label).c_str(),
		        (ARTDAQTableBase::ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME + "/" + rmaster.label + ".fcl").c_str());
	}

	progressBar.step();

	std::lock_guard<std::mutex> lk(theArtdaqSupervisor->daqinterface_mutex_);
	theArtdaqSupervisor->getDAQState_();
	if(theArtdaqSupervisor->daqinterface_state_ != "stopped" && theArtdaqSupervisor->daqinterface_state_ != "")
	{
		__GEN_SS__ << "Cannot configure DAQInterface because it is in the wrong state"
		           << " (" << theArtdaqSupervisor->daqinterface_state_ << " != stopped)!" << __E__;
		__GEN_SS_THROW__
	}

	__GEN_COUT__ << "Calling setdaqcomps" << __E__;
	__GEN_COUT__ << "Status before setdaqcomps: " << theArtdaqSupervisor->daqinterface_state_ << __E__;
	PyObject* pName1 = PyString_FromString("setdaqcomps");

	PyObject* readerDict = PyDict_New();
	for(auto& reader : readerInfo)
	{
		PyObject* readerName = PyString_FromString(reader.label.c_str());

		PyObject* readerData      = PyList_New(3);
		PyObject* readerHost      = PyString_FromString(reader.hostname.c_str());
		PyObject* readerPort      = PyString_FromString("-1");
		PyObject* readerSubsystem = PyString_FromString(std::to_string(reader.subsystem).c_str());
		PyList_SetItem(readerData, 0, readerHost);
		PyList_SetItem(readerData, 1, readerPort);
		PyList_SetItem(readerData, 2, readerSubsystem);
		PyDict_SetItem(readerDict, readerName, readerData);
	}
	PyObject* res1 = PyObject_CallMethodObjArgs(theArtdaqSupervisor->daqinterface_ptr_, pName1, readerDict, NULL);
	Py_DECREF(readerDict);

	if(res1 == NULL)
	{
		PyErr_Print();
		__GEN_SS__ << "Error calling setdaqcomps transition" << __E__;
		__GEN_SS_THROW__;
	}
	theArtdaqSupervisor->getDAQState_();
	__GEN_COUT__ << "Status after setdaqcomps: " << theArtdaqSupervisor->daqinterface_state_ << __E__;

	progressBar.step();
	__GEN_COUT__ << "Calling do_boot" << __E__;
	__GEN_COUT__ << "Status before boot: " << theArtdaqSupervisor->daqinterface_state_ << __E__;
	PyObject* pName2      = PyString_FromString("do_boot");
	PyObject* pStateArgs1 = PyString_FromString((ARTDAQTableBase::ARTDAQ_FCL_PATH + "/boot.txt").c_str());
	PyObject* res2        = PyObject_CallMethodObjArgs(theArtdaqSupervisor->daqinterface_ptr_, pName2, pStateArgs1, NULL);

	if(res2 == NULL)
	{
		PyErr_Print();
		__GEN_SS__ << "Error calling boot transition" << __E__;
		__GEN_SS_THROW__;
	}

	theArtdaqSupervisor->getDAQState_();
	if(theArtdaqSupervisor->daqinterface_state_ != "booted")
	{
		__GEN_SS__ << "DAQInterface boot transition failed!" << __E__;
		__GEN_SS_THROW__
	}
	__GEN_COUT__ << "Status after boot: " << theArtdaqSupervisor->daqinterface_state_ << __E__;

	progressBar.step();
	__GEN_COUT__ << "Calling do_config" << __E__;
	__GEN_COUT__ << "Status before config: " << theArtdaqSupervisor->daqinterface_state_ << __E__;
	PyObject* pName3      = PyString_FromString("do_config");
	PyObject* pStateArgs2 = Py_BuildValue("[s]", FAKE_CONFIG_NAME);
	PyObject* res3        = PyObject_CallMethodObjArgs(theArtdaqSupervisor->daqinterface_ptr_, pName3, pStateArgs2, NULL);

	if(res3 == NULL)
	{
		PyErr_Print();
		__GEN_SS__ << "Error calling config transition" << __E__;
		__GEN_SS_THROW__;
	}
	theArtdaqSupervisor->getDAQState_();
	// if(theArtdaqSupervisor->daqinterface_state_ != "ready")
	// {
	// 	__GEN_SS__ << "DAQInterface config transition failed!" << __E__
	// 	           << "Supervisor state: "<< theArtdaqSupervisor->daqinterface_state_ <<
	// __E__;
	// 	__GEN_SS_THROW__;
	// }
	__GEN_COUT__ << "Status after config: " << theArtdaqSupervisor->daqinterface_state_ << __E__;
	progressBar.complete();
	__GEN_COUT__ << "Configured." << __E__;

}  // end configuringThread()
catch(const std::runtime_error& e)
{
	__SS__ << "Error was caught while configuring: " << e.what() << __E__;
	__COUT_ERR__ << "\n" << ss.str();
	std::lock_guard<std::mutex> lock(theArtdaqSupervisor->thread_mutex_);  // lock out for remainder of scope
	theArtdaqSupervisor->thread_error_message_ = ss.str();
}
catch(...)
{
	__SS__ << "Unknown error was caught while configuring. Please checked the logs." << __E__;
	__COUT_ERR__ << "\n" << ss.str();
	std::lock_guard<std::mutex> lock(theArtdaqSupervisor->thread_mutex_);  // lock out for remainder of scope
	theArtdaqSupervisor->thread_error_message_ = ss.str();
}  // end configuringThread() error handling

//========================================================================================================================
void ARTDAQSupervisor::transitionHalting(toolbox::Event::Reference event) try
{
	__SUP_COUT__ << "Halting..." << __E__;
	std::lock_guard<std::mutex> lk(daqinterface_mutex_);
	getDAQState_();
	__SUP_COUT__ << "Status before halt: " << daqinterface_state_ << __E__;

	PyObject* pName = PyString_FromString("do_command");
	PyObject* pArg  = PyString_FromString("Shutdown");
	PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, pArg, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SS__ << "Error calling Shutdown transition" << __E__;
		__SUP_SS_THROW__;
	}

	getDAQState_();
	__SUP_COUT__ << "Status after halt: " << daqinterface_state_ << __E__;
	__SUP_COUT__ << "Halted." << __E__;
}  // end transitionHalting()
catch(const std::runtime_error& e)
{
	const std::string transitionName = "Halting";
	// if halting from Failed state, then ignore errors
	if(theStateMachine_.getProvenanceStateName() == RunControlStateMachine::FAILED_STATE_NAME ||
	   theStateMachine_.getProvenanceStateName() == RunControlStateMachine::HALTED_STATE_NAME)
	{
		__SUP_COUT_INFO__ << "Error was caught while halting (but ignoring because "
		                     "previous state was '"
		                  << RunControlStateMachine::FAILED_STATE_NAME << "'): " << e.what() << __E__;
	}
	else  // if not previously in Failed state, then fail
	{
		__SUP_SS__ << "Error was caught while " << transitionName << ": " << e.what() << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception("Transition Error" /*name*/,
		                                         ss.str() /* message*/,
		                                         "ARTDAQSupervisorBase::transition" + transitionName /*module*/,
		                                         __LINE__ /*line*/,
		                                         __FUNCTION__ /*function*/
		);
	}
}  // end transitionHalting() std::runtime_error exception handling
catch(...)
{
	const std::string transitionName = "Halting";
	// if halting from Failed state, then ignore errors
	if(theStateMachine_.getProvenanceStateName() == RunControlStateMachine::FAILED_STATE_NAME ||
	   theStateMachine_.getProvenanceStateName() == RunControlStateMachine::HALTED_STATE_NAME)
	{
		__SUP_COUT_INFO__ << "Unknown error was caught while halting (but ignoring "
		                     "because previous state was '"
		                  << RunControlStateMachine::FAILED_STATE_NAME << "')." << __E__;
	}
	else  // if not previously in Failed state, then fail
	{
		__SUP_SS__ << "Unknown error was caught while " << transitionName << ". Please checked the logs." << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception("Transition Error" /*name*/,
		                                         ss.str() /* message*/,
		                                         "ARTDAQSupervisorBase::transition" + transitionName /*module*/,
		                                         __LINE__ /*line*/,
		                                         __FUNCTION__ /*function*/
		);
	}
}  // end transitionHalting() exception handling

//========================================================================================================================
void ARTDAQSupervisor::transitionInitializing(toolbox::Event::Reference event)
{
	__SUP_COUT__ << "Initializing..." << __E__;
	init();
	__SUP_COUT__ << "Initialized." << __E__;
}  // end transitionInitializing()

//========================================================================================================================
void ARTDAQSupervisor::transitionPausing(toolbox::Event::Reference event)
{
	__SUP_COUT__ << "Pausing..." << __E__;
	std::lock_guard<std::mutex> lk(daqinterface_mutex_);

	getDAQState_();
	__SUP_COUT__ << "Status before pause: " << daqinterface_state_ << __E__;

	PyObject* pName = PyString_FromString("do_command");
	PyObject* pArg  = PyString_FromString("Pause");
	PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, pArg, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SS__ << "Error calling Pause transition" << __E__;
		__SUP_SS_THROW__;
	}

	getDAQState_();
	__SUP_COUT__ << "Status after pause: " << daqinterface_state_ << __E__;

	__SUP_COUT__ << "Paused." << __E__;
}  // end transitionPausing()

//========================================================================================================================
void ARTDAQSupervisor::transitionResuming(toolbox::Event::Reference event)
{
	__SUP_COUT__ << "Resuming..." << __E__;
	std::lock_guard<std::mutex> lk(daqinterface_mutex_);

	getDAQState_();
	__SUP_COUT__ << "Status before resume: " << daqinterface_state_ << __E__;
	PyObject* pName = PyString_FromString("do_command");
	PyObject* pArg  = PyString_FromString("Resume");
	PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, pArg, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SS__ << "Error calling Resume transition" << __E__;
		__SUP_SS_THROW__;
	}
	getDAQState_();
	__SUP_COUT__ << "Status after resume: " << daqinterface_state_ << __E__;
	__SUP_COUT__ << "Resumed." << __E__;
}  // end transitionResuming()

//========================================================================================================================
void ARTDAQSupervisor::transitionStarting(toolbox::Event::Reference event)
{
	__SUP_COUT__ << "Starting..." << __E__;
	{
		std::lock_guard<std::mutex> lk(daqinterface_mutex_);
		getDAQState_();
		__SUP_COUT__ << "Status before start: " << daqinterface_state_ << __E__;
		auto runNumber = SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getParameters().getValue("RunNumber");

		PyObject* pName      = PyString_FromString("do_start_running");
		int       run_number = std::stoi(runNumber);
		PyObject* pStateArgs = PyInt_FromLong(run_number);
		PyObject* res        = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, pStateArgs, NULL);

		if(res == NULL)
		{
			PyErr_Print();
			__SS__ << "Error calling start transition" << __E__;
			__SUP_SS_THROW__;
		}
		getDAQState_();
		__SUP_COUT__ << "Status after start: " << daqinterface_state_ << __E__;
		if(daqinterface_state_ != "running")
		{
			__SS__ << "DAQInterface start transition failed!" << __E__;
			__SUP_SS_THROW__;
		}
	}
	start_runner_();
	__SUP_COUT__ << "Started." << __E__;
}  // end transitionStarting()

//========================================================================================================================
void ARTDAQSupervisor::transitionStopping(toolbox::Event::Reference event)
{
	__SUP_COUT__ << "Stopping..." << __E__;
	std::lock_guard<std::mutex> lk(daqinterface_mutex_);
	getDAQState_();
	__SUP_COUT__ << "Status before stop: " << daqinterface_state_ << __E__;
	PyObject* pName = PyString_FromString("do_stop_running");
	PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SS__ << "Error calling stop transition" << __E__;
		__SUP_SS_THROW__;
	}
	getDAQState_();
	__SUP_COUT__ << "Status after stop: " << daqinterface_state_ << __E__;
	__SUP_COUT__ << "Stopped." << __E__;
}  // end transitionStopping()

//========================================================================================================================
void ots::ARTDAQSupervisor::enteringError(toolbox::Event::Reference event)
{
	__SUP_COUT__ << "Entering error recovery state" << __E__;
	std::lock_guard<std::mutex> lk(daqinterface_mutex_);
	getDAQState_();
	__SUP_COUT__ << "Status before error: " << daqinterface_state_ << __E__;

	PyObject* pName = PyString_FromString("do_recover");
	PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SS__ << "Error calling recover transition" << __E__;
		__SUP_SS_THROW__;
	}
	getDAQState_();
	__SUP_COUT__ << "Status after error: " << daqinterface_state_ << __E__;
	__SUP_COUT__ << "EnteringError DONE." << __E__;

}  // end enteringError()

//========================================================================================================================
void ots::ARTDAQSupervisor::getDAQState_()
{
	//__SUP_COUT__ << "Getting DAQInterface state" << __E__;

	if(daqinterface_ptr_ == nullptr)
	{
		daqinterface_state_ = "";
		return;
	}

	PyObject* pName = PyString_FromString("state");
	PyObject* pArg  = PyString_FromString("DAQInterface");
	PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, pArg, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SS__ << "Error calling state function" << __E__;
		__SUP_SS_THROW__;
		return;
	}
	daqinterface_state_ = std::string(PyString_AsString(res));
	//__SUP_COUT__ << "getDAQState_ DONE: state=" << result << __E__;
}  // end getDAQState_()

//========================================================================================================================
void ots::ARTDAQSupervisor::daqinterfaceRunner_()
{
	TLOG(TLVL_TRACE) << "Runner thread starting";
	runner_running_ = true;
	while(runner_running_)
	{
		if(daqinterface_ptr_ != NULL)
		{
			std::unique_lock<std::mutex> lk(daqinterface_mutex_);
			getDAQState_();
			std::string state_before = daqinterface_state_;

			if(daqinterface_state_ == "running" || daqinterface_state_ == "ready" || daqinterface_state_ == "booted")
			{
				try
				{
					TLOG(TLVL_TRACE) << "Calling DAQInterface::check_proc_heartbeats";
					PyObject* pName = PyString_FromString("check_proc_heartbeats");
					PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, NULL);
					TLOG(TLVL_TRACE) << "Done with DAQInterface::check_proc_heartbeats call";

					if(res == NULL)
					{
						runner_running_ = false;
						lk.unlock();
						PyErr_Print();
						__SS__ << "Error calling check_proc_heartbeats function" << __E__;
						__SUP_SS_THROW__;
						break;
					}
				}
				catch(cet::exception& ex)
				{
					runner_running_ = false;
					lk.unlock();
					PyErr_Print();
					__SS__ << "An cet::exception occurred while calling "
					          "check_proc_heartbeats function: "
					       << ex.explain_self() << __E__;
					__SUP_SS_THROW__;
					break;
				}
				catch(std::exception& ex)
				{
					runner_running_ = false;
					lk.unlock();
					PyErr_Print();
					__SS__ << "An std::exception occurred while calling "
					          "check_proc_heartbeats function: "
					       << ex.what() << __E__;
					__SUP_SS_THROW__;
					break;
				}
				catch(...)
				{
					runner_running_ = false;
					lk.unlock();
					PyErr_Print();
					__SS__ << "An unknown Error occurred while calling runner function" << __E__;
					__SUP_SS_THROW__;
					break;
				}

				getDAQState_();
				if(daqinterface_state_ != state_before)
				{
					runner_running_ = false;
					lk.unlock();
					__SS__ << "DAQInterface state unexpectedly changed from " << state_before << " to " << daqinterface_state_
					       << ". Check supervisor log file for more info!" << __E__;
					__SUP_SS_THROW__;
					break;
				}
			}
		}
		else
		{
			break;
		}
		usleep(1000000);
	}
	runner_running_ = false;
	TLOG(TLVL_TRACE) << "Runner thread complete";
}  // end daqinterfaceRunner_()

//========================================================================================================================
void ots::ARTDAQSupervisor::stop_runner_()
{
	runner_running_ = false;
	if(runner_thread_ && runner_thread_->joinable())
	{
		runner_thread_->join();
		runner_thread_.reset(nullptr);
	}
}  // end stop_runner_()

//========================================================================================================================
void ots::ARTDAQSupervisor::start_runner_()
{
	stop_runner_();
	runner_thread_ = std::make_unique<std::thread>(&ots::ARTDAQSupervisor::daqinterfaceRunner_, this);
}  // end start_runner_()
