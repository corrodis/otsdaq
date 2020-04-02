#include "artdaq-ots/Overlays/FragmentType.hh"
#include "artdaq/ArtModules/ArtdaqFragmentNamingService.h"

#include "TRACE/tracemf.h"
#define TRACE_NAME "OtsArtdaqFragmentNamingService"

/**
 * \brief OtsArtdaqFragmentNamingService extends ArtdaqFragmentNamingService.
 * This implementation uses artdaq-demo's SystemTypeMap and directly assigns names based on it
 */
class OtsArtdaqFragmentNamingService : public ArtdaqFragmentNamingService
{
  public:
	/**
	 * \brief DefaultArtdaqFragmentNamingService Destructor
	 */
	virtual ~OtsArtdaqFragmentNamingService() = default;

	/**
	 * \brief OtsArtdaqFragmentNamingService Constructor
	 */
	OtsArtdaqFragmentNamingService(fhicl::ParameterSet const&, art::ActivityRegistry&);

  private:
};

OtsArtdaqFragmentNamingService::OtsArtdaqFragmentNamingService(fhicl::ParameterSet const& ps, art::ActivityRegistry& r) : ArtdaqFragmentNamingService(ps, r)
{
	TLOG(TLVL_DEBUG) << "OtsArtdaqFragmentNamingService CONSTRUCTOR START";
	SetBasicTypes(ots::makeFragmentTypeMap());
	TLOG(TLVL_DEBUG) << "OtsArtdaqFragmentNamingService CONSTRUCTOR END";
}

DECLARE_ART_SERVICE_INTERFACE_IMPL(OtsArtdaqFragmentNamingService, ArtdaqFragmentNamingServiceInterface, LEGACY)
DEFINE_ART_SERVICE_INTERFACE_IMPL(OtsArtdaqFragmentNamingService, ArtdaqFragmentNamingServiceInterface)