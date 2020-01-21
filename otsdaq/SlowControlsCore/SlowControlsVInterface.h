#ifndef _ots_SlowControlsVInterface_h_
#define _ots_SlowControlsVInterface_h_

#include "otsdaq/Configurable/Configurable.h"
#include "otsdaq/FiniteStateMachine/VStateMachine.h"

#include <array>
#include <string>

// clang-format off
namespace ots
{
class SlowControlsVInterface : public Configurable, public VStateMachine
{
  public:
	SlowControlsVInterface(const std::string&       interfaceType,
	                       const std::string&       interfaceUID,
	                       const ConfigurationTree& theXDAQContextConfigTree,
	                       const std::string&       configurationPath)
	    : Configurable(theXDAQContextConfigTree, configurationPath)
  	    , VStateMachine(Configurable::theConfigurationRecordName_)
	    , interfaceUID_(interfaceUID)
	    , interfaceType_(interfaceType)
	    , mfSubject_("controls-" + interfaceType_ + "-" + interfaceUID_)
	{
		// inheriting children classes should use __GEN_COUT_*
		//	for decorations using mfSubject.
		__GEN_COUT__ << __E__;
		__GEN_COUTV__(interfaceUID_);
		__GEN_COUTV__(mfSubject_);
	}

	virtual ~SlowControlsVInterface(void) {}

	virtual void initialize() = 0;


	virtual void                                       	subscribe(std::string Name)       	= 0;
	virtual void                                       	subscribeJSON(std::string List)   	= 0;
	virtual void                                       	unsubscribe(std::string Name)     	= 0;
	virtual std::vector<std::string /*Name*/>           getChannelList()                  	= 0;
	virtual std::string                                	getList(std::string format)       	= 0;
	virtual std::array<std::string, 4>                 	getCurrentValue(std::string Name) 	= 0;
	virtual std::vector<std::vector<std::string>> 	   	getChannelHistory(std::string Name)	= 0;
	virtual std::array<std::string, 9>                 	getSettings(std::string Name)     	= 0;



	virtual void 										configure(void) override {;} //by default do nothing on FSM transitions
	virtual void 										halt(void) override {;} //by default do nothing on FSM transitions
	virtual void 										pause(void) override {;} //by default do nothing on FSM transitions
	virtual void 										resume(void) override {;} //by default do nothing on FSM transitions
	virtual void 										start(std::string runNumber) override {;} //by default do nothing on FSM transitions
	virtual void 										stop(void) override {;} //by default do nothing on FSM transitions

	// States
	virtual bool 										running(void) override { return false; } //This is a workloop/thread, by default do nothing and end thread during running (Note: return true would repeat call)

  protected:
	const std::string interfaceUID_;
	const std::string interfaceType_;
	const std::string mfSubject_;
};
// clang-format on
}  // namespace ots
#endif
