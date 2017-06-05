#ifndef _ots_FEDataManagerSupervisor_h_
#define _ots_FEDataManagerSupervisor_h_

#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"
#include "otsdaq-core/MPICore/MPIInitializer.h"

namespace ots
{

class FEDataManagerSupervisor: public CoreSupervisorBase
{

public:

    XDAQ_INSTANTIATOR();

    FEDataManagerSupervisor              (xdaq::ApplicationStub * s) throw (xdaq::exception::Exception);
    virtual ~FEDataManagerSupervisor     (void);

private:
};

}

#endif
