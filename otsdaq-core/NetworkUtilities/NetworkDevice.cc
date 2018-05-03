#include "otsdaq-core/NetworkUtilities/NetworkDevice.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include <iostream>
#include <sstream>

#include <assert.h>

#include <unistd.h>
#include <arpa/inet.h>
//#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#if defined(SIOCGIFHWADDR)
#include <net/if.h>
#else
#include <net/if_dl.h>
#endif
#include <cstdlib>
#include <cstring>
#include <cstdio>

using namespace ots;



//========================================================================================================================
NetworkDevice::NetworkDevice(std::string IPAddress, unsigned int IPPort) : communicationInterface_(NULL)
{
    //network stuff
    deviceAddress_.sin_family = AF_INET;// use IPv4 host byte order
    deviceAddress_.sin_port   = htons(IPPort);// short, network byte order

    if(inet_aton(IPAddress.c_str(), &deviceAddress_.sin_addr) == 0)
    {
        std::cout << __COUT_HDR_FL__ << __LINE__ << "FATAL: Invalid IP address " <<  IPAddress << std::endl;
        //FIXME substitute with try catch solution !!
        assert(0);
    }

    memset(&(deviceAddress_.sin_zero), '\0', 8);// zero the rest of the struct
}

//========================================================================================================================
NetworkDevice::~NetworkDevice(void)
{
  for(std::map<int,int>::iterator it=openSockets_.begin(); it!=openSockets_.end(); it++)
    {
        std::cout << __COUT_HDR_FL__ << "Closing socket for port " << it->second << std::endl;
        close(it->first);
    }
    openSockets_.clear();
}

//========================================================================================================================
int NetworkDevice::initSocket(std::string socketPort)
{

    struct addrinfo  hints;

    struct addrinfo* res;
    int status    =  0;
    int socketOut = -1;

    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;   // use IPv4 for OtsUDPHardware
    hints.ai_socktype = SOCK_DGRAM;// SOCK_DGRAM
    hints.ai_flags    = AI_PASSIVE;// fill in my IP for me

    bool socketInitialized = false;
    int fromPort = FirstSocketPort;
    int toPort   = LastSocketPort;

    if(socketPort != "")
        fromPort = toPort = strtol(socketPort.c_str(),0,10);

    std::stringstream port;

    for(int p=fromPort; p<=toPort && !socketInitialized; p++)
    {
        port.str("");
        port << p;
        std::cout << __COUT_HDR_FL__ << __LINE__ << "]\tBinding port: " << port.str() << std::endl;

        if((status = getaddrinfo(NULL, port.str().c_str(), &hints, &res)) != 0)
        {
            std::cout << __COUT_HDR_FL__ << __LINE__ << "]\tGetaddrinfo error status: " << status << std::endl;
            continue;
        }

        // make a socket:
        socketOut = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

        // bind it to the port we passed in to getaddrinfo():
        if(bind(socketOut, res->ai_addr, res->ai_addrlen) == -1)
        {
	  mf::LogError("NetworkDevice") << __LINE__ << "]\tFailed bind for port: " <<  port.str();
            socketOut = -1;
            exit(0);
        }
        else
        {
            char yes = '1';
            setsockopt(socketOut,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
            socketInitialized = true;
            openSockets_[socketOut] = p;
            std::cout << __COUT_HDR_FL__ << __LINE__ << "]\tSocket Number: " << socketOut << " for port: " << p << " initialized." << std::endl;
        }

        freeaddrinfo(res); // free the linked-list
    }

    return socketOut;
}

//========================================================================================================================
int NetworkDevice::initSocket(unsigned int socketPort)
{
    std::stringstream socket("");
    if(socketPort != 0)
        socket << socketPort;
    return initSocket(socket.str());
}

//========================================================================================================================
int NetworkDevice::send(int socketDescriptor, const std::string& buffer)
{
    if(sendto(socketDescriptor,buffer.c_str(),buffer.size(),0,(struct sockaddr *)&(deviceAddress_), sizeof(deviceAddress_)) < (int)(buffer.size()))
    {
        std::cout << __COUT_HDR_FL__ << "Error writing buffer" << std::endl;
        return -1;
    }
    return 0;
}

//========================================================================================================================
int NetworkDevice::receive(int socketDescriptor, std::string& buffer)
{
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0; //set timeout period for select()
    fd_set fileDescriptor;  //setup set for select()
    FD_ZERO(&fileDescriptor);
    FD_SET(socketDescriptor,&fileDescriptor);
    select(socketDescriptor+1, &fileDescriptor, 0, 0, &tv);

    if(FD_ISSET(socketDescriptor,&fileDescriptor))
    {
        std::string bufferS;
        struct sockaddr_in tmpAddress;
        socklen_t addressLength = sizeof(tmpAddress);
        int nOfBytes;
        char   readBuffer[maxSocketSize];
        if ((nOfBytes=recvfrom(socketDescriptor, readBuffer, maxSocketSize, 0, (struct sockaddr *)&tmpAddress, &addressLength)) == -1)
        {
            std::cout << __COUT_HDR_FL__ << __LINE__ << "Error reading buffer" << std::endl;
            return -1;
        }
        buffer.resize(nOfBytes);
        for(int i=0; i<nOfBytes; i++)
            buffer[i] = readBuffer[i];
    }
    else
    {
        std::cout << __COUT_HDR_FL__ << __LINE__ << "]\tNetwork device unresponsive. Read request timed out." << std::endl;
        return -1;
    }

    return 0;
}

//========================================================================================================================
int NetworkDevice::listen(int socketDescriptor, std::string& buffer)
{
    std::cout << __COUT_HDR_FL__ << "LISTENING!!!!!" << std::endl;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0; //set timeout period for select()
    fd_set fileDescriptor;  //setup set for select()
    FD_ZERO(&fileDescriptor);
    FD_SET(socketDescriptor,&fileDescriptor);
    select(socketDescriptor+1, &fileDescriptor, 0, 0, &tv);

    if(FD_ISSET(socketDescriptor,&fileDescriptor))
    {
        std::string bufferS;
        struct sockaddr_in tmpAddress;
        socklen_t addressLength = sizeof(tmpAddress);
        int nOfBytes;
        char   readBuffer[maxSocketSize];
        if ((nOfBytes=recvfrom(socketDescriptor, readBuffer, maxSocketSize, 0, (struct sockaddr *)&tmpAddress, &addressLength)) == -1)
        {
            std::cout << __COUT_HDR_FL__ << __LINE__ << "Error reading buffer" << std::endl;
            return -1;
        }
        buffer.resize(nOfBytes);
        for(int i=0; i<nOfBytes; i++)
            buffer[i] = readBuffer[i];
    }
    else
    {
        std::cout << __COUT_HDR_FL__ << __LINE__ << "Timed out" << std::endl;
        return -1;
    }

    return 0;
}

//========================================================================================================================
int NetworkDevice::ping(int socketDescriptor)
{
    std::string pingMsg(1,0);
    if(send(socketDescriptor, pingMsg) == -1)
    {
        std::cout << __COUT_HDR_FL__ << __LINE__ << "]\tCan't send ping Message!" << std::endl;
        return -1;
    }

    std::string bufferS;
    if(receive(socketDescriptor,bufferS) == -1)
    {
        std::cout << __COUT_HDR_FL__ << __LINE__ <<"]\tFailed to ping device"<< std::endl;
        return -1;
    }
    /*
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; //set timeout period for select()
        fd_set fileDescriptor;  //setup set for select()
        FD_ZERO(&fileDescriptor);
        FD_SET(socketDescriptor,&fileDescriptor);
        select(socketDescriptor+1, &fileDescriptor, 0, 0, &tv);

        if(FD_ISSET(socketDescriptor,&fileDescriptor))
        {
        std::string bufferS;
            if(receive(socketDescriptor,bufferS) == -1)
            {
               std::cout << __COUT_HDR_FL__ << __LINE__ <<"]\tFailed to ping device"<< std::endl;
               return -1;
            }
        }
        else
        {
            std::cout << __COUT_HDR_FL__ << __LINE__ << "]\tNetwork device unresponsive. Ping request timed out." << std::endl;
            return -1;
        }
    */
    return 0;
}

//========================================================================================================================
std::string NetworkDevice::getFullIPAddress(std::string partialIpAddress)
{
    std::cout << __COUT_HDR_FL__ << "partial IP: " << partialIpAddress << std::endl;
    if(getInterface(partialIpAddress))
    {
        char *p, addr[32];
        p=inet_ntoa(((struct sockaddr_in *)(communicationInterface_->ifa_addr))->sin_addr);
        strncpy(addr,p,sizeof(addr)-1);
        addr[sizeof(addr)-1]='\0';

        return addr;
    }
    else
    {
        std::cout << __COUT_HDR_FL__ << "FIXME substitute with try catch solution !!\n\nFailed to locate network interface matching partial IP address" << std::endl;
        //FIXME substitute with try catch solution !!
        assert(0);

        //std::cout << __COUT_HDR_FL__ << "Failed to locate network interface matching partial IP address" << std::endl;
        return "";
    }
}

//========================================================================================================================
std::string NetworkDevice::getInterfaceName(std::string ipAddress)
{
    std::cout << __COUT_HDR_FL__ << "IP: " << ipAddress << std::endl;
    if(getInterface(ipAddress))
        return communicationInterface_->ifa_name;
    else
    {
        //FIXME substitute with try catch solution !!
        std::cout << __COUT_HDR_FL__ << "FIXME substitute with try catch solution !!\n\nFailed to get interface name!" << std::endl;
        assert(0);

        std::cout << __COUT_HDR_FL__ << "Failed to get interface name for IP address" << std::endl;
        return "";
    }
}

//========================================================================================================================
int NetworkDevice::getInterface(std::string ipAddress)
{
    int family, s;
    char host[NI_MAXHOST];


    std::cout << __COUT_HDR_FL__ << "IP2: " << ipAddress << std::endl;
    if(communicationInterface_ != 0)
    {
        s = getnameinfo(communicationInterface_->ifa_addr,

                        (communicationInterface_->ifa_addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) :

                        sizeof(struct sockaddr_in6),
                        host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if (s != 0)
        {
            std::cout << __COUT_HDR_FL__ << "getnameinfo() failed: " << gai_strerror(s) << std::endl;
            //FIXME substitute with try catch solution !!
            std::cout << __COUT_HDR_FL__ << "FIXME substitute with try catch solution !!\n\nFailed to get name info!" << std::endl;
            assert(0);
        }

        if (std::string(host).find(ipAddress) != std::string::npos)
            return true;
        else
        {
            delete communicationInterface_;
            communicationInterface_ = NULL;
        }
    }

    struct ifaddrs* ifaddr;

    struct ifaddrs* ifa;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        //FIXME substitute with try catch solution !!
        std::cout << __COUT_HDR_FL__ << "FIXME substitute with try catch solution !!\n\nFailed to get ifaddress!" << std::endl;
        assert(0);
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    bool found = false;

    for (ifa = ifaddr; ifa != NULL && !found; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        /* For an AF_INET* interface address, display the address */

        if (family == AF_INET || family == AF_INET6)
        {
            s = getnameinfo(ifa->ifa_addr,

                            (family == AF_INET) ? sizeof(struct sockaddr_in) :

                            sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

            if (s != 0)
            {
                std::cout << __COUT_HDR_FL__ << "getnameinfo() failed: " << gai_strerror(s) << std::endl;
                //FIXME substitute with try catch solution !!
                std::cout << __COUT_HDR_FL__ << "FIXME substitute with try catch solution !!\n\nFailed to get getnameinfo!" << std::endl;
                assert(0);
            }

            if (std::string(host).find(ipAddress) != std::string::npos)
            {
                communicationInterface_ = new struct ifaddrs(*ifa);
                found = true;
            }
        }
    }

    freeifaddrs(ifaddr);

    return found;
}

//========================================================================================================================
std::string NetworkDevice::getMacAddress(std::string interfaceName)
{

    std::string macAddress(6,'0');
#if defined(SIOCGIFHWADDR)

    struct ifreq ifr;
    int sock;

    sock=socket(PF_INET, SOCK_STREAM, 0);

    if (-1==sock)
    {
        perror("socket() ");
        return "";
    }

    strncpy(ifr.ifr_name,interfaceName.c_str(),sizeof(ifr.ifr_name)-1);
    ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';

    if (-1==ioctl(sock, SIOCGIFHWADDR, &ifr))
    {
        perror("ioctl(SIOCGIFHWADDR) ");
        return "";
    }
    for (int j=0; j<6; j++)
        macAddress[j] = ifr.ifr_hwaddr.sa_data[j];
#else

    char mac[6];
    ifaddrs* iflist;
    bool found = false;
    if (getifaddrs(&iflist) == 0)
    {
        for (ifaddrs* cur = iflist; cur; cur = cur->ifa_next)
        {
            if ((cur->ifa_addr->sa_family == AF_LINK) &&
                    (strcmp(cur->ifa_name, interfaceName.c_str()) == 0) &&
                    cur->ifa_addr)
            {
                sockaddr_dl* sdl = (sockaddr_dl*)cur->ifa_addr;
                memcpy(mac, LLADDR(sdl), sdl->sdl_alen);
                found = true;
                break;
            }
        }

        freeifaddrs(iflist);
    }
    for (int j=0; j<6; j++)
        macAddress[j] = mac[j];
#endif

    return macAddress;
}
