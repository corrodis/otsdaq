#include "artdaq-ots/Overlays/FragmentType.hh"
#include "artdaq-core/Data/FragmentNameHelper.hh"

#include "otsdaq/Macros/CoutMacros.h"
#define TRACE_NAME "OtsFragmentNameHelper"

namespace ots
{
/**
 * \brief OtsFragmentNameHelper extends ArtdaqFragmentNamingService.
 * This implementation uses artdaq-demo's SystemTypeMap and directly assigns names based on it
 */
class OtsFragmentNameHelper : public artdaq::ArtdaqFragmentNameHelper
{
  public:
	/**
	 * \brief DefaultArtdaqFragmentNamingService Destructor
	 */
	~OtsFragmentNameHelper() override = default;

	/**
	 * \brief OtsFragmentNameHelper Constructor
	 */
	OtsFragmentNameHelper(std::string unidentified_instance_name, std::vector<std::pair<artdaq::Fragment::type_t, std::string>> extraTypes);

  private:
	OtsFragmentNameHelper(OtsFragmentNameHelper const&) = delete;
	OtsFragmentNameHelper(OtsFragmentNameHelper&&)      = delete;
	OtsFragmentNameHelper& operator=(OtsFragmentNameHelper const&) = delete;
	OtsFragmentNameHelper& operator=(OtsFragmentNameHelper&&) = delete;
};

OtsFragmentNameHelper::OtsFragmentNameHelper(std::string                                                   unidentified_instance_name,
                                                           std::vector<std::pair<artdaq::Fragment::type_t, std::string>> extraTypes)
    : ArtdaqFragmentNameHelper(unidentified_instance_name, extraTypes)
{
	TLOG(TLVL_DEBUG) << "OtsFragmentNameHelper CONSTRUCTOR START";
	SetBasicTypes(ots::makeFragmentTypeMap());
	TLOG(TLVL_DEBUG) << "OtsFragmentNameHelper CONSTRUCTOR END";
}
}  // namespace artdaq

DEFINE_ARTDAQ_FRAGMENT_NAME_HELPER(ots::OtsFragmentNameHelper)