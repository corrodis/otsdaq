#ifndef _ots_EventBuilderInterface_h_
#define _ots_EventBuilderInterface_h_

#include <future>
#include <memory>
#include <string>
#include "artdaq/Application/EventBuilderCore.hh"

namespace ots
{

//class OtsUDPFERConfiguration;

class EventBuilderInterface
{
public:

//    EventBuilderInterface (std::string name, const OtsUDPFERConfiguration* artDAQFERConfiguration);
    EventBuilderInterface (int mpi_rank, std::string name);
    EventBuilderInterface (EventBuilderInterface const&) = delete;
    virtual ~EventBuilderInterface();// = default;
    EventBuilderInterface& operator=(EventBuilderInterface const&) = delete;

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
    std::unique_ptr<artdaq::EventBuilderCore> event_builder_ptr_;
    std::future<size_t> event_building_future_;

    //FIXME These should go...
    std::string report_string_;
    bool external_request_status_;
};

}

#endif
