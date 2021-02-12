#ifndef _TCPListenServer_h_
#define _TCPListenServer_h_

#include "otsdaq/NetworkUtilities/TCPServerBase.h"
#include "otsdaq/NetworkUtilities/TCPReceiverSocket.h"

namespace ots
{
class TCPListenServer : public TCPServerBase
{
  public:
	TCPListenServer(unsigned int serverPort, unsigned int maxNumberOfClients = -1);
	virtual ~TCPListenServer(void);

	template<class T>
	T           receive();
	std::string receivePacket();

  protected:
	void acceptConnections() override;
	int  lastReceived;
};
template<class T>
inline T TCPListenServer::receive()
{
	if(!fConnectedClients.empty())
	{
		auto it = fConnectedClients.find(lastReceived);
		if(it == fConnectedClients.end() || ++it == fConnectedClients.end())
			it = fConnectedClients.begin();
		lastReceived = it->first;
		return reinterpret_cast<TCPReceiverSocket*>(it->second)->receive<T>();
	}
	throw std::runtime_error("No clients connected!");
}
}  // namespace ots
#endif
