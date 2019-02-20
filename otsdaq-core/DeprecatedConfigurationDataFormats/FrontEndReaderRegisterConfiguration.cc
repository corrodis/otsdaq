#include "otsdaq-core/ConfigurationDataFormats/FERRegisterTable.h"

#include <iostream>

using namespace ots;

const std::string FERRegisterConfiguration::staticTableName_ =
    "FERRegisterConfiguration";
//==============================================================================
FERRegisterConfiguration::FERRegisterConfiguration()
    : RegisterConfiguration(FERRegisterConfiguration::staticTableName_)
{
}
//==============================================================================
FERRegisterConfiguration::~FERRegisterConfiguration(void) {}
