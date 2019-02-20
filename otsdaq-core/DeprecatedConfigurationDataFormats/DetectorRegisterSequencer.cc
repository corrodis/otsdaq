#include "otsdaq-core/ConfigurationDataFormats/DetectorRegisterSequencer.h"

#include <iostream>

using namespace ots;

const std::string DetectorRegisterSequencer::staticTableName_ =
    "DetectorRegisterSequencer";
//==============================================================================
DetectorRegisterSequencer::DetectorRegisterSequencer()
    : RegisterConfiguration(DetectorRegisterSequencer::staticTableName_)
{
}
//==============================================================================
DetectorRegisterSequencer::~DetectorRegisterSequencer(void) {}
