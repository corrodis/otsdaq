#include "otsdaq/NetworkUtilities/TCPPublishServer.h"
#include "otsdaq/NetworkUtilities/TCPTransmitterSocket.h"

#include <iostream>

using namespace ots;

//==============================================================================
TCPPublishServer::TCPPublishServer(unsigned int serverPort, unsigned int maxNumberOfClients) : TCPServerBase(serverPort, maxNumberOfClients) {}

//==============================================================================
TCPPublishServer::~TCPPublishServer(void)
{
//	std::cout << __PRETTY_FUNCTION__ << "Done" << std::endl;
}

//==============================================================================
void TCPPublishServer::acceptConnections()
{
	while(true)
	{
		try
		{
			// __attribute__((unused)) TCPTransmitterSocket* clientSocket = acceptClient<TCPTransmitterSocket>();
			acceptClient<TCPTransmitterSocket>();
		}
		catch(int e)
		{
			std::cout << __PRETTY_FUNCTION__ << "SHUTTING DOWN SOCKET" << std::endl;
			std::cout << __PRETTY_FUNCTION__ << "SHUTTING DOWN SOCKET" << std::endl;
			std::cout << __PRETTY_FUNCTION__ << "SHUTTING DOWN SOCKET" << std::endl;

			if(e == E_SHUTDOWN)
				break;
		}
	}
	//fAcceptPromise.set_value(true);
}
