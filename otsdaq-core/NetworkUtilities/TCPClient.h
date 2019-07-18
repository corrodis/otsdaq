#ifndef _ots_TCPClient_h_
#define _ots_TCPClient_h_

#include "otsdaq-core/NetworkUtilities/TCPTransceiverSocket.h"
#include "otsdaq-core/NetworkUtilities/TCPClientBase.h"
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
