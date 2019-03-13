#include "otsdaq-core/ConfigurationDataFormats/FERRegisterSequencer.h"

#include <iostream>

using namespace ots;

const std::string FERRegisterSequencer::staticTableName_ = "FERRegisterSequencer";
//==============================================================================
FERRegisterSequencer::FERRegisterSequencer()
    : RegisterConfiguration(FERRegisterSequencer::staticTableName_)
{
}
//==============================================================================
FERRegisterSequencer::~FERRegisterSequencer(void) {}
