#ifndef _ots_FESupervisor_h_
#define _ots_FESupervisor_h_

#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

namespace ots
{

class FESupervisor: public CoreSupervisorBase
{

public:

    XDAQ_INSTANTIATOR();

    FESupervisor                    (xdaq::ApplicationStub * s) throw (xdaq::exception::Exception);
    virtual ~FESupervisor           (void);

};

}

#endif
