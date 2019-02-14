#include "otsdaq-core/ConfigurationDataFormats/FEWRegisterConfiguration.h"

#include <iostream>

using namespace ots;

const std::string FEWRegisterConfiguration::staticConfigurationName_ = "FEWRegisterConfiguration";
//==============================================================================
FEWRegisterConfiguration::FEWRegisterConfiguration() :
		RegisterConfiguration(FEWRegisterConfiguration::staticConfigurationName_)
{}
//==============================================================================
FEWRegisterConfiguration::~FEWRegisterConfiguration(void)
{}
