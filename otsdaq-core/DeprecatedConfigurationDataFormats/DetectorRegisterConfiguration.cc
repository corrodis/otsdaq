#include "otsdaq-core/ConfigurationDataFormats/DetectorRegisterTable.h"

#include <iostream>

using namespace ots;

const std::string DetectorRegisterConfiguration::staticTableName_ =
    "DetectorRegisterConfiguration";
//==============================================================================
DetectorRegisterConfiguration::DetectorRegisterConfiguration()
    : RegisterConfiguration(DetectorRegisterConfiguration::staticTableName_)
{
}
//==============================================================================
DetectorRegisterConfiguration::~DetectorRegisterConfiguration(void) {}
