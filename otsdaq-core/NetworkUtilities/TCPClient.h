#ifndef _ots_TCPClient_h_
#define _ots_TCPClient_h_

#include <string>
#include "otsdaq-core/NetworkUtilities/TCPClientBase.h"
#include "otsdaq-core/NetworkUtilities/TCPTransceiverSocket.h"

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
