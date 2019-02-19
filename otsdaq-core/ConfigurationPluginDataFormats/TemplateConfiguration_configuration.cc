#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/TemplateConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>
#include <string>

using namespace ots;

//==============================================================================
TemplateConfiguration::TemplateConfiguration(void) : TableBase("TemplateConfiguration")
{
	////////////////////////////////////////////////////////////////////////////
	// WARNING: the field names used in C++ MUST match the Configuration INFO  //
	////////////////////////////////////////////////////////////////////////////
}

//==============================================================================
TemplateConfiguration::~TemplateConfiguration(void) {}

//==============================================================================
void TemplateConfiguration::init(ConfigurationManager* configManager)
{
	// do something to validate or refactor table
	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
	__COUT__ << configManager->__SELF_NODE__ << std::endl;

	//	__COUT__ << configManager->getNode(this->getTableName()).getValueAsString()
	//		  											  << std::endl;

	std::string                                             value;
	std::vector<std::pair<std::string, ConfigurationTree> > children =
	    configManager->__SELF_NODE__.getChildren();
	for(auto& childPair : children)
	{
		// do something for each row in table
		__COUT__ << childPair.first << std::endl;
		__COUT__ << childPair.second.getNode(colNames_.colColumnName_) << std::endl;
		childPair.second.getNode(colNames_.colColumnName_).getValue(value);
	}
}

DEFINE_OTS_CONFIGURATION(TemplateConfiguration)
