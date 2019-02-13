#ifndef _ots_TCPDataStreamerBase_h_
#define _ots_TCPDataStreamerBase_h_

#include "otsdaq-core/NetworkUtilities/TCPSocket.h" // Make sure this is always first because <sys/types.h> (defined in Socket.h) must be first
#include <string>

namespace ots
{

class TCPDataStreamerBase : public TCPSocket
{
public:
	TCPDataStreamerBase(unsigned int port);
	virtual ~TCPDataStreamerBase(void);

	int send(const std::string& buffer) { return TCPSocket::send(buffer); }
	int send(const std::vector<uint32_t>& buffer) { return TCPSocket::send(buffer); }
	int send(const std::vector<uint16_t>& buffer) { return TCPSocket::send(buffer); }
};

}

#endif
