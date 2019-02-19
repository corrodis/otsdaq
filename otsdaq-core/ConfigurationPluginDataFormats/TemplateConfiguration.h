#ifndef _ots_TemplateConfiguration_h_
#define _ots_TemplateConfiguration_h_

#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
class TemplateConfiguration : public TableBase
{
  public:
	TemplateConfiguration(void);
	virtual ~TemplateConfiguration(void);

	// Methods
	void init(ConfigurationManager* configManager);

	// Getters

  private:
	// Column names
	struct ColTemplate
	{
		std::string const colColumnName_ = "ColumnName";
	} colNames_;
};
}  // namespace ots
#endif
