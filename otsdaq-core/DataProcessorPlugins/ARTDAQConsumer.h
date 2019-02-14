#ifndef _ots_ARTDAQConsumer_h_
#define _ots_ARTDAQConsumer_h_

#include "artdaq/Application/BoardReaderApp.hh"
#include "otsdaq-core/Configurable/Configurable.h"
#include "otsdaq-core/DataManager/DataConsumer.h"

#include <future>
#include <memory>
#include <string>

namespace ots
{
//ARTDAQConsumer
//	This class is a Data Consumer Plugin that allows a single artdaq Event Builder to be instantiate as a Consumer to an otsdaq Buffer.
class ARTDAQConsumer : public DataConsumer, public Configurable
{
  public:
	ARTDAQConsumer (std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& configurationPath);
	virtual ~ARTDAQConsumer (void);

	void initLocalGroup (int rank);
	//void destroy          (void);

	//void configure         (fhicl::ParameterSet const& pset);
	void configure (int rank);
	void halt (void);
	void pauseProcessingData (void);
	void resumeProcessingData (void);
	void startProcessingData (std::string runNumber) override;
	void stopProcessingData (void);
	//int universalRead	  (char *address, char *returnValue) override {;}
	//void universalWrite	  (char *address, char *writeValue) override {;}
	//artdaq::BoardReaderCore* getFragmentReceiverPtr(){return fragment_receiver_ptr_.get();}
	//void ProcessData_(artdaq::FragmentPtrs & frags) override;

  private:
	bool workLoopThread (toolbox::task::WorkLoop* workLoop) { return false; }

	std::unique_ptr<artdaq::BoardReaderApp> fragment_receiver_ptr_;
	std::string                             name_;

	//FIXME These should go...
	std::string         report_string_;
	bool                external_request_status_;
	fhicl::ParameterSet fhiclConfiguration_;
};

}  // namespace ots

#endif
