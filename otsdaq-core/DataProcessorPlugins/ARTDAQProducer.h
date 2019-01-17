#ifndef _ots_ARTDAQProducer_h_
#define _ots_ARTDAQProducer_h_

#include "otsdaq-core/DataManager/DataProducer.h"
#include "otsdaq-core/Configurable/Configurable.h"
#include "artdaq/Application/BoardReaderApp.hh"


#include <future>
#include <memory>
#include <string>

namespace ots
{

//ARTDAQProducer
//	This class is a Data Producer Plugin that allows a single artdaq Event Builder to be instantiate as a Producer to an otsdaq Buffer.
class ARTDAQProducer : public DataProducer, public Configurable
{

public:
    ARTDAQProducer         (std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& configurationPath);
    virtual ~ARTDAQProducer(void);

    void initLocalGroup    (int rank);
    //void destroy          (void);

    //void configure         (fhicl::ParameterSet const& pset);
    void configure           (int rank);
    void halt                (void);
    void pauseProcessingData (void);
    void resumeProcessingData(void);
    void startProcessingData (std::string runNumber) override;
    void stopProcessingData  (void);
    //int universalRead	  (char *address, char *returnValue) override {;}
    //void universalWrite	  (char *address, char *writeValue) override {;}
    //artdaq::BoardReaderCore* getFragmentReceiverPtr(){return fragment_receiver_ptr_.get();}
    //void ProcessData_(artdaq::FragmentPtrs & frags) override;

private:
    bool workLoopThread(toolbox::task::WorkLoop* workLoop){return false;}

    std::unique_ptr<artdaq::BoardReaderApp> fragment_receiver_ptr_;
    std::string                              name_;

    //FIXME These should go...
    std::string          report_string_;
    bool                 external_request_status_;
    fhicl::ParameterSet  fhiclConfiguration_;
};

}

#endif
