#ifndef _ots_ARTDAQDataManager_h_
#define _ots_ARTDAQDataManager_h_

#include "otsdaq-core/DataManager/DataManager.h"
#include "otsdaq-core/MPICore/MPIInitializer.h"

namespace ots
{

class ConfigurationManager;

class ARTDAQDataManager : public DataManager
{
public:
    ARTDAQDataManager(const ConfigurationTree& theXDAQContextConfigTree, const std::string& supervisorConfigurationPath);
    virtual ~ARTDAQDataManager(void);
    void configure(void);
    void stop(void);

private:
    MPIInitializer theMPIProcess_;
};

}

#endif
