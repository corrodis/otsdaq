//#ifndef BEAGLEBONE
//#include "otsdaq_cmsburninbox/BeagleBone/BeagleBoneUtils/TCPServerBase.h"
//#else
#include "otsdaq/NetworkUtilities/TCPServerBase.h"
#include "otsdaq/NetworkUtilities/TCPTransmitterSocket.h"
//#endif

#include <arpa/inet.h>
#include <errno.h>   // errno
#include <string.h>  // errno
#include <iostream>

using namespace ots;

//==============================================================================
TCPServerBase::TCPServerBase(int serverPort, unsigned int /*maxNumberOfClients*/)
    : /*fMaxNumberOfClients(maxNumberOfClients),*/ fAccept(true)//, fAcceptFuture(fAcceptPromise.get_future())
{
	int opt = 1;  // SO_REUSEADDR - man socket(7)
	if(::setsockopt(getSocketId(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0)
	{
		close();
		throw std::runtime_error(std::string("Setsockopt: ") + strerror(errno));
	}

	struct sockaddr_in serverAddr;
	bzero((char*)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family      = AF_INET;
	serverAddr.sin_port        = htons(serverPort);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if(::bind(getSocketId(), (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
	{
		close();
		throw std::runtime_error(std::string("Bind: ") + strerror(errno));
	}

	if(::listen(getSocketId(), fMaxConnectionBacklog) != 0)
	{
		close();
		throw std::runtime_error(std::string("Listen: ") + strerror(errno));
	}
	startAccept();
}

//==============================================================================
TCPServerBase::~TCPServerBase(void)
{
	std::cout << __PRETTY_FUNCTION__ << "Shutting down accept for socket: " << getSocketId() << std::endl;
	shutdownAccept();
	while(fAcceptFuture.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready)
		std::cout << __PRETTY_FUNCTION__ << "Server accept still running" << std::endl;
	std::cout << __PRETTY_FUNCTION__ << "Closing connected client sockets for socket: " << getSocketId() << std::endl;
	closeClientSockets();
	std::cout << __PRETTY_FUNCTION__ << "Closed all sockets connected to server: " << getSocketId() << std::endl;
}

//==============================================================================
void TCPServerBase::startAccept(void)
{
	fAccept = true;
	std::thread thread(&TCPServerBase::acceptConnections, this);
	thread.detach();
}

// An accepts waits for a connection and returns the opened socket number
//==============================================================================
int TCPServerBase::accept(bool blocking)
{
	std::cout << __PRETTY_FUNCTION__ << "Now server accept connections on socket: " << getSocketId() << std::endl;
	if (getSocketId() == invalidSocketId)
	{
		throw std::logic_error("Accept called on a bad socket object (this object was moved)");
	}

	struct sockaddr_storage serverStorage;
	socklen_t addr_size = sizeof serverStorage;
	int clientSocket = invalidSocketId;
	if (blocking)
	{
		clientSocket = ::accept(getSocketId(), (struct sockaddr *)&serverStorage, &addr_size);
		std::cout << __LINE__<< "] " << __PRETTY_FUNCTION__ << "fAccept? " << fAccept << std::endl;
		if (!fAccept)
		{
			throw E_SHUTDOWN;
		}
		if (clientSocket == invalidSocketId)
		{
			std::cout << __PRETTY_FUNCTION__ << "New socket invalid?: " << clientSocket << " errno: " << errno << std::endl;
			throw std::runtime_error(std::string("Accept: ") + strerror(errno));
		}
		std::cout << __PRETTY_FUNCTION__ << "Server just accepted a connection on socket: " << getSocketId() << " Client socket: " << clientSocket << std::endl;
		return clientSocket;
	}
	else
	{
		constexpr int sleepMSeconds = 5;
		constexpr int timeoutSeconds = 0;
		constexpr int timeoutUSeconds = 1000;
		struct timeval timeout;
		timeout.tv_sec = timeoutSeconds;
		timeout.tv_usec = timeoutUSeconds;

		fd_set fdSet;

		while (fAccept)
		{
			FD_ZERO(&fdSet);
			FD_SET(getSocketId(), &fdSet);
			select(getSocketId() + 1, &fdSet, 0, 0, &timeout);

			if (FD_ISSET(getSocketId(), &fdSet))
			{
				struct sockaddr_in clientAddress;
				socklen_t socketSize = sizeof(clientAddress);
				//int newSocketFD = ::accept4(fdServerSocket_,(struct sockaddr*)&clientAddress,&socketSize, (pushOnly_ ? SOCK_NONBLOCK : 0));
				clientSocket = ::accept(getSocketId(), (struct sockaddr *)&clientAddress, &socketSize); //Blocking since select goes in timeout if there is nothing
				if (clientSocket == invalidSocketId)
				{
					std::cout << __PRETTY_FUNCTION__ << "New socket invalid?: " << clientSocket << " errno: " << errno << std::endl;
					throw std::runtime_error(std::string("Accept: ") + strerror(errno));
				}
				return clientSocket;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(sleepMSeconds));
		}
		throw E_SHUTDOWN;
	}
}

//==============================================================================
void TCPServerBase::closeClientSockets(void)
{
	for(auto& socket : fConnectedClients)
	{
		socket.second->sendClose();
		delete socket.second;
	}
	fConnectedClients.clear();
}

//==============================================================================
void TCPServerBase::closeClientSocket(int socket)
{
	for(auto it = fConnectedClients.begin(); it != fConnectedClients.end(); it++)
		if(it->second->getSocketId() == socket)
		{
			it->second->sendClose();
			delete it->second;
			fConnectedClients.erase(it--);
		}
}

//==============================================================================
void TCPServerBase::broadcastPacket(const char* message, std::size_t length)
{
	broadcastPacket(std::string(message,length));
}

//==============================================================================
void TCPServerBase::broadcastPacket(const std::string& message)
{
	for(auto it = fConnectedClients.begin(); it != fConnectedClients.end(); it++)
	{
		try
		{
			dynamic_cast<TCPTransmitterSocket*>(it->second)->sendPacket(message);
		}
		catch(const std::exception& e)
		{
			std::cout << __PRETTY_FUNCTION__ << "Error: " << e.what() << std::endl;
			delete it->second;
			fConnectedClients.erase(it--);
		}
	}
}

//========================================================================================================================
void TCPServerBase::broadcast(const char* message, std::size_t length)
{
	//	std::lock_guard<std::mutex> lock(clientsMutex_);
	for (auto it = fConnectedClients.begin(); it != fConnectedClients.end(); it++)
	{
		try
		{
			dynamic_cast<TCPTransmitterSocket *>(it->second)->send(message, length);
		}
		catch (const std::exception &e)
		{
			//std::cout << __PRETTY_FUNCTION__ << "Connection closed with the server! Stop writing!" << std::endl;
			delete it->second;
			fConnectedClients.erase(it--);
		}
	}
}

//==============================================================================
void TCPServerBase::broadcast(const std::string& message)
{
	for(auto it = fConnectedClients.begin(); it != fConnectedClients.end(); it++)
	{
		try
		{
			dynamic_cast<TCPTransmitterSocket*>(it->second)->send(message);
		}
		catch(const std::exception& e)
		{
			std::cout << __PRETTY_FUNCTION__ << "Error: " << e.what() << std::endl;
			delete it->second;
			fConnectedClients.erase(it--);
		}
	}
}

//==============================================================================
void TCPServerBase::broadcast(const std::vector<char>& message)
{
	for(auto it = fConnectedClients.begin(); it != fConnectedClients.end(); it++)
	{
		try
		{
			dynamic_cast<TCPTransmitterSocket*>(it->second)->send(message);
		}
		catch(const std::exception& e)
		{
			std::cout << __PRETTY_FUNCTION__ << "Error: " << e.what() << std::endl;
			delete it->second;
			fConnectedClients.erase(it--);
		}
	}
}

//==============================================================================
void TCPServerBase::broadcast(const std::vector<uint16_t>& message)
{
	for(auto it = fConnectedClients.begin(); it != fConnectedClients.end(); it++)
	{
		try
		{
			dynamic_cast<TCPTransmitterSocket*>(it->second)->send(message);
		}
		catch(const std::exception& e)
		{
			std::cout << __PRETTY_FUNCTION__ << "Error: " << e.what() << std::endl;
			delete it->second;
			fConnectedClients.erase(it--);
		}
	}
}

//==============================================================================
void TCPServerBase::shutdownAccept()
{
	fAccept = false;
	shutdown(getSocketId(), SHUT_RD);
}
