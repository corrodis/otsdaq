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
#include <sys/socket.h>

// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netdb.h>


using namespace ots;

//==============================================================================
TCPServerBase::TCPServerBase(int serverPort, unsigned int maxNumberOfClients)
    : fMaxNumberOfClients(maxNumberOfClients)
	, fAccept(true)
{
	int opt = 1;  // SO_REUSEADDR - man socket(7)
	if(::setsockopt(getSocketId(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1)
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
    //freeaddrinfo(serverAddr); // all done with this structure

	if(::listen(getSocketId(), fMaxConnectionBacklog) != 0)
	{
		close();
		throw std::runtime_error(std::string("Listen: ") + strerror(errno));
	}
	//CANNOT GO IN THE CONSTRUCTOR OR IT MIGHT START BEFORE THE CHILD CLASS CONSTRUCTOR IS FULLY CONSTRUCTED 
	//THIS MIGHT RESULT IN THE CALL OF THE VIRTUAL TCPServerBase::acceptConnections
	//startAccept();
}

//==============================================================================
TCPServerBase::~TCPServerBase(void)
{
	std::cout << __PRETTY_FUNCTION__ << "Shutting down accept for socket: " << getSocketId() << std::endl;
	shutdownAccept();
	while(fAcceptFuture.valid() && fAcceptFuture.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready)
	{
	 	std::cout << __PRETTY_FUNCTION__ << "Server accept still running" << std::endl;
		shutdownAccept();
	}
	//std::cout << __PRETTY_FUNCTION__ << "Closing connected client sockets for socket: " << getSocketId() << std::endl;
	closeClientSockets();
	//std::cout << __PRETTY_FUNCTION__ << "Closed all sockets connected to server: " << getSocketId() << std::endl;
}

//==============================================================================
void TCPServerBase::startAccept(void)
{
	std::cout << __PRETTY_FUNCTION__ << "Begin startAccept" << std::endl;
	fAccept = true;
	fAcceptFuture = std::async(std::launch::async, &TCPServerBase::acceptConnections, this);
	//std::thread thread = std::thread(&TCPServerBase::acceptConnections, this);
	//thread.detach();
	std::cout << __PRETTY_FUNCTION__ << "Done startAccept" << std::endl;
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

    struct sockaddr_storage clientAddress; // connector's address information
    socklen_t clientAddressSize = sizeof(clientAddress);
	int clientSocket = invalidSocketId;
	if (blocking)
	{
		//std::cout << __PRETTY_FUNCTION__ << "Number of connected clients: " << fConnectedClients.size() << std::endl;
		//clientSocket = ::accept4(getSocketId(),(struct sockaddr *)&clientAddress,  &clientAddressSize, 0);
		while(true)
		{
			clientSocket = ::accept(getSocketId(), (struct sockaddr *)&clientAddress, &clientAddressSize);
			if(fAccept && fMaxNumberOfClients> 0 && fConnectedClients.size() >= fMaxNumberOfClients)
			{
				send(clientSocket, "Too many clients connected!", 27, 0);
				::shutdown(clientSocket, SHUT_WR);
				continue;
			}
			break;
		}
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
//This method is called in the distructor so I need to wait for the threads to be done!
void TCPServerBase::closeClientSockets(void)
{
	for(auto& socket : fConnectedClients)
	{
		try
		{
			socket.second->sendClose();
		}
		catch(const std::exception& e)
		{
			//I can get here with the TCPPubishServer because it doesn't keep track of the clients that might have already disconnected
			//Just do nothing!
			std::cout << __PRETTY_FUNCTION__ << e.what() << '\n';
		}
		
		auto clientThread = fConnectedClientsFuture.find(socket.first);
		if(clientThread != fConnectedClientsFuture.end())
			clientThread->second.wait();//Waiting for client thread
		delete socket.second;
	}
	fConnectedClients.clear();
	fConnectedClientsFuture.clear();
}

//==============================================================================
void TCPServerBase::closeClientSocket(int socket)
{
	//This method is called inside the thread itself so it cannot call the removeClientSocketFuture!!!
	auto it = fConnectedClients.find(socket);
	if(it != fConnectedClients.end())
	{
		if(it->second->getSocketId() == socket)
		{
			try
			{
				it->second->sendClose();
			}
			catch(const std::exception& e)
			{
				//I can get here with the TCPPubishServer because it doesn't keep track of the clients that might have already disconnected
				//Just do nothing!
				std::cout << __PRETTY_FUNCTION__ << e.what() << '\n';
			}
			delete it->second;
			fConnectedClients.erase(it);
		}
		else
		{
			throw std::runtime_error(std::string("SocketId in fConnectedClients != socketId in TCPSocket! Impossible!!!"));
		}
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
			std::cout << __PRETTY_FUNCTION__ << "I don't think that this error is possible because I close the socket when I get disconnected...if you see this then you should contact Lorenzo Uplegger" << std::endl;
			std::cout << __PRETTY_FUNCTION__ << "This should only happen with the TCPSubscribeServer because it doesn't keep track of the connected clients..." << std::endl;
			std::cout << __PRETTY_FUNCTION__ << "Error: " << e.what() << std::endl;
			delete it->second;
			fConnectedClientsFuture.erase(fConnectedClientsFuture.find(it->first));
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
			std::cout << __PRETTY_FUNCTION__ << "I don't think that this error is possible because I close the socket when I get disconnected...if you see this then you should contact Lorenzo Uplegger" << std::endl;
			std::cout << __PRETTY_FUNCTION__ << "This should only happen with the TCPSubscribeServer because it doesn't keep track of the connected clients..." << std::endl;
			std::cout << __PRETTY_FUNCTION__ << "Error: " << e.what() << std::endl;
			fConnectedClientsFuture.erase(fConnectedClientsFuture.find(it->first));
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
			std::cout << __PRETTY_FUNCTION__ << "I don't think that this error is possible because I close the socket when I get disconnected...if you see this then you should contact Lorenzo Uplegger" << std::endl;
			std::cout << __PRETTY_FUNCTION__ << "This should only happen with the TCPSubscribeServer because it doesn't keep track of the connected clients..." << std::endl;
			std::cout << __PRETTY_FUNCTION__ << "Error: " << e.what() << std::endl;
			fConnectedClientsFuture.erase(fConnectedClientsFuture.find(it->first));
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
			std::cout << __PRETTY_FUNCTION__ << "I don't think that this error is possible because I close the socket when I get disconnected...if you see this then you should contact Lorenzo Uplegger" << std::endl;
			std::cout << __PRETTY_FUNCTION__ << "This should only happen with the TCPSubscribeServer because it doesn't keep track of the connected clients..." << std::endl;
			std::cout << __PRETTY_FUNCTION__ << "Error: " << e.what() << std::endl;
			fConnectedClientsFuture.erase(fConnectedClientsFuture.find(it->first));
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
			std::cout << __PRETTY_FUNCTION__ << "This should only happen with the TCPSubscribeServer because it doesn't keep track of the connected clients..." << std::endl;
			std::cout << __PRETTY_FUNCTION__ << "Error: " << e.what() << std::endl;
			fConnectedClientsFuture.erase(fConnectedClientsFuture.find(it->first));
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
