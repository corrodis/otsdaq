#ifndef _ots_TCPClient_h_
#define _ots_TCPClient_h_

#include "otsdaq/NetworkUtilities/TCPClientBase.h"
#include "otsdaq/NetworkUtilities/TCPTransceiverSocket.h"
#include <string>

namespace ots
{
class TCPClient : public TCPTransceiverSocket, public TCPClientBase
{
  public:
	TCPClient(const std::string& serverIP, int serverPort);
	virtual ~TCPClient(void);
};
}

#endif
