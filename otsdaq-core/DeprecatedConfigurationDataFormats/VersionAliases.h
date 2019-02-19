#ifndef _ots_VersionAliases_h_
#define _ots_VersionAliases_h_

#include <string>

#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
class VersionAliases : public TableBase
{
  public:
	VersionAliases(void);
	~VersionAliases(void);

	// Methods
	void init(ConfigurationManager* configManager);

	// Getters
	unsigned int getAliasedKey(std::string alias) const;

  private:
	enum
	{
		VersionAlias,
		Version,
		KOC
	};
};
}  // namespace ots
#endif
