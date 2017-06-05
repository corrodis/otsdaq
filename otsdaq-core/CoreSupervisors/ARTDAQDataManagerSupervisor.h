#ifndef _ots_ARTDAQDataManagerSupervisor_h_
#define _ots_ARTDAQDataManagerSupervisor_h_

#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

namespace ots
{

class ARTDAQDataManagerSupervisor: public CoreSupervisorBase
{

public:

    XDAQ_INSTANTIATOR();

    ARTDAQDataManagerSupervisor              (xdaq::ApplicationStub * s) throw (xdaq::exception::Exception);
    virtual ~ARTDAQDataManagerSupervisor     (void);

private:
};

}

#endif
