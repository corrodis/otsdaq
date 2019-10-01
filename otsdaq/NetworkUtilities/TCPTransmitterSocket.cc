#include "otsdaq/NetworkUtilities/TCPTransmitterSocket.h"
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include "otsdaq/NetworkUtilities/TCPPacket.h"
//#include <iostream>

using namespace ots;

//========================================================================================================================
TCPTransmitterSocket::TCPTransmitterSocket(int socketId) : TCPSocket(socketId) {}

//========================================================================================================================
TCPTransmitterSocket::~TCPTransmitterSocket(void) {}

//========================================================================================================================
void TCPTransmitterSocket::sendPacket(const std::string& buffer)
{
	send(TCPPacket::encode(buffer));
}

//========================================================================================================================
void TCPTransmitterSocket::send(char const* buffer, std::size_t size)
{
	std::size_t put = write(getSocketId(), buffer, size);
	if(put == static_cast<std::size_t>(-1))
	{
		switch(errno)
		{
		// case EINVAL:
		// case EBADF:
		// case ECONNRESET:
		// case ENXIO:
		case EPIPE:
		{
			// Fatal error. Programming bug
			throw std::domain_error(std::string("Write: critical error: ") +
			                        strerror(errno));
		}
		// case EDQUOT:
		// case EFBIG:
		// case EIO:
		// case ENETDOWN:
		// case ENETUNREACH:
		case ENOSPC:
		{
			// Resource acquisition failure or device error
			throw std::runtime_error(std::string("Write: resource failure: ") +
			                         strerror(errno));
		}
		case EINTR:
			break;
			// TODO: Check for user interrupt flags.
			//       Beyond the scope of this project
			//       so continue normal operations.
		case EAGAIN:
		{
			// Temporary error.
			throw std::runtime_error(std::string("Write: temporary error: ") +
			                         strerror(errno));
		}
		default:
		{
			throw std::runtime_error(std::string("Write: returned -1: ") +
			                         strerror(errno));
		}
		}
	}
}

//========================================================================================================================
void TCPTransmitterSocket::send(const std::string& buffer)
{
	send(&buffer.at(0), buffer.size());
}

//========================================================================================================================
void TCPTransmitterSocket::send(const std::vector<char>& buffer)
{
	send(&buffer.at(0), buffer.size());
}
