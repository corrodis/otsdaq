#ifndef _TCPPublishServer_h_
#define _TCPPublishServer_h_

#include "otsdaq-core/NetworkUtilities/TCPServerBase.h"

namespace ots
{

class TCPPublishServer : public TCPServerBase
{
public:
	TCPPublishServer(int serverPort, unsigned int maxNumberOfClients=-1);
	virtual ~TCPPublishServer(void);

protected:
	void acceptConnections() override;

};
}
#endif
