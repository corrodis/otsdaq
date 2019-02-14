#ifndef _ots_TCPDataListenerProducer_h_
#define _ots_TCPDataListenerProducer_h_

#include "otsdaq-core/Configurable/Configurable.h"
#include "otsdaq-core/DataManager/DataProducer.h"
#include "otsdaq-core/NetworkUtilities/TCPSocket.h"  // Make sure this is always first because <sys/types.h> (defined in Socket.h) must be first

#include <string>

namespace ots {

class ConfigurationTree;

class TCPDataListenerProducer : public DataProducer, public Configurable, public TCPSocket {
       public:
	TCPDataListenerProducer(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID,
				const ConfigurationTree& theXDAQContextConfigTree, const std::string& configurationPath);
	virtual ~TCPDataListenerProducer(void);

       protected:
	bool workLoopThread(toolbox::task::WorkLoop* workLoop);
	void slowWrite(void);
	void fastWrite(void);
	//For slow write
	std::string			   data_;
	std::map<std::string, std::string> header_;
	//For fast write
	std::string*			    dataP_;
	std::map<std::string, std::string>* headerP_;

	std::string    ipAddress_;
	unsigned short port_;

	//bool getNextFragment(void);
};

}  // namespace ots

#endif
