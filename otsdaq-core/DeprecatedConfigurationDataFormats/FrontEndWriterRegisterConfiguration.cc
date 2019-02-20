#include "otsdaq-core/ConfigurationDataFormats/FEWRegisterTable.h"

#include <iostream>

using namespace ots;

const std::string FEWRegisterConfiguration::staticTableName_ =
    "FEWRegisterConfiguration";
//==============================================================================
FEWRegisterConfiguration::FEWRegisterConfiguration()
    : RegisterConfiguration(FEWRegisterConfiguration::staticTableName_)
{
}
//==============================================================================
FEWRegisterConfiguration::~FEWRegisterConfiguration(void) {}
