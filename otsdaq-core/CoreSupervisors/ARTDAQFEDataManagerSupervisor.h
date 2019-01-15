#ifndef _ots_ARTDAQFEDataManagerSupervisor_h_
#define _ots_ARTDAQFEDataManagerSupervisor_h_

#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

namespace ots
{

class ARTDAQFEDataManagerSupervisor: public CoreSupervisorBase
{

public:

    XDAQ_INSTANTIATOR();

    ARTDAQFEDataManagerSupervisor              (xdaq::ApplicationStub * s) ;
    virtual ~ARTDAQFEDataManagerSupervisor     (void);

private:
};

}

#endif
