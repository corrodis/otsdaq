
#include "otsdaq/ARTDAQSupervisor/ARTDAQSupervisor.hh"

#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/make_ParameterSet.h"
//#include "messagefacility/MessageLogger/MessageLogger.h"

#include <boost/exception/all.hpp>

//#include <cassert>
//#include <fstream>
//#include <iostream>
//#include "otsdaq-core/TableCore/TableGroupKey.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(ARTDAQSupervisor)

#define ARTDAQ_FCL_PATH std::string(__ENV__("USER_DATA")) + "/" + "ARTDAQConfigurations/"
#define DAQINTERFACE_PORT                    \
	std::atoi(__ENV__("ARTDAQ_BASE_PORT")) + \
	    (partition_ * std::atoi(__ENV__("ARTDAQ_PORTS_PER_PARTITION")))

//========================================================================================================================
ARTDAQSupervisor::ARTDAQSupervisor(xdaq::ApplicationStub* stub)
    : CoreSupervisorBase(stub), partition_(0)
{
	__SUP_COUT__ << "Constructor." << __E__;

	INIT_MF("ARTDAQSupervisor");

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

	Py_XDECREF(daqinterface_ptr_);

	Py_Finalize();
	__SUP_COUT__ << "Destroyed." << __E__;
}  // end destroy()

//========================================================================================================================
void ARTDAQSupervisor::transitionConfiguring(toolbox::Event::Reference e)
{
	__SUP_COUT__ << "Configuring..." << __E__;
	__SUP_COUT__ << SOAPUtilities::translate(theStateMachine_.getCurrentMessage())
	             << __E__;

	std::vector<std::string> brs = std::vector<std::string>();
	brs.push_back("component01");

	ProgressBar pb;
	pb.reset("ARTDAQSupervisor", "Configure");

	__SUP_COUT__ << "Creating boardreader list" << __E__;

	/// TODO: Create boardreader list!

	pb.step();

	__SUP_COUT__ << "Writing boot.txt" << __E__;

	/// TODO: Write boot.txt!

	pb.step();

	__SUP_COUT__ << "Writing configuration files" << __E__;

	/// TODO: Write configuration directory!

	pb.step();

	__SUP_COUT__ << "Calling setdaqcomps" << __E__;
	PyObject* pName1 = PyString_FromString("setdaqcomps");
	PyObject* brlist = PyTuple_New(brs.size());
	for(size_t ii = 0; ii < brs.size(); ++ii)
	{
		PyObject* brString = PyString_FromString(brs[ii].c_str());
		PyTuple_SetItem(brlist, ii, brString);
	}
	PyObject* res1 = PyObject_CallMethodObjArgs(daqinterface_ptr_, pName1, brlist, NULL);
	Py_DECREF(brlist);

	if(res1 == NULL)
	{
		PyErr_Print();
		__SUP_COUT_ERR__ << "Error calling start transition" << __E__;
	}

	pb.step();
	__SUP_COUT__ << "Calling do_boot" << __E__;
	PyObject* pName2      = PyString_FromString("do_boot");
	PyObject* pStateArgs1 = PyString_FromString("boot.txt");
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
	PyObject* pStateArgs2 = Py_BuildValue("[s]", "ots_config");
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
