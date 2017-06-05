#include "otsdaq-core/NetworkUtilities/TransmitterSocket.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
TransmitterSocket::TransmitterSocket(void)
{
	__MOUT__ << std::endl;
}

//========================================================================================================================
TransmitterSocket::TransmitterSocket(const std::string &IPAddress, unsigned int port)
: Socket(IPAddress, port)
{
	__MOUT__ << std::endl;
}

//========================================================================================================================
TransmitterSocket::~TransmitterSocket(void)
{
}

//========================================================================================================================
int TransmitterSocket::send(Socket& toSocket, const std::string& buffer)
{

	//lockout other senders for the remainder of the scope
	std::lock_guard<std::mutex> lock(sendMutex_);

//	__MOUT__ << "Socket Descriptor #: " << socketNumber_ <<
//			" from-port: " << ntohs(socketAddress_.sin_port) <<
//			" to-port: " << ntohs(toSocket.getSocketAddress().sin_port) << std::endl;

	if(sendto(socketNumber_, buffer.c_str(), buffer.size(), 0,
			(struct sockaddr *)&(toSocket.getSocketAddress()),
			sizeof(sockaddr_in)) < (int)(buffer.size()))
	{
		__MOUT__ << "Error writing buffer for port " << ntohs(socketAddress_.sin_port) << std::endl;
		return -1;
	}
	return 0;
}

//========================================================================================================================
int TransmitterSocket::send(Socket& toSocket, const std::vector<uint32_t>& buffer)
{
	//lockout other senders for the remainder of the scope
	std::lock_guard<std::mutex> lock(sendMutex_);

//	__MOUT__ << "Socket Descriptor #: " << socketNumber_ <<
//			" from-port: " << ntohs(socketAddress_.sin_port) <<
//			" to-port: " << ntohs(toSocket.getSocketAddress().sin_port) << std::endl;

	if(sendto(socketNumber_, &buffer[0], buffer.size()*sizeof(uint32_t), 0, (struct sockaddr *)&(toSocket.getSocketAddress()), sizeof(sockaddr_in)) < (int)(buffer.size()*sizeof(uint32_t)))
	{
		__MOUT__ << "Error writing buffer for port " << ntohs(socketAddress_.sin_port) << std::endl;
		return -1;
	}
	return 0;

}
