#include "otsdaq/EventBuilderApp/EventBuilderInterface.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

//#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include <iostream>
#include <set>

using namespace ots;

//========================================================================================================================
EventBuilderInterface::EventBuilderInterface(int mpi_rank, std::string name) :
  mpi_rank_        (mpi_rank),
  name_            (name)
{
}

//========================================================================================================================
//EventBuilderInterface::EventBuilderInterface (std::string name, const OtsUDPFERConfiguration* artDAQBuilderConfiguration) :
//        theARTDAQBuilderConfiguration_(artDAQBuilderConfiguration),
//        name_                         (name)
//{}

//========================================================================================================================
EventBuilderInterface::~EventBuilderInterface(void)
{}

//========================================================================================================================
void EventBuilderInterface::configure(fhicl::ParameterSet const& pset)
{
    std::cout << __COUT_HDR_FL__ << "\tConfigure" << std::endl;
    report_string_ = "";
    external_request_status_ = true;

    // in the following block, we first destroy the existing EventBuilder
    // instance, then create a new one.  Doing it in one step does not
    // produce the desired result since that creates a new instance and
    // then deletes the old one, and we need the opposite order.
    //event_builder_ptr_.reset(nullptr);
    if (event_builder_ptr_.get() == 0) {
      event_builder_ptr_.reset(new artdaq::EventBuilderCore(mpi_rank_, name_));
      external_request_status_ = event_builder_ptr_->initialize(pset);
    } else {
      std::cout << __COUT_HDR_FL__ << "CANNOT RECONFIGURE!!!" << std::endl;
    }
    if (! external_request_status_) {
      report_string_ = "Error initializing an EventBuilderCore named";
      report_string_.append(name_ + " with ");
      report_string_.append("ParameterSet = \"" + pset.to_string() + "\".");
    }

//    return external_request_status_;
}

//========================================================================================================================
void EventBuilderInterface::halt(void)
{
    std::cout << __COUT_HDR_FL__ << "\tHalt" << std::endl;
    report_string_ = "";
    // Faking the transition...
    external_request_status_ = event_builder_ptr_->shutdown();
    if (! external_request_status_) {
      report_string_ = "Error shutting down ";
      report_string_.append(name_ + ".");
    }
}

//========================================================================================================================
void EventBuilderInterface::pause(void)
{
    std::cout << __COUT_HDR_FL__ << "\tPause" << std::endl;
    report_string_ = "";
    external_request_status_ = event_builder_ptr_->pause();
    if (! external_request_status_) {
      report_string_ = "Error pausing ";
      report_string_.append(name_ + ".");
    }

    if (event_building_future_.valid()) {
      event_building_future_.get();
    }
}

//========================================================================================================================
void EventBuilderInterface::resume(void)
{
    std::cout << __COUT_HDR_FL__ << "\tResume" << std::endl;
    report_string_ = "";
    external_request_status_ = event_builder_ptr_->resume();
    if (! external_request_status_) {
      report_string_ = "Error resuming ";
      report_string_.append(name_ + ".");
    }

    event_building_future_ =
      std::async(std::launch::async, &artdaq::EventBuilderCore::process_fragments,
                 event_builder_ptr_.get());

}

//========================================================================================================================
void EventBuilderInterface::start(std::string runNumber)
{
    std::cout << __COUT_HDR_FL__ << "\tStart" << std::endl;

    //art::RunNumber_t artRunNumber = boost::lexical_cast<art::RunNumber_t>(runNumber);
    art::RunID runId((art::RunNumber_t)boost::lexical_cast<art::RunNumber_t>(runNumber));

    report_string_ = "";
    external_request_status_ = event_builder_ptr_->start(runId);
    if (! external_request_status_) {
      report_string_ = "Error starting ";
      report_string_.append(name_ + " for run ");
      report_string_.append("number ");
      report_string_.append(boost::lexical_cast<std::string>(runId.run()));
      report_string_.append(".");
    }

    event_building_future_ = std::async(std::launch::async, &artdaq::EventBuilderCore::process_fragments, event_builder_ptr_.get());

//    return external_request_status_;
}

//========================================================================================================================
void EventBuilderInterface::stop(void)
{
    std::cout << __COUT_HDR_FL__ << "\tStart" << std::endl;
	  report_string_ = "";
	  external_request_status_ = event_builder_ptr_->stop();
	  if (! external_request_status_) {
	    report_string_ = "Error stopping ";
	    report_string_.append(name_ + ".");
	  }

	  if (event_building_future_.valid()) {
	    event_building_future_.get();
	  }
}
