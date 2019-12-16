#ifndef _TCPSubscribeClient_h_
#define _TCPSubscribeClient_h_

#include "otsdaq/NetworkUtilities/TCPClientBase.h"
#include "otsdaq/NetworkUtilities/TCPTransceiverSocket.h"
#include <string>

namespace ots
{
class TCPSubscribeClient : public TCPReceiverSocket, public TCPClientBase
{
  public:
	// TCPSubscribeClient();
	TCPSubscribeClient(const std::string& serverIP, int serverPort);
	virtual ~TCPSubscribeClient(void);
};
}
#endif
