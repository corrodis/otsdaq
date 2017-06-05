#include "otsdaq/AggregatorApp/AggregatorInterface.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
//#include "otsdaq-demo/UserConfigurationDataFormats/FEROtsUDPHardwareConfiguration.h"
#include <iostream>
#include <set>


using namespace ots;

//========================================================================================================================
AggregatorInterface::AggregatorInterface(int mpi_rank, std::string name) :
  mpi_rank_        (mpi_rank),
  name_            (name)
{
}

//========================================================================================================================
//AggregatorInterface::AggregatorInterface (std::string name, const OtsUDPFERConfiguration* artDAQBuilderConfiguration) :
//        theARTDAQBuilderConfiguration_(artDAQBuilderConfiguration),
//        name_                         (name)
//{}

//========================================================================================================================
AggregatorInterface::~AggregatorInterface(void)
{}

//========================================================================================================================
void AggregatorInterface::configure(fhicl::ParameterSet const& pset)
{
    std::cout << __COUT_HDR_FL__ << "\tConfigure" << std::endl;
    report_string_ = "";

    //aggregator_ptr_.reset(nullptr);
    if (aggregator_ptr_.get() == 0) {
      aggregator_ptr_.reset(new artdaq::AggregatorCore(mpi_rank_, name_));
    external_request_status_ = aggregator_ptr_->initialize(pset);
    } else {
      std::cout << __COUT_HDR_FL__ << "CANNOT RECONFIGURE!" << std::endl;
    }
    if (!external_request_status_) {
      report_string_ = "Error initializing ";
      report_string_.append(name_ + " ");
      report_string_.append("with ParameterSet = \"" + pset.to_string() + "\".");
    }
}

//========================================================================================================================
void AggregatorInterface::halt(void)
{
    std::cout << __COUT_HDR_FL__ << "\tHalt" << std::endl;
    report_string_ = "";
    external_request_status_ = aggregator_ptr_->shutdown();
    if (!external_request_status_) {
      report_string_ = "Error shutting down ";
      report_string_.append(name_ + ".");
    }
}

//========================================================================================================================
void AggregatorInterface::pause(void)
{
    std::cout << __COUT_HDR_FL__ << "\tPause" << std::endl;
    report_string_ = "";
    external_request_status_ = aggregator_ptr_->pause();
    if (!external_request_status_) {
      report_string_ = "Error pausing ";
      report_string_.append(name_ + ".");
    }

    if (aggregator_future_.valid()) {
      aggregator_future_.get();
    }
}

//========================================================================================================================
void AggregatorInterface::resume(void)
{
    std::cout << __COUT_HDR_FL__ << "\tResume" << std::endl;
    report_string_ = "";
    external_request_status_ = aggregator_ptr_->resume();
    if (!external_request_status_) {
      report_string_ = "Error resuming ";
      report_string_.append(name_ + ".");
    }

    aggregator_future_ =
      std::async(std::launch::async, &artdaq::AggregatorCore::process_fragments, aggregator_ptr_.get());

}

//========================================================================================================================
void AggregatorInterface::start(std::string runNumber)
{
    std::cout << __COUT_HDR_FL__ << "\tStart" << std::endl;

    //art::RunNumber_t artRunNumber = boost::lexical_cast<art::RunNumber_t>(runNumber);
    art::RunID runId((art::RunNumber_t)boost::lexical_cast<art::RunNumber_t>(runNumber));

    report_string_ = "";
    external_request_status_ = aggregator_ptr_->start(runId);
    if (!external_request_status_) {
      report_string_ = "Error starting ";
      report_string_.append(name_ + " ");
      report_string_.append("for run number ");
      report_string_.append(boost::lexical_cast<std::string>(runId.run()));
      report_string_.append(".");
    }

      aggregator_future_ = std::async(std::launch::async, &artdaq::AggregatorCore::process_fragments, aggregator_ptr_.get());
}

//========================================================================================================================
void AggregatorInterface::stop(void)
{
    std::cout << __COUT_HDR_FL__ << "\tStop" << std::endl;
    report_string_ = "";
    external_request_status_ = aggregator_ptr_->stop();
    if (!external_request_status_) {
      report_string_ = "Error stopping ";
      report_string_.append(name_ + ".");
    }

    if (aggregator_future_.valid()) {
      aggregator_future_.get();
    }
}
