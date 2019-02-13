/*
 * ViewRegisterInfo.cpp
 *
 *  Created on: Jul 29, 2015
 *      Author: parilla
 *
 *
 *
 */

#include "otsdaq-core/ConfigurationDataFormats/ViewRegisterInfo.h"

using namespace ots;


//==============================================================================
ViewRegisterInfo::ViewRegisterInfo(std::string typeName, std::string registerName, std::string baseAddress, int size, std::string access)
{
		dataTable_.push_back(typeName),
		dataTable_.push_back(registerName),
		dataTable_.push_back(baseAddress),
		  dataTable_.push_back(std::to_string(size)),
		dataTable_.push_back(access);
}

//==============================================================================
ViewRegisterInfo::~ViewRegisterInfo(void)
{
}

//==============================================================================
const std::string& ViewRegisterInfo::getTypeName(void) const
{
	return dataTable_.at(typeName_);
}

//==============================================================================
const std::string& ViewRegisterInfo::getRegisterName(void) const
{
	return dataTable_.at(registerName_);
}
//==============================================================================
const std::string& ViewRegisterInfo::getBaseAddress(void) const
{
	return dataTable_.at(baseAddress_);
}
//==============================================================================
const int ViewRegisterInfo::getSize(void) const
{
  return std::stoi(dataTable_.at(size_));
}
//==============================================================================
const std::string& ViewRegisterInfo::getAccess(void) const
{
	return dataTable_.at(access_);
}
//==============================================================================
const int ViewRegisterInfo::getNumberOfColumns(void) const{
	return dataTable_.size();
}


