#ifndef _ots_UDPDataStreamerBase_h_
#define _ots_UDPDataStreamerBase_h_

#include "otsdaq-core/NetworkUtilities/TransmitterSocket.h" // Make sure this is always first because <sys/types.h> (defined in Socket.h) must be first
#include "otsdaq-core/NetworkUtilities/ReceiverSocket.h" // Make sure this is always first because <sys/types.h> (defined in Socket.h) must be first
#include <string>

namespace ots
{

class UDPDataStreamerBase : public TransmitterSocket
{
public:
	UDPDataStreamerBase(std::string IPAddress, unsigned int port, std::string toIPAddress, unsigned int toPort);
	virtual ~UDPDataStreamerBase(void);

	int send(const std::string& buffer) { return TransmitterSocket::send(streamToSocket_,buffer); }
	int send(const std::vector<uint32_t>& buffer) { return TransmitterSocket::send(streamToSocket_,buffer); }

protected:
	ReceiverSocket streamToSocket_;
};

}

#endif
