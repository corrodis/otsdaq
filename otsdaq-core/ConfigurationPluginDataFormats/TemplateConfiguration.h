#ifndef _ots_TemplateConfiguration_h_
#define _ots_TemplateConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

namespace ots {

class TemplateConfiguration : public ConfigurationBase {
       public:
	TemplateConfiguration(void);
	virtual ~TemplateConfiguration(void);

	//Methods
	void init(ConfigurationManager *configManager);

	//Getters

       private:
	//Column names
	struct ColTemplate
	{
		std::string const colColumnName_ = "ColumnName";
	} colNames_;
};
}  // namespace ots
#endif
