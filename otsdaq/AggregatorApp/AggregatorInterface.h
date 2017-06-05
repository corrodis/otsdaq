#ifndef _ots_ARTDAQAggregatorInterface_h_
#define _ots_ARTDAQAggregatorInterface_h_

#include <future>
#include <memory>
#include <string>
#include "artdaq/Application/AggregatorCore.hh"

namespace ots
{

//class OtsUDPFERConfiguration;

class AggregatorInterface
{
public:

//    AggregatorInterface (std::string name, const OtsUDPFERConfiguration* artDAQFERConfiguration);
    AggregatorInterface (int mpi_rank, std::string name);
    AggregatorInterface (AggregatorInterface const&) = delete;
    virtual ~AggregatorInterface();// = default;
    AggregatorInterface& operator=(AggregatorInterface const&) = delete;

    void configure        (fhicl::ParameterSet const& pset);
    void halt             (void);
    void pause            (void);
    void resume           (void);
    void start            (std::string runNumber);
    void stop             (void);

    std::string  getBuilderName       (void){return name_;}

protected:
    //const OtsUDPFERConfiguration* theARTDAQBuilderConfiguration_;
private:
    int mpi_rank_;
    std::string name_;
    std::unique_ptr<artdaq::AggregatorCore> aggregator_ptr_;
    std::future<size_t> aggregator_future_;

    //FIXME These should go...
    std::string report_string_;
    bool external_request_status_;
};

}

#endif
