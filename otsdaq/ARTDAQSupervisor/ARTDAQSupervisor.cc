
#include "otsdaq/ARTDAQSupervisor/ARTDAQSupervisor.hh"

#include "otsdaq-core/TablePlugins/ARTDAQDataLoggerTable.h"
#include "otsdaq-core/TablePlugins/ARTDAQDispatcherTable.h"
#include "otsdaq-core/TablePlugins/ARTDAQBoardReaderTable.h"
#include "otsdaq-core/TablePlugins/ARTDAQBuilderTable.h"

#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/make_ParameterSet.h"
//#include "messagefacility/MessageLogger/MessageLogger.h"

#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>

#include <signal.h>
//#include <cassert>
//#include <fstream>
//#include <iostream>
//#include "otsdaq-core/TableCore/TableGroupKey.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(ARTDAQSupervisor)

#define ARTDAQ_FCL_PATH std::string(__ENV__("USER_DATA")) + "/" + "ARTDAQConfigurations/"
#define FAKE_CONFIG_NAME "ots_config"
#define DAQINTERFACE_PORT                    \
	std::atoi(__ENV__("ARTDAQ_BASE_PORT")) + \
	    (partition_ * std::atoi(__ENV__("ARTDAQ_PORTS_PER_PARTITION")))

static ARTDAQSupervisor*                         instance = nullptr;
static std::unordered_map<int, struct sigaction> old_actions =
    std::unordered_map<int, struct sigaction>();
static bool sighandler_init = false;
static void signal_handler(int signum)
{
	// Messagefacility may already be gone at this point, TRACE ONLY!
	TRACE_STREAMER(TLVL_ERROR, &("ARTDAQsupervisor")[0], 0, 0, 0)
	    << "A signal of type " << signum
	    << " was caught by ARTDAQSupervisor. Shutting down DAQInterface, "
	       "then proceeding with default handlers!";

	if(instance)
		instance->destroy();

	sigset_t set;
	pthread_sigmask(SIG_UNBLOCK, NULL, &set);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	TRACE_STREAMER(TLVL_ERROR, &("SharedMemoryManager")[0], 0, 0, 0)
	    << "Calling default signal handler";
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
		    SIGINT,
		    SIGILL,
		    SIGABRT,
		    SIGFPE,
		    SIGSEGV,
		    SIGPIPE,
		    SIGALRM,
		    SIGTERM,
		    SIGUSR2,
		    SIGHUP};  // SIGQUIT is used by art in normal operation
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
{
	__SUP_COUT__ << "Constructor." << __E__;

	INIT_MF("ARTDAQSupervisor");
	init_sighandler(this);

	// Write out settings file
	auto          settings_file = __ENV__("DAQINTERFACE_SETTINGS");
	std::ofstream o(settings_file, std::ios::trunc);

	o << "log_directory: "
	  << getSupervisorProperty("log_directory", std::string(__ENV__("OTSDAQ_LOG_DIR")))
	  << std::endl;
	o << "record_directory: "
	  << getSupervisorProperty("record_directory", ARTDAQ_FCL_PATH) << std::endl;
	o << "package_hashes_to_save: "
	  << getSupervisorProperty("package_hashes_to_save",
	                           "[artdaq, otsdaq, otsdaq-utilities]")
	  << std::endl;
	// Note that productsdir_for_bash_scripts is REQUIRED!
	o << "productsdir_for_bash_scripts: "
	  << getSupervisorProperty("productsdir_for_bash_scripts") << std::endl;
	o << "boardreader timeout: " << getSupervisorProperty("boardreader_timeout", 30)
	  << std::endl;
	o << "eventbuilder timeout: " << getSupervisorProperty("eventbuilder_timeout", 30)
	  << std::endl;
	o << "datalogger timeout: " << getSupervisorProperty("datalogger_timeout", 30)
	  << std::endl;
	o << "dispatcher timeout: " << getSupervisorProperty("dispatcher_timeout", 30)
	  << std::endl;
	o << "max_fragment_size_bytes: "
	  << getSupervisorProperty("max_fragment_size_bytes", 1048576) << std::endl;
	o << "transfer_plugin_to_use: "
	  << getSupervisorProperty("transfer_plugin_to_use", "Autodetect") << std::endl;
	o << "all_events_to_all_dispatchers: " << std::boolalpha
	  << getSupervisorProperty("all_events_to_all_dispatchers", true) << std::endl;
	o << "data_directory_override: "
	  << getSupervisorProperty("data_directory_override",
	                           std::string(__ENV__("ARTDAQ_OUTPUT_DIR")))
	  << std::endl;
	o << "max_configurations_to_list: "
	  << getSupervisorProperty("max_configurations_to_list", 10) << std::endl;
	o << "disable_unique_rootfile_labels: "
	  << getSupervisorProperty("disable_unique_rootfile_labels", false) << std::endl;
	o << "use_messageviewer: " << std::boolalpha
	  << getSupervisorProperty("use_messageviewer", false) << std::endl;
	o << "fake_messagefacility: " << std::boolalpha
	  << getSupervisorProperty("fake_messagefacility", false) << std::endl;
	o << "advanced_memory_usage: " << std::boolalpha
	  << getSupervisorProperty("advanced_memory_usage", false) << std::endl;

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
	__SUP_COUT__ << "Initializing..." << __E__;

	// allSupervisorInfo_.init(getApplicationContext());
	artdaq::configureMessageFacility("ARTDAQSupervisor");
	__SUP_COUT__ << "artdaq MF configured." << __E__;

	// initialization
	char* daqinterface_dir = getenv("ARTDAQ_DAQINTERFACE_DIR");
	if(daqinterface_dir == NULL)
	{
		__SUP_COUT_ERR__ << "ARTDAQ_DAQINTERFACE_DIR environment variable not set! This "
		                    "means that DAQInterface has not been setup!"
		                 << __E__;
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
			__SUP_COUT_ERR__ << "Failed to load rc.control.daqinterface" << __E__;
		}
		else
		{
			__SUP_COUT__ << "Loading python module dictionary" << __E__;
			PyObject* pDict = PyModule_GetDict(pModule);
			if(pDict == NULL)
			{
				PyErr_Print();
				__SUP_COUT_ERR__ << "Unable to load module dictionary" << __E__;
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

	__SUP_COUT__ << "Initialized." << __E__;
}  // end init()

//========================================================================================================================
void ARTDAQSupervisor::destroy(void)
{
	__SUP_COUT__ << "Destroying..." << __E__;

	if(daqinterface_ptr_ != NULL)
	{
		PyObject* pName = PyString_FromString("recover");
		PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, NULL);

		Py_XDECREF(daqinterface_ptr_);
	}

	Py_Finalize();
	__SUP_COUT__ << "Destroyed." << __E__;
}  // end destroy()

//========================================================================================================================
void ARTDAQSupervisor::transitionConfiguring(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Configuring..." << __E__;
	__SUP_COUT__ << SOAPUtilities::translate(theStateMachine_.getCurrentMessage())
	             << __E__;

	ProgressBar pb;
	pb.reset("ARTDAQSupervisor", "Configure");

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

	const std::string& uid =
	    theConfigurationManager_
	        ->getNode(ConfigurationManager::XDAQ_APPLICATION_TABLE_NAME + "/" +
	                  CorePropertySupervisorBase::getSupervisorUID() + "/" +
	                  "LinkToSupervisorTable")
	        .getValueAsString();
	__SUP_COUT__ << "Supervisor uid is " << uid << ", getting supervisor table node"
	             << __E__;

	auto theSupervisorNode = getSupervisorTableNode();

	pb.step();

	std::list<std::pair<std::string, std::string>> readerInfo;
	{
		__SUP_COUT__ << "Checking for BoardReaders" << __E__;
		auto readersLink = theSupervisorNode.getNode("boardreadersLink");
		if(!readersLink.isDisconnected() && readersLink.getChildren().size() > 0)
		{
			auto readers = readersLink.getChildren();
			__SUP_COUT__ << "There are " << readers.size() << " configured BoardReaders"
			             << __E__;

			for(auto& reader : readers)
			{
				if(reader.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				       .getValue<bool>())
				{
					auto readerUID = reader.second.getNode("SupervisorUID").getValue();
					auto readerHost =
					    reader.second.getNode("DAQInterfaceHostname").getValue();
					__SUP_COUT__ << "Found BoardReader with UID " << readerUID
					             << " and DAQInterface Hostname " << readerHost << __E__;
					readerInfo.push_back(std::make_pair(readerUID, readerHost));

					ARTDAQBoardReaderTable brt;
					brt.outputFHICL(
					    theConfigurationManager_,
					    reader.second,
					    0,
					    readerHost,
					    10000,
					    theConfigurationManager_->__GET_CONFIG__(XDAQContextTable));
				}
				else
				{
					__SUP_COUT__ << "BoardReader "
					             << reader.second.getNode("SupervisorUID").getValue()
					             << " is disabled." << __E__;
				}
			}
		}
		else
		{
			__SUP_COUT_ERR__ << "Error: There should be at least one BoardReader!";
			return;
		}
	}
	pb.step();

	std::list<std::pair<std::string, std::string>> builderInfo;
	{
		auto buildersLink = theSupervisorNode.getNode("eventbuildersLink");
		if(!buildersLink.isDisconnected() && buildersLink.getChildren().size() > 0)
		{
			auto builders = buildersLink.getChildren();

			for(auto& builder : builders)
			{
				if(builder.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				       .getValue<bool>())
				{
					auto builderUID = builder.second.getNode("SupervisorUID").getValue();
					auto builderHost =
					    builder.second.getNode("DAQInterfaceHostname").getValue();
					__SUP_COUT__ << "Found EventBuilder with UID " << builderUID
					             << " and DAQInterface Hostname " << builderHost << __E__;
					builderInfo.push_back(std::make_pair(builderUID, builderHost));

					ARTDAQBuilderTable bt;
					bt.outputFHICL(
					    theConfigurationManager_,
					    builder.second,
					    0,
					    builderHost,
					    10000,
					    theConfigurationManager_->__GET_CONFIG__(XDAQContextTable));
				}
				else
				{
					__SUP_COUT__ << "EventBuilder "
					             << builder.second.getNode("SupervisorUID").getValue()
					             << " is disabled." << __E__;
				}
			}
		}
		else
		{
			__SUP_COUT_ERR__ << "Error: There should be at least one EventBuilder!";
			return;
		}
	}

	pb.step();

	std::list<std::pair<std::string, std::string>> loggerInfo;
	{
		auto dataloggersLink = theSupervisorNode.getNode("dataloggersLink");
		if(!dataloggersLink.isDisconnected())
		{
			auto dataloggers = dataloggersLink.getChildren();

			for(auto& datalogger : dataloggers)
			{
				if(datalogger.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				       .getValue<bool>())
				{
					auto loggerHost =
					    datalogger.second.getNode("DAQInterfaceHostname").getValue();
					auto loggerUID =
					    datalogger.second.getNode("SupervisorUID").getValue();

					__SUP_COUT__ << "Found DataLogger with UID " << loggerUID
					             << " and DAQInterface Hostname " << loggerHost << __E__;
					loggerInfo.push_back(std::make_pair(loggerUID, loggerHost));
					ARTDAQDataLoggerTable dlt;
					dlt.outputFHICL(
					    theConfigurationManager_,
					    datalogger.second,
					    0,
					    loggerHost,
					    10000,
					    theConfigurationManager_->__GET_CONFIG__(XDAQContextTable));
				}
				else
				{
					__SUP_COUT__ << "DataLogger "
					             << datalogger.second.getNode("SupervisorUID").getValue()
					             << " is disabled." << __E__;
				}
			}
		}
	}

	std::list<std::pair<std::string, std::string>> dispatcherInfo;
	{
		auto dispatchersLink = theSupervisorNode.getNode("dispatchersLink");
		if(!dispatchersLink.isDisconnected())
		{
			auto dispatchers = dispatchersLink.getChildren();

			for(auto& dispatcher : dispatchers)
			{
				if(dispatcher.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				       .getValue<bool>())
				{
					auto dispHost =
					    dispatcher.second.getNode("DAQInterfaceHostname").getValue();
					auto dispUID = dispatcher.second.getNode("SupervisorUID").getValue();
					__SUP_COUT__ << "Found Dispatcher with UID " << dispUID
					             << " and DAQInterface Hostname " << dispHost << __E__;
					dispatcherInfo.push_back(std::make_pair(dispUID, dispHost));

					ARTDAQDispatcherTable adt;
					adt.outputFHICL(
					    theConfigurationManager_,
					    dispatcher.second,
					    0,
					    dispHost,
					    10000,
					    theConfigurationManager_->__GET_CONFIG__(XDAQContextTable));
				}
				else
				{
					__SUP_COUT__ << "Dispatcher "
					             << dispatcher.second.getNode("SupervisorUID").getValue()
					             << " is disabled." << __E__;
				}
			}
		}
	}

	// Check lists
	if(readerInfo.size() == 0)
	{
		__SUP_COUT_ERR__ << "There must be at least one enabled BoardReader!" << __E__;
		return;
	}
	if(builderInfo.size() == 0)
	{
		__SUP_COUT_ERR__ << "There must be at least one enabled EventBuilder!" << __E__;
		return;
	}

	pb.step();

	__SUP_COUT__ << "Writing boot.txt" << __E__;

	auto debugLevel = theSupervisorNode.getNode("DAQInterfaceDebugLevel").getValue<int>();
	auto setupScript = theSupervisorNode.getNode("DAQSetupScript").getValue();

	std::ofstream o(ARTDAQ_FCL_PATH + "/boot.txt", std::ios::trunc);
	o << "DAQ setup script: " << setupScript << std::endl;
	o << "debug level: " << debugLevel << std::endl;
	o << std::endl;

	for(auto& builder : builderInfo)
	{
		o << "EventBuilder host: " << builder.second << std::endl;
		o << "EventBuilder label: " << builder.first << std::endl;
		o << std::endl;
	}
	for(auto& logger : loggerInfo)
	{
		o << "DataLogger host: " << logger.second << std::endl;
		o << "DataLogger label: " << logger.first << std::endl;
		o << std::endl;
	}
	for(auto& dispatcher : dispatcherInfo)
	{
		o << "Dispatcher host: " << dispatcher.second << std::endl;
		o << "Dispatcher label: " << dispatcher.first << std::endl;
		o << std::endl;
	}
	o.close();

	pb.step();

	__SUP_COUT__ << "Building configuration directory" << __E__;

	boost::system::error_code ignored;
	boost::filesystem::remove_all(ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME, ignored);
	mkdir((ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME).c_str(), 0755);

	for(auto& br : readerInfo)
	{
		symlink((ARTDAQ_FCL_PATH + "boardReader-" + br.first + ".fcl").c_str(),
		        (ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME + "/" + br.first + ".fcl").c_str());
	}
	for(auto& eb : builderInfo)
	{
		symlink((ARTDAQ_FCL_PATH + "builder-" + eb.first + ".fcl").c_str(),
		        (ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME + "/" + eb.first + ".fcl").c_str());
	}
	for(auto& dl : loggerInfo)
	{
		symlink((ARTDAQ_FCL_PATH + "datalogger-" + dl.first + ".fcl").c_str(),
		        (ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME + "/" + dl.first + ".fcl").c_str());
	}
	for(auto& di : dispatcherInfo)
	{
		symlink((ARTDAQ_FCL_PATH + "dispatcher-" + di.first + ".fcl").c_str(),
		        (ARTDAQ_FCL_PATH + FAKE_CONFIG_NAME + "/" + di.first + ".fcl").c_str());
	}

	pb.step();

	__SUP_COUT__ << "Calling setdaqcomps" << __E__;
	PyObject* pName1 = PyString_FromString("setdaqcomps");

	PyObject* readerDict = PyDict_New();
	for(auto& reader : readerInfo)
	{
		PyObject* readerName = PyString_FromString(reader.first.c_str());

		PyObject* readerData = PyList_New(2);
		PyObject* readerHost = PyString_FromString(reader.second.c_str());
		PyObject* readerPort = PyString_FromString("-1");
		PyList_SetItem(readerData, 0, readerHost);
		PyList_SetItem(readerData, 1, readerPort);
		PyDict_SetItem(readerDict, readerName, readerData);
	}
	PyObject* res1 =
	    PyObject_CallMethodObjArgs(daqinterface_ptr_, pName1, readerDict, NULL);
	Py_DECREF(readerDict);

	if(res1 == NULL)
	{
		PyErr_Print();
		__SUP_COUT_ERR__ << "Error calling start transition" << __E__;
	}

	pb.step();
	__SUP_COUT__ << "Calling do_boot" << __E__;
	PyObject* pName2      = PyString_FromString("do_boot");
	PyObject* pStateArgs1 = PyString_FromString((ARTDAQ_FCL_PATH + "/boot.txt").c_str());
	PyObject* res2 =
	    PyObject_CallMethodObjArgs(daqinterface_ptr_, pName2, pStateArgs1, NULL);

	if(res2 == NULL)
	{
		PyErr_Print();
		__SUP_COUT_ERR__ << "Error calling start transition" << __E__;
	}

	pb.step();
	__SUP_COUT__ << "Calling do_config" << __E__;
	PyObject* pName3      = PyString_FromString("do_config");
	PyObject* pStateArgs2 = Py_BuildValue("[s]", FAKE_CONFIG_NAME);
	PyObject* res3 =
	    PyObject_CallMethodObjArgs(daqinterface_ptr_, pName3, pStateArgs2, NULL);

	if(res3 == NULL)
	{
		PyErr_Print();
		__SUP_COUT_ERR__ << "Error calling start transition" << __E__;
	}
	pb.complete();
	__SUP_COUT__ << "Configured." << __E__;
}  // end transitionConfiguring()

//========================================================================================================================
void ARTDAQSupervisor::transitionHalting(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Halting..." << __E__;
	PyObject* pName = PyString_FromString("do_command");
	PyObject* pArg  = PyString_FromString("Shutdown");
	PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, pArg, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SUP_COUT_ERR__ << "Error calling Shutdown transition" << __E__;
	}

	__SUP_COUT__ << "Halted." << __E__;
}  // end transitionHalting()

//========================================================================================================================
void ARTDAQSupervisor::transitionInitializing(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Initializing..." << __E__;
	init();
	__SUP_COUT__ << "Initialized." << __E__;
}  // end transitionInitializing()

//========================================================================================================================
void ARTDAQSupervisor::transitionPausing(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Pausing..." << __E__;
	PyObject* pName = PyString_FromString("do_command");
	PyObject* pArg  = PyString_FromString("Pause");
	PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, pArg, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SUP_COUT_ERR__ << "Error calling Pause transition" << __E__;
	}
	__SUP_COUT__ << "Paused." << __E__;
}  // end transitionPausing()

//========================================================================================================================
void ARTDAQSupervisor::transitionResuming(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Resuming..." << __E__;

	PyObject* pName = PyString_FromString("do_command");
	PyObject* pArg  = PyString_FromString("Resume");
	PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, pArg, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SUP_COUT_ERR__ << "Error calling Resume transition" << __E__;
	}
	__SUP_COUT__ << "Resumed." << __E__;
}  // end transitionResuming()

//========================================================================================================================
void ARTDAQSupervisor::transitionStarting(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Starting..." << __E__;
	auto runNumber = SOAPUtilities::translate(theStateMachine_.getCurrentMessage())
	                     .getParameters()
	                     .getValue("RunNumber");

	PyObject* pName      = PyString_FromString("do_start_running");
	int       run_number = std::stoi(runNumber);
	PyObject* pStateArgs = PyInt_FromLong(run_number);
	PyObject* res =
	    PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, pStateArgs, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SUP_COUT_ERR__ << "Error calling start transition" << __E__;
	}
	__SUP_COUT__ << "Started." << __E__;
}  // end transitionStarting()

//========================================================================================================================
void ARTDAQSupervisor::transitionStopping(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Stopping..." << __E__;
	PyObject* pName = PyString_FromString("do_stop_running");
	PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SUP_COUT_ERR__ << "Error calling stop transition" << __E__;
	}
	__SUP_COUT__ << "Stopped." << __E__;
}  // end transitionStopping()

void ots::ARTDAQSupervisor::enteringError(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Entering error recovery state" << __E__;

	PyObject* pName = PyString_FromString("do_recover");
	PyObject* res   = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName, NULL);

	if(res == NULL)
	{
		PyErr_Print();
		__SUP_COUT_ERR__ << "Error calling recover transition" << __E__;
	}
	__SUP_COUT__ << "EnteringError DONE." << __E__;
}
