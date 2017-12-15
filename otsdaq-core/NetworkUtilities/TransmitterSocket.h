#ifndef _ots_TransmitterSocket_h_
#define _ots_TransmitterSocket_h_

#include "otsdaq-core/NetworkUtilities/Socket.h"

#include <string>
#include <vector>
#include <mutex>        //for std::mutex

namespace ots
{

class TransmitterSocket : public virtual Socket
{
public:
	TransmitterSocket(const std::string &IPAddress, unsigned int port=0);
	virtual ~TransmitterSocket(void);

	int send(Socket& toSocket, const std::string& buffer, bool verbose=false);
	int send(Socket& toSocket, const std::vector<uint32_t>& buffer, bool verbose=false);
	int send(Socket& toSocket, const std::vector<uint16_t>& buffer, bool verbose=false);

protected:
	TransmitterSocket(void);

private:
	std::mutex	sendMutex_; //to make transmitter socket thread safe
							//	i.e. multiple threads can share a socket and call send()

};

}

#endif
