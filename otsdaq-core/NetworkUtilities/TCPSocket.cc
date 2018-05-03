#include "otsdaq-core/NetworkUtilities/TCPSocket.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include <iostream>
#include <cassert>
#include <sstream>

#include <unistd.h>
#include <arpa/inet.h>
//#include <sys/TCPSocket.h>
#include <netdb.h>
//#include <ifaddrs.h>
//#include <sys/ioctl.h>
//#if defined(SIOCGIFHWADDR)
//#include <net/if.h>
//#else
//#include <net/if_dl.h>
//#endif
//#include <cstdlib>
#include <cstring>
//#include <cstdio>
#include "trace.h"

using namespace ots;

//========================================================================================================================
TCPSocket::TCPSocket(const std::string &senderHost, unsigned int senderPort, int receiveBufferSize)
	: host_(senderHost)
	, port_(senderPort)
	, TCPSocketNumber_(-1)
	, SendSocket_(-1)
	, isSender_(false)
	, bufferSize_(receiveBufferSize)
	, chunkSize_(65000)
{}

TCPSocket::TCPSocket(unsigned int listenPort, int sendBufferSize)
	: port_(listenPort)
	, TCPSocketNumber_(-1)
	, SendSocket_(-1)
	, isSender_(true)
	, bufferSize_(sendBufferSize)
	, chunkSize_(65000)
{
	TCPSocketNumber_ = TCP_listen_fd(listenPort, 0);
	if (bufferSize_ > 0)
	{
		int len;
		socklen_t lenlen = sizeof(len);
		len = 0;
		auto sts = getsockopt(TCPSocketNumber_, SOL_SOCKET, SO_SNDBUF, &len, &lenlen);
		TRACE(3, "TCPConnect SNDBUF initial: %d sts/errno=%d/%d lenlen=%d", len, sts, errno, lenlen);
		len = bufferSize_;
		sts = setsockopt(TCPSocketNumber_, SOL_SOCKET, SO_SNDBUF, &len, lenlen);
		if (sts == -1)
			TRACE(0, "Error with setsockopt SNDBUF %d", errno);
		len = 0;
		sts = getsockopt(TCPSocketNumber_, SOL_SOCKET, SO_SNDBUF, &len, &lenlen);
		if (len < (bufferSize_ * 2))
			TRACE(1, "SNDBUF %d not expected (%d) sts/errno=%d/%d"
				  , len, bufferSize_, sts, errno);
		else
			TRACE(3, "SNDBUF %d sts/errno=%d/%d", len, sts, errno);
	}
}

//========================================================================================================================
//protected constructor
TCPSocket::TCPSocket(void)
{
	__SS__ << "ERROR: This method should never be called. This is the protected constructor. There is something wrong in your inheritance scheme!" << std::endl;
	__COUT__ << "\n" << ss.str();

	throw std::runtime_error(ss.str());
}

//========================================================================================================================
TCPSocket::~TCPSocket(void)
{
	__COUT__ << "CLOSING THE TCPSocket #" << TCPSocketNumber_ << " IP: " << host_ << " port: " << port_ << std::endl;
	if (TCPSocketNumber_ != -1)
		close(TCPSocketNumber_);
	if (SendSocket_ != -1)
		close(SendSocket_);
}

void TCPSocket::connect()
{
	if (isSender_)
	{
		sockaddr_in addr;
		socklen_t arglen = sizeof(addr);
		SendSocket_ = accept(TCPSocketNumber_, (struct sockaddr *)&addr, &arglen);

		if (SendSocket_ == -1)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}

		MagicPacket m;
		auto sts = read(SendSocket_, &m, sizeof(MagicPacket));
		if (!checkMagicPacket(m) || sts != sizeof(MagicPacket))
		{
			perror("magic");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		TCPSocketNumber_ = TCPConnect(host_.c_str(), port_, 0, O_NONBLOCK);
		auto m = makeMagicPacket(port_);
		auto sts = ::send(TCPSocketNumber_, &m, sizeof(MagicPacket), 0);
		if (sts != sizeof(MagicPacket))
		{
			perror("connect");
			exit(EXIT_FAILURE);
		}
	}
}


int TCPSocket::send(const uint8_t* data, size_t size)
{
	std::unique_lock<std::mutex> lk(socketMutex_);
	if (SendSocket_ == -1)
	{
		connect();
	}

	size_t offset = 0;
	int sts = 1;

	while (offset < size && sts > 0)
	{
		auto thisSize = size - offset > chunkSize_ ? chunkSize_ : size - offset;

		sts = ::send(SendSocket_, data + offset, thisSize, 0);

		// Dynamically resize chunk size to match send calls
		if (static_cast<size_t>(sts) != size - offset && static_cast<size_t>(sts) < chunkSize_)
		{
			chunkSize_ = sts;
		}
		offset += sts;
	}

	if (sts <= 0)
	{
		__COUT__ << "Error writing buffer for port " << port_ << ": " << strerror(errno) << std::endl;
		return -1;
	}
	return 0;
}

//========================================================================================================================
int TCPSocket::send(const std::string& buffer)
{
	return send(reinterpret_cast<const uint8_t*>(buffer.c_str()), buffer.size());
}

//========================================================================================================================
int TCPSocket::send(const std::vector<uint16_t>& buffer)
{
	return send(reinterpret_cast<const uint8_t*>(&buffer[0]), buffer.size() * sizeof(uint16_t));
}

//========================================================================================================================
int TCPSocket::send(const std::vector<uint32_t>& buffer)
{
	return send(reinterpret_cast<const uint8_t*>(&buffer[0]), buffer.size() * sizeof(uint32_t));
}

//========================================================================================================================
//receive ~~
//	returns 0 on success, -1 on failure
//	NOTE: must call Socket::initialize before receiving!
int TCPSocket::receive(uint8_t* buffer, unsigned int timeoutSeconds, unsigned int timeoutUSeconds)
{
	//lockout other receivers for the remainder of the scope
	std::unique_lock<std::mutex> lk(socketMutex_);

	if (TCPSocketNumber_ == -1)
	{
		connect();
	}

	//set timeout period for select()
	struct timeval timeout;
	timeout.tv_sec = timeoutSeconds;
	timeout.tv_usec = timeoutUSeconds;

	fd_set fdSet;
	FD_ZERO(&fdSet);
	FD_SET(TCPSocketNumber_, &fdSet);
	select(TCPSocketNumber_ + 1, &fdSet, 0, 0, &timeout);

	if (FD_ISSET(TCPSocketNumber_, &fdSet))
	{
		auto sts = -1;
		if ((sts = read(TCPSocketNumber_, buffer, chunkSize_)) == -1)
		{
			__COUT__ << "Error reading buffer from: " << host_ << ":" << port_ << std::endl;
			return -1;
		}

		//NOTE: this is inexpensive according to Lorenzo/documentation in C++11 (only increases size once and doesn't decrease size)
		return sts;
	}

	return -1;
}

//========================================================================================================================
//receive ~~
//	returns 0 on success, -1 on failure
//	NOTE: must call Socket::initialize before receiving!
int TCPSocket::receive(std::string& buffer, unsigned int timeoutSeconds, unsigned int timeoutUSeconds)
{
	buffer.resize(chunkSize_);
	auto sts = receive(reinterpret_cast<uint8_t*>(&buffer[0]), timeoutSeconds, timeoutUSeconds);
	if (sts > 0)
	{
		buffer.resize(sts);
		return 0;
	}
	return -1;
}

//========================================================================================================================
//receive ~~
//	returns 0 on success, -1 on failure
//	NOTE: must call Socket::initialize before receiving!
int TCPSocket::receive(std::vector<uint32_t>& buffer, unsigned int timeoutSeconds, unsigned int timeoutUSeconds)
{
	buffer.resize(chunkSize_ / sizeof(uint32_t));
	auto sts = receive(reinterpret_cast<uint8_t*>(&buffer[0]), timeoutSeconds, timeoutUSeconds);
	if (sts > 0)
	{
		buffer.resize(sts);
		return 0;
	}
	return -1;
}

