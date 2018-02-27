#ifndef _ots_DataManagerSupervisor_h_
#define _ots_DataManagerSupervisor_h_

#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

namespace ots
{

class DataManagerSupervisor: public CoreSupervisorBase
{

public:

    XDAQ_INSTANTIATOR();

    DataManagerSupervisor              (xdaq::ApplicationStub * s) throw (xdaq::exception::Exception);
    virtual ~DataManagerSupervisor     (void);

private:
};

}

#endif
