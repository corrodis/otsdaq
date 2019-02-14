#include "otsdaq-core/ConfigurationDataFormats/FEWRegisterSequencer.h"

#include <iostream>

using namespace ots;

const std::string FEWRegisterSequencer::staticConfigurationName_ = "FEWRegisterSequencer";
//==============================================================================
FEWRegisterSequencer::FEWRegisterSequencer ()
    : RegisterConfiguration (FEWRegisterSequencer::staticConfigurationName_)
{
}
//==============================================================================
FEWRegisterSequencer::~FEWRegisterSequencer (void)
{
}
