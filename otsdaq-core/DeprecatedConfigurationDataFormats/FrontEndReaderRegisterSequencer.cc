#include "otsdaq-core/ConfigurationDataFormats/FERRegisterSequencer.h"

#include <iostream>

using namespace ots;

const std::string FERRegisterSequencer::staticConfigurationName_ = "FERRegisterSequencer";
//==============================================================================
FERRegisterSequencer::FERRegisterSequencer ()
    : RegisterConfiguration (FERRegisterSequencer::staticConfigurationName_)
{
}
//==============================================================================
FERRegisterSequencer::~FERRegisterSequencer (void)
{
}
