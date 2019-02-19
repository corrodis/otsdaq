#include "otsdaq-core/ConfigurationDataFormats/DetectorRegisterSequencer.h"

#include <iostream>

using namespace ots;

const std::string DetectorRegisterSequencer::staticConfigurationName_ =
    "DetectorRegisterSequencer";
//==============================================================================
DetectorRegisterSequencer::DetectorRegisterSequencer()
    : RegisterConfiguration(DetectorRegisterSequencer::staticConfigurationName_)
{
}
//==============================================================================
DetectorRegisterSequencer::~DetectorRegisterSequencer(void) {}
