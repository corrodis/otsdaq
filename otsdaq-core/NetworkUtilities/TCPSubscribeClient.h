#ifndef _TCPSubscribeClient_h_
#define _TCPSubscribeClient_h_

#include "otsdaq-core/NetworkUtilities/TCPTransceiverSocket.h"
#include "otsdaq-core/NetworkUtilities/TCPClientBase.h"
#include <string>

namespace ots
{

class TCPSubscribeClient : public TCPReceiverSocket, public TCPClientBase
{
public:
	//TCPSubscribeClient();
	TCPSubscribeClient(const std::string& serverIP, int serverPort);
	virtual ~TCPSubscribeClient(void);
};
}
#endif
