/*
 * Register.cpp
 *
 *  Created on: Jul 29, 2015
 *      Author: parilla
 */

#include "otsdaq-core/ConfigurationDataFormats/Register.h"

using namespace ots;

Register::Register(std::string name) : registerName_(name) {}

Register::~Register() {}
//==============================================================================
void Register::setState(std::string state, std::pair<int, int> valueSequencePair)
{
	if(state == "INITIALIZE")
	{
		initialize_ = valueSequencePair;
	}
	else if(state == "CONFIGURATION")
	{
		configuration_ = valueSequencePair;
	}
	else if(state == "START")
	{
		start_ = valueSequencePair;
	}
	else if(state == "HALT")
	{
		halt_ = valueSequencePair;
	}
	else if(state == "PAUSE")
	{
		pause_ = valueSequencePair;
	}
	else if(state == "RESUME")
	{
		resume_ = valueSequencePair;
	}

	return;
}
//==============================================================================
void Register::fillRegisterInfo(std::string registerBaseAddress,
                                int         registerSize,
                                std::string registerAccess)
{
	registerBaseAddress_ = registerBaseAddress;
	registerSize_        = registerSize;
	registerAccess_      = registerAccess;

	return;
}
//==============================================================================
void Register::setInitialize(std::pair<int, int> initialize)
{
	initialize_ = initialize;

	return;
}
//==============================================================================
void Register::setConfigure(std::pair<int, int> configure)
{
	configuration_ = configure;

	return;
}
//==============================================================================
void Register::setStart(std::pair<int, int> start)
{
	start_ = start;

	return;
}
//==============================================================================
void Register::setHalt(std::pair<int, int> halt)
{
	halt_ = halt;

	return;
}
//==============================================================================
void Register::setPause(std::pair<int, int> pause)
{
	pause_ = pause;

	return;
}
//==============================================================================
void Register::setResume(std::pair<int, int> resume) { resume_ = resume; }

// Getters
//==============================================================================
std::string Register::getName(void) { return registerName_; }
//==============================================================================
std::string Register::getBaseAddress(void) { return registerBaseAddress_; }
//==============================================================================
int Register::getSize(void) { return registerSize_; }
//==============================================================================
std::string Register::getAccess(void) { return registerAccess_; }
//==============================================================================
std::pair<int, int> Register::getInitialize(void) { return initialize_; }
//==============================================================================
std::pair<int, int> Register::getConfigure(void) { return configuration_; }
//==============================================================================
std::pair<int, int> Register::getStart(void) { return start_; }
//==============================================================================
std::pair<int, int> Register::getHalt(void) { return halt_; }
//==============================================================================
std::pair<int, int> Register::getPause(void) { return pause_; }
//==============================================================================
std::pair<int, int> Register::getResume(void) { return resume_; }
