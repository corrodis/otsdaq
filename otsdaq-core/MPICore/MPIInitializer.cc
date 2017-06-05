#include "otsdaq-core/MPICore/MPIInitializer.h"

#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "fhiclcpp/make_ParameterSet.h"



#include <iostream>


using namespace ots;

//========================================================================================================================
MPIInitializer::MPIInitializer(void)
  : local_group_comm_(0), rank_(0)
{
}

//========================================================================================================================
void MPIInitializer::init(std::string name, artdaq::TaskType taskType)
{

	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "BEGIN" << std::endl;

	//artdaq::configureMessageFacility("boardreader");
	//artdaq::configureMessageFacility(name.c_str());

	// initialization
	//FIXME This was FUNNELED
	int const wanted_threading_level { MPI_THREAD_MULTIPLE };

	//std::cout << __COUT_HDR_FL__ << "MPI INITIALIZER RETURNS: " << MPI_Init_thread(0, 0, wanted_threading_level, 0) << std::endl;
	//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "I WAS STUCK" << std::endl;


	//int const wanted_threading_level { MPI_THREAD_FUNNELED };

	//MPI_Comm local_group_comm;
	//std::unique_ptr<artdaq::MPISentry> mpiSentry;

	try
	{
		//mpiSentry_.reset( new artdaq::MPISentry(0, 0, wanted_threading_level, artdaq::TaskType::BoardReaderTask, local_group_comm_) );
		mpiSentry_.reset( new artdaq::MPISentry(0, 0, wanted_threading_level, taskType, local_group_comm_) );
	}
	catch (cet::exception& errormsg)
	{
		std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "DIDN't FIND MPI!" << std::endl;

//		mf::LogError("BoardReaderMain") << errormsg ;
//		mf::LogError("BoardReaderMain") << "MPISentry error encountered in BoardReaderMain; exiting...";
		mf::LogError(name) << errormsg ;
		mf::LogError(name) << "MPISentry error encountered in " << name << "; exiting...";
		throw errormsg;
	}

	rank_ = mpiSentry_->rank();
//	std::string    name = "BoardReader";
	unsigned short port = 5100;

	//artdaq::setMsgFacAppName(name, port);
	mf::LogDebug(name + "InterfaceManager") << "artdaq version " <<
			artdaq::GetPackageBuildInfo::getPackageBuildInfo().getPackageVersion()
			<< ", built " <<
			artdaq::GetPackageBuildInfo::getPackageBuildInfo().getBuildTimestamp();

	// create the BoardReaderApp
	//	  artdaq::BoardReaderApp br_app(local_group_comm, name);
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "END" << std::endl;
}

//========================================================================================================================
MPIInitializer::~MPIInitializer(void)
{
	mpiSentry_.reset(nullptr);
}
