#include "otsdaq-core/NetworkUtilities/TransceiverSocket.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
TransceiverSocket::TransceiverSocket(void)
{
	//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << std::endl;
}

//========================================================================================================================
TransceiverSocket::TransceiverSocket(std::string IPAddress, unsigned int port) :
        Socket (IPAddress, port)
{
	//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << std::endl;
}

//========================================================================================================================
TransceiverSocket::~TransceiverSocket(void)
{}
