#include "otsdaq-core/ConfigurationDataFormats/FERRegisterConfiguration.h"

#include <iostream>

using namespace ots;

const std::string FERRegisterConfiguration::staticConfigurationName_ = "FERRegisterConfiguration";
//==============================================================================
FERRegisterConfiguration::FERRegisterConfiguration()
    : RegisterConfiguration(FERRegisterConfiguration::staticConfigurationName_) {}
//==============================================================================
FERRegisterConfiguration::~FERRegisterConfiguration(void) {}
