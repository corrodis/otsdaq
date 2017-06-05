#include "otsdaq-core/NetworkUtilities/UDPDataStreamerBase.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

#include <iostream>

using namespace ots;

//NOTE: if you want to inherit from this one you MUST initialize
//the Socket Constructor that is commented out here in your class
//========================================================================================================================
UDPDataStreamerBase::UDPDataStreamerBase(std::string IPAddress, unsigned int port, std::string toIPAddress, unsigned int toPort)
: TransmitterSocket (IPAddress, port)
, streamToSocket_	(toIPAddress, toPort)
{
	std::cout << __COUT_HDR_FL__ << "IPAddress " << IPAddress << std::endl;
	std::cout << __COUT_HDR_FL__ << "port " << port << std::endl;
	std::cout << __COUT_HDR_FL__ << "toIPAddress " << toIPAddress << std::endl;
	std::cout << __COUT_HDR_FL__ << "toPort " << toPort << std::endl;
	std::cout << __COUT_HDR_FL__ << std::endl;
	std::cout << __COUT_HDR_FL__ << std::endl;
	std::cout << __COUT_HDR_FL__ << std::endl;
	std::cout << __COUT_HDR_FL__ << std::endl;
	std::cout << __COUT_HDR_FL__ << std::endl;

	Socket::initialize();
	std::cout << __COUT_HDR_FL__ << "done!" << std::endl;
}

//========================================================================================================================
UDPDataStreamerBase::~UDPDataStreamerBase(void)
{
}

