#include "otsdaq/NetworkUtilities/TCPReceiverSocket.h"
#include <sys/socket.h>
#include <unistd.h>  //read
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace ots;
//========================================================================================================================
TCPReceiverSocket::TCPReceiverSocket(int socketId) : TCPSocket(socketId)
{
	fPacket.reset();
}

//========================================================================================================================
TCPReceiverSocket::~TCPReceiverSocket(void) {}

//========================================================================================================================
std::string TCPReceiverSocket::receivePacket(void)
{
	while(true)
	{
		std::string retVal = "";
		if(fPacket.decode(retVal))
			return retVal;
		fPacket += receive<std::string>();
	}
}

//========================================================================================================================
std::size_t TCPReceiverSocket::receive(char* buffer, std::size_t bufferSize)
{
	// std::cout << __PRETTY_FUNCTION__ << "Receiving Message for socket: " <<
	// getSocketId() << std::endl;
	if(getSocketId() == 0)
	{
		throw std::logic_error("Bad socket object (this object was moved)");
	}
	std::size_t dataRead = ::read(getSocketId(), buffer, bufferSize);
	if(dataRead == static_cast<std::size_t>(-1))
	{
		std::stringstream error;
		switch(errno)
		{
		case EBADF:
			error
			    << "Socket file descriptor " << getSocketId()
			    << " is not a valid file descriptor or is not open for reading...Errno: "
			    << errno;
			break;
		case EFAULT:
			error << "Buffer is outside your accessible address space...Errno: " << errno;
			break;
		case ENXIO:
		{
			// Fatal error. Programming bug
			error << "Read critical error caused by a programming bug...Errno: " << errno;
			throw std::domain_error(error.str());
		}
		case EINTR:
			// TODO: Check for user interrupt flags.
			//       Beyond the scope of this project
			//       so continue normal operations.
			error << "The call was interrupted by a signal before any data was "
			         "read...Errno: "
			      << errno;
			break;
		case EAGAIN:
		{
			// Temporary error.
			// Simply retry the read.
			std::cout << __PRETTY_FUNCTION__
			          << "Read returned EAGAIN error which means that the read was "
			             "temporarily broken. If this continues it is a problem because "
			             "I am in a recursive loop!"
			          << std::endl;
			dataRead = receive(buffer, bufferSize);
			return dataRead;
		}
		case ENOTCONN:
		{
			// Connection broken.
			// Return the data we have available and exit
			// as if the connection was closed correctly.
			return dataRead;
		}
		default:
		{
			error << "Read: returned -1...Errno: " << errno;
		}
		}
		throw std::runtime_error(error.str());
	}
	// std::cout << __PRETTY_FUNCTION__ << "Message received with no errors for socket: "
	// << getSocketId() << std::endl;
	return dataRead;
}
