#ifndef _TCPTransceiverSocket_h_
#define _TCPTransceiverSocket_h_

#include "otsdaq-core/NetworkUtilities/TCPTransmitterSocket.h"
#include "otsdaq-core/NetworkUtilities/TCPReceiverSocket.h"

namespace ots
{

// A class that can read/write to a socket
class TCPTransceiverSocket: public TCPReceiverSocket, public TCPTransmitterSocket
{
public:
	TCPTransceiverSocket(int socketId=invalidSocketId);
	virtual ~TCPTransceiverSocket(void);
	TCPTransceiverSocket(TCPTransceiverSocket const&)  = delete ;
	TCPTransceiverSocket(TCPTransceiverSocket&& theTCPTransceiverSocket) = default;

	std::string sendAndReceivePacket(const std::string& sendBuffer);
	std::string sendAndReceive      (const std::string& sendBuffer);

};
}
#endif
