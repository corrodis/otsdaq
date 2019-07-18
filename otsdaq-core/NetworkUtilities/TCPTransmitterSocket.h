#ifndef _ots_TCPTransmitterSocket_h_
#define _ots_TCPTransmitterSocket_h_

#include "otsdaq-core/NetworkUtilities/TCPSocket.h"
#include <string>
#include <vector>

namespace ots
{

// A class that can write to a socket
class TCPTransmitterSocket : public virtual TCPSocket
{
public:
	TCPTransmitterSocket(int socketId = invalidSocketId);
	virtual ~TCPTransmitterSocket(void);
	//TCPTransmitterSocket(TCPTransmitterSocket const&)  = delete ;
	TCPTransmitterSocket(TCPTransmitterSocket &&theTCPTransmitterSocket) = default;

	void send(char const*              buffer, std::size_t size);
	void send(const std::string&       buffer);
	void send(const std::vector<char>& buffer);
	template <typename T>

	void send(const std::vector<T> &buffer)
	{
		send(reinterpret_cast<const char *>(&buffer.at(0)), buffer.size() * sizeof(T));
	}

	void sendPacket(const std::string& buffer);
};
}
#endif
