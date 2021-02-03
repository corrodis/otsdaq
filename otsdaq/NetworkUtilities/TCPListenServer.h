#ifndef _TCPListenServer_h_
#define _TCPListenServer_h_

#include "otsdaq/NetworkUtilities/TCPServerBase.h"
#include "otsdaq/NetworkUtilities/TCPReceiverSocket.h"

namespace ots
{
class TCPListenServer : public TCPReceiverSocket, public TCPServerBase
{
  public:
	TCPListenServer(unsigned int serverPort, unsigned int maxNumberOfClients = -1);
	virtual ~TCPListenServer(void);


  protected:
	void acceptConnections() override;
};
}
#endif
