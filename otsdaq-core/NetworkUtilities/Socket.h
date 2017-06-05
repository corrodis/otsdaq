#ifndef _ots_Socket_h_
#define _ots_Socket_h_

#include <sys/types.h>
#include <netinet/in.h>
#include <string>

namespace ots
{

class Socket
{
public:
	Socket(const std::string &IPAddress, unsigned int port=0);
    virtual ~Socket(void);

    virtual void initialize(void);
    const struct sockaddr_in& getSocketAddress(void);
    const std::string& getIPAddress() { return IPAddress_; }
    const unsigned int& getPort() { return port_; }

protected:
	enum{maxSocketSize_ = 65536};

	Socket(void);
    struct sockaddr_in socketAddress_;
    int                socketNumber_;
    //unsigned int maxSocketSize_;

private:
    enum{FirstSocketPort=10000,LastSocketPort=15000};
    std::string  IPAddress_;
    unsigned int port_;
};

}

#endif
