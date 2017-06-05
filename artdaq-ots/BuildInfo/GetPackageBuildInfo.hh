#ifndef artdaq_ots_BuildInfo_GetPackageBuildInfo_hh
#define artdaq_ots_BuildInfo_GetPackageBuildInfo_hh

#include "artdaq-core/Data/PackageBuildInfo.hh"

#include <string>

namespace ots {

  struct GetPackageBuildInfo {

    static artdaq::PackageBuildInfo getPackageBuildInfo();
  };

}

#endif /* artdaq_ots_BuildInfo_GetPackageBuildInfo_hh */
