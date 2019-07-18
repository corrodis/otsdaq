#ifndef _ots_TCPServer_h_
#define _ots_TCPServer_h_

#include "otsdaq-core/NetworkUtilities/TCPServerBase.h"
#include <string>

namespace ots
{

class TCPTransceiverSocket;

class TCPServer : public TCPServerBase
{
public:
	TCPServer(int serverPort, unsigned int maxNumberOfClients=-1);
	virtual ~TCPServer(void);

	virtual std::string  interpretMessage(const std::string& buffer) = 0;
private:
	void acceptConnections(void) override;
	void connectClient    (TCPTransceiverSocket* clientSocket);
};

}

#endif
