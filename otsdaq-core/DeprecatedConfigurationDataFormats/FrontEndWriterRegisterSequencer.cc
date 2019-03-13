#include "otsdaq-core/ConfigurationDataFormats/FEWRegisterSequencer.h"

#include <iostream>

using namespace ots;

const std::string FEWRegisterSequencer::staticTableName_ = "FEWRegisterSequencer";
//==============================================================================
FEWRegisterSequencer::FEWRegisterSequencer()
    : RegisterConfiguration(FEWRegisterSequencer::staticTableName_)
{
}
//==============================================================================
FEWRegisterSequencer::~FEWRegisterSequencer(void) {}
