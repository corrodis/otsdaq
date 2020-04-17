#ifndef _ots_TCPTransmitterSocket_h_
#define _ots_TCPTransmitterSocket_h_

#include <iostream>
#include <string>
#include <vector>
#include "otsdaq/NetworkUtilities/TCPSocket.h"

namespace ots
{
// A class that can write to a socket
class TCPTransmitterSocket : public virtual TCPSocket
{
  public:
	TCPTransmitterSocket(int socketId = invalidSocketId);
	virtual ~TCPTransmitterSocket(void);
	// TCPTransmitterSocket(TCPTransmitterSocket const&)  = delete ;
	TCPTransmitterSocket(TCPTransmitterSocket&& theTCPTransmitterSocket) = default;

	void send(char const* buffer, std::size_t size);
	void send(const std::string& buffer);
	void send(const std::vector<char>& buffer);
	void send(const std::vector<uint16_t>& buffer);

	template<typename T>
	void send(const std::vector<T>& buffer)
	{
		if(buffer.size() == 0)
		{
			std::cout << __PRETTY_FUNCTION__ << "I am sorry but I won't send an empty packet!" << std::endl;
			return;
		}
		send(reinterpret_cast<const char*>(&buffer.at(0)), buffer.size() * sizeof(T));
	}

	void sendPacket(char const* buffer, std::size_t size);
	void sendPacket(const std::string& buffer);
	void setSendTimeout(unsigned int timeoutSeconds, unsigned int timeoutMicroSeconds);
};
}  // namespace ots
#endif
