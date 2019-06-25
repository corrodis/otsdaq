#ifndef _ots_Configurations_h_
#define _ots_Configurations_h_

#include <set>
#include <string>

#include "otsdaq-coreTableCore/TableBase.h"
#include "otsdaq-coreTableCore/TableGroupKey.h"
#include "otsdaq-coreTableCore/TableVersion.h"

namespace ots
{
class Configurations : public TableBase
{
  public:
	Configurations(void);
	virtual ~Configurations(void);

	// Methods
	void init(ConfigurationManager* configManager);
	bool findKOC(TableGroupKey TableGroupKey, std::string koc) const;

	// Getters
	TableVersion getConditionVersion(const TableGroupKey& TableGroupKey,
	                                 std::string          koc) const;

	std::set<std::string> getListOfKocs(
	    TableGroupKey TableGroupKey = TableGroupKey()) const;  // INVALID to get all Kocs
	void getListOfKocsForView(
	    TableView*             cfgView,
	    std::set<std::string>& kocList,
	    TableGroupKey TableGroupKey = TableGroupKey()) const;  // INVALID to get all Kocs

	// Setters
	int setConditionVersionForView(TableView*    cfgView,
	                               TableGroupKey TableGroupKey,
	                               std::string   koc,
	                               TableVersion  newKOCVersion);

  private:
	enum
	{
		TableGroupKeyAlias,
		KOC,
		ConditionVersion
	};
};
}  // namespace ots
#endif
