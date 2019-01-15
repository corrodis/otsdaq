#ifndef _ots_FEDataManagerSupervisor_h_
#define _ots_FEDataManagerSupervisor_h_

#include "otsdaq-core/CoreSupervisors/FESupervisor.h" //CoreSupervisorBase.h"

namespace ots
{

class FEDataManagerSupervisor: public FESupervisor //CoreSupervisorBase
{

public:

    XDAQ_INSTANTIATOR();

    FEDataManagerSupervisor              (xdaq::ApplicationStub * s) ;
    virtual ~FEDataManagerSupervisor     (void);

private:
};

}

#endif
