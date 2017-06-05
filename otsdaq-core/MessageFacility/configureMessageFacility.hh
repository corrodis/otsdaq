#ifndef otsdaq_core_MessageFacility_configureMessageFacility_hh
#define otsdaq_core_MessageFacility_configureMessageFacility_hh

#include <string>

namespace ots
{
  static bool messagefacility_initialized = false;

  // Configure and start the message facility. Provide the program
  // name so that messages will be appropriately tagged.
  void configureMessageFacility(char const* progname);

  // Set the message facility application name using the specified
  // application type 
  void setMsgFacAppName(const std::string& appType);
}

#endif /* otsdaq_core_MessageFacility_configureMessageFacility_hh */
