#include "otsdaq-core/ConfigurationDataFormats/DetectorRegisterConfiguration.h"

#include <iostream>

using namespace ots;

const std::string DetectorRegisterConfiguration::staticConfigurationName_ = "DetectorRegisterConfiguration";
//==============================================================================
DetectorRegisterConfiguration::DetectorRegisterConfiguration()
    : RegisterConfiguration(DetectorRegisterConfiguration::staticConfigurationName_)
{
}
//==============================================================================
DetectorRegisterConfiguration::~DetectorRegisterConfiguration(void)
{
}
