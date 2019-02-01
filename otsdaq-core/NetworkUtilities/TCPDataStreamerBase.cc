#include "otsdaq-core/NetworkUtilities/TCPDataStreamerBase.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include <iostream>

using namespace ots;

//NOTE: if you want to inherit from this one you MUST initialize
//the Socket Constructor that is commented out here in your class
//========================================================================================================================
TCPDataStreamerBase::TCPDataStreamerBase(unsigned int port)
: TCPSocket (port)
{
	std::cout << __COUT_HDR_FL__ << "port " << port << std::endl;
	std::cout << __COUT_HDR_FL__ << std::endl;
	std::cout << __COUT_HDR_FL__ << std::endl;
	std::cout << __COUT_HDR_FL__ << std::endl;
	std::cout << __COUT_HDR_FL__ << std::endl;
	std::cout << __COUT_HDR_FL__ << std::endl;

	std::cout << __COUT_HDR_FL__ << "done!" << std::endl;
}

//========================================================================================================================
TCPDataStreamerBase::~TCPDataStreamerBase(void)
{
}

