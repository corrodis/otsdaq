#include "otsdaq-core/NetworkUtilities/TransmitterSocket.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
TransmitterSocket::TransmitterSocket(void)
{
	__COUT__ << std::endl;
}

//========================================================================================================================
TransmitterSocket::TransmitterSocket(const std::string &IPAddress, unsigned int port)
	: Socket(IPAddress, port)
{
	__COUT__ << std::endl;
}

//========================================================================================================================
TransmitterSocket::~TransmitterSocket(void)
{}

//========================================================================================================================
int TransmitterSocket::send(Socket& toSocket, const std::string& buffer)
{

	//lockout other senders for the remainder of the scope
	std::lock_guard<std::mutex> lock(sendMutex_);

	//	__COUT__ << "Socket Descriptor #: " << socketNumber_ <<
	//			" from-port: " << ntohs(socketAddress_.sin_port) <<
	//			" to-port: " << ntohs(toSocket.getSocketAddress().sin_port) << std::endl;

	constexpr size_t MAX_SEND_SIZE = 1500;
	size_t offset = 0;
	int sts = 1;

	while (offset < buffer.size() && sts > 0)
	{
		auto thisSize = buffer.size() - offset > MAX_SEND_SIZE ? MAX_SEND_SIZE : buffer.size() - offset;
		sts = sendto(socketNumber_, buffer.c_str() + offset, thisSize, 0, (struct sockaddr *)&(toSocket.getSocketAddress()), sizeof(sockaddr_in));
		offset += sts;
	}

	if (sts <= 0)
	{
		__COUT__ << "Error writing buffer for port " << ntohs(socketAddress_.sin_port) << ": " << strerror(errno) << std::endl;
		return -1;
	}
	return 0;
}

//========================================================================================================================
int TransmitterSocket::send(Socket& toSocket, const std::vector<uint16_t>& buffer)
{

	//lockout other senders for the remainder of the scope
	std::lock_guard<std::mutex> lock(sendMutex_);

	//	__COUT__ << "Socket Descriptor #: " << socketNumber_ <<
	//			" from-port: " << ntohs(socketAddress_.sin_port) <<
	//			" to-port: " << ntohs(toSocket.getSocketAddress().sin_port) << std::endl;

	constexpr size_t MAX_SEND_SIZE = 1500;
	size_t offset = 0;
	int sts = 1;

	while (offset < buffer.size() && sts > 0)
	{
		auto thisSize = 2 * (buffer.size() - offset) > MAX_SEND_SIZE ? MAX_SEND_SIZE : 2 * (buffer.size() - offset);
		sts = sendto(socketNumber_, &buffer[0] + offset, thisSize, 0, (struct sockaddr *)&(toSocket.getSocketAddress()), sizeof(sockaddr_in));
		offset += sts / 2;
	}

	if (sts <= 0)
	{
		__COUT__ << "Error writing buffer for port " << ntohs(socketAddress_.sin_port) << ": " << strerror(errno) << std::endl;
		return -1;
	}
	return 0;
}

//========================================================================================================================
int TransmitterSocket::send(Socket& toSocket, const std::vector<uint32_t>& buffer)
{
	//lockout other senders for the remainder of the scope
	std::lock_guard<std::mutex> lock(sendMutex_);

	//	__COUT__ << "Socket Descriptor #: " << socketNumber_ <<
	//			" from-port: " << ntohs(socketAddress_.sin_port) <<
	//			" to-port: " << ntohs(toSocket.getSocketAddress().sin_port) << std::endl;

	if (sendto(socketNumber_, &buffer[0], buffer.size() * sizeof(uint32_t), 0, (struct sockaddr *)&(toSocket.getSocketAddress()), sizeof(sockaddr_in)) < (int)(buffer.size() * sizeof(uint32_t)))
	{
		__COUT__ << "Error writing buffer for port " << ntohs(socketAddress_.sin_port) << std::endl;
		return -1;
	}
	return 0;

}
