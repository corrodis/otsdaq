#include "otsdaq-core/FECore/FESlowControlsWorkLoop.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include "otsdaq-core/FECore/FEVInterface.h"

#include <iostream>
#include <sstream>

using namespace ots;

bool FESlowControlsWorkLoop::workLoopThread(toolbox::task::WorkLoop* workLoop) {
  return false;
  //	return interface_->slowControlsRunning();
}
