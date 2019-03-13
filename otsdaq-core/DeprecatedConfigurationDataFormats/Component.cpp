/*
 * Component.cpp
 *
 *  Created on: Jul 29, 2015
 *      Author: parilla
 */

#include "otsdaq-core/ConfigurationDataFormats/Component.h"

#include <iostream>

using namespace ots;

Component::Component(std::string name, std::string typeName)
    : componentName_(name), typeName_(typeName)
{
}

Component::~Component()
{
	// TODO Auto-generated destructor stub
}
//==============================================================================
void Component::addRegister(std::string name)
{
	Register tempRegister_(name);
	registers_.push_back(tempRegister_);
}
//==============================================================================
void Component::addRegister(std::string name,
                            std::string baseAddress,
                            int         size,
                            std::string access,
                            int         globalSequencePosition,
                            int         globalValue)
{
	Register tempRegister_(name);
	tempRegister_.fillRegisterInfo(baseAddress, size, access);
	tempRegister_.setInitialize(std::make_pair(globalSequencePosition, globalValue));
	tempRegister_.setConfigure(std::make_pair(globalSequencePosition, globalValue));
	tempRegister_.setStart(std::make_pair(globalSequencePosition, globalValue));
	tempRegister_.setHalt(std::make_pair(globalSequencePosition, globalValue));
	tempRegister_.setPause(std::make_pair(globalSequencePosition, globalValue));
	tempRegister_.setResume(std::make_pair(globalSequencePosition, globalValue));
	registers_.push_back(tempRegister_);
}
//==============================================================================
void Component::addRegister(std::string name,
                            std::string baseAddress,
                            int         size,
                            std::string access,
                            int         initializeSequencePosition,
                            int         initializeValue,
                            int         configureSequencePosition,
                            int         configureValue)
{
	Register tempRegister_(name);
	tempRegister_.fillRegisterInfo(baseAddress, size, access);
	tempRegister_.setInitialize(
	    std::make_pair(initializeSequencePosition, initializeValue));
	tempRegister_.setConfigure(std::make_pair(configureSequencePosition, configureValue));
	tempRegister_.setStart(std::make_pair(-1, 0));
	tempRegister_.setHalt(std::make_pair(-1, 0));
	tempRegister_.setPause(std::make_pair(-1, 0));
	tempRegister_.setResume(std::make_pair(-1, 0));
	registers_.push_back(tempRegister_);
}
//==============================================================================
void Component::addRegister(std::string name,
                            std::string baseAddress,
                            int         size,
                            std::string access,
                            int         initializeSequencePosition,
                            int         initializeValue,
                            int         configureSequencePosition,
                            int         configureValue,
                            int         startSequencePosition,
                            int         startValue,
                            int         haltSequencePosition,
                            int         haltValue,
                            int         pauseSequencePosition,
                            int         pauseValue,
                            int         resumeSequencePosition,
                            int         resumeValue)
{
	Register tempRegister_(name);
	tempRegister_.fillRegisterInfo(baseAddress, size, access);
	tempRegister_.setInitialize(
	    std::make_pair(initializeSequencePosition, initializeValue));
	tempRegister_.setConfigure(std::make_pair(configureSequencePosition, configureValue));
	tempRegister_.setStart(std::make_pair(startSequencePosition, startValue));
	tempRegister_.setHalt(std::make_pair(haltSequencePosition, haltValue));
	tempRegister_.setPause(std::make_pair(pauseSequencePosition, pauseValue));
	tempRegister_.setResume(std::make_pair(resumeSequencePosition, resumeValue));
	registers_.push_back(tempRegister_);
}

// Getters
//==============================================================================
std::list<Register> Component::getRegisters(void) { return registers_; }
//==============================================================================
std::list<Register>* Component::getRegistersPointer(void) { return &registers_; }
//==============================================================================
std::string Component::getComponentName(void) { return componentName_; }
//==============================================================================
std::string Component::getTypeName(void) { return typeName_; }
//==============================================================================
std::string Component::printPair(std::pair<int, int> in)
{
	return "(" + std::to_string(in.first) + "," + std::to_string(in.second) + ")";
}

void Component::printInfo(void)
{
	std::cout << __COUT_HDR_FL__
	          << "======================================================================="
	             "======="
	          << std::endl;
	std::cout << __COUT_HDR_FL__ << "===============================" << componentName_
	          << std::endl;
	std::cout << __COUT_HDR_FL__
	          << "======================================================================="
	             "======="
	          << std::endl;
	std::cout << __COUT_HDR_FL__
	          << "REGISTER \t\t BASE ADDRESS \t\t SIZE \t\t ACCESS \t\t||\t INIT \t "
	             "CONFIG \t START \t STOP \t PAUSE \t RESUME "
	          << std::endl;

	for(std::list<Register>::iterator aRegister = registers_.begin();
	    aRegister != registers_.end();
	    aRegister++)
	{
		std::cout << __COUT_HDR_FL__ << aRegister->getName() << "\t\t"
		          << aRegister->getBaseAddress() << "\t\t"
		          << std::to_string(aRegister->getSize()) << "\t\t"
		          << aRegister->getAccess() << "\t\t||\t"
		          << printPair(aRegister->getInitialize()) << "/t"
		          << printPair(aRegister->getStart()) << "\t"
		          << printPair(aRegister->getHalt()) << "\t"
		          << printPair(aRegister->getPause()) << "\t"
		          << printPair(aRegister->getResume()) << std::endl;
	}
}
