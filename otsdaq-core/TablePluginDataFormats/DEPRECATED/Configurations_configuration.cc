#include "otsdaq-core/ConfigurationPluginDataFormats/Configurations.h"
#include <iostream>
#include "../../Macros/TablePluginMacros.h"

using namespace ots;

//==============================================================================
Configurations::Configurations(void) : TableBase("Configurations")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	// ConfigurationsInfo.xml
	//<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	// xsi:noNamespaceSchemaLocation="TableInfo.xsd">
	//  <CONFIGURATION Name="Configurations">
	//    <VIEW Name="CONFIGURATIONS" Type="File,Database,DatabaseTest">
	//      <COLUMN Name="TableGroupKey" StorageName="CONFIGURATION_KEY" DataType="NUMBER"
	//      /> <COLUMN Name="KOC"	            StorageName="KOC"
	//      DataType="VARCHAR2"/> <COLUMN Name="ConditionVersion"
	//      StorageName="CONDITION_VERSION" DataType="NUMBER"  />
	//    </VIEW>
	//  </CONFIGURATION>
	//</ROOT>
}

//==============================================================================
Configurations::~Configurations(void) {}

//==============================================================================
void Configurations::init(ConfigurationManager* configManager)
{
	/*
	    std::string       keyName;
	    unsigned int keyValue;
	    theKeys_.clear();
	    for(unsigned int row=0; row<TableBase::activeTableView_->getNumberOfRows(); row++)
	    {
	        TableBase::activeTableView_->getValue(keyName,row,ConfigurationAlias);
	        TableBase::activeTableView_->getValue(keyValue,row,TableGroupKeyId);
	        theKeys_[keyName] = keyValue;
	    }
	 */
}

//==============================================================================
bool Configurations::findKOC(TableGroupKey TableGroupKey, std::string koc) const
{
	unsigned int tmpTableGroupKey;  // this is type to extract from table
	std::string  tmpKOC;
	for(unsigned int row = 0; row < TableBase::activeTableView_->getNumberOfRows(); row++)
	{
		TableBase::activeTableView_->getValue(tmpTableGroupKey, row, TableGroupKeyAlias);
		if(TableGroupKey == tmpTableGroupKey)
		{
			TableBase::activeTableView_->getValue(tmpKOC, row, KOC);
			// std::cout << __COUT_HDR_FL__ << "Looking for KOC: " << tmpKOC << " for " <<
			// koc << std::endl;
			if(tmpKOC == koc)
				return true;
		}
	}
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "ERROR: Can't find KOC " << koc
	          << std::endl;
	return false;
}

//==============================================================================
// getConditionVersion
// FIXME -- new ConfiguratoinGroup and TableGroupKey should be used!
TableVersion Configurations::getConditionVersion(const TableGroupKey& TableGroupKey,
                                                 std::string          koc) const
{
	unsigned int tmpTableGroupKey;  // this is type to extract from table
	std::string  tmpKOC;
	unsigned int conditionVersion;  // this is type to extract from table

	for(unsigned int row = 0; row < TableBase::activeTableView_->getNumberOfRows(); row++)
	{
		TableBase::activeTableView_->getValue(tmpTableGroupKey, row, TableGroupKeyAlias);
		if(TableGroupKey == tmpTableGroupKey)
		{
			TableBase::activeTableView_->getValue(tmpKOC, row, KOC);
			if(tmpKOC == koc)
			{
				TableBase::activeTableView_->getValue(
				    conditionVersion, row, ConditionVersion);
				// std::cout << __COUT_HDR_FL__ << "\tConditionVersion " <<
				// ConditionVersion << std::endl;
				return TableVersion(conditionVersion);
			}
		}
	}
	std::cout << __COUT_HDR_FL__
	          << "***********************************************************************"
	             "*****************************************************"
	          << std::endl;
	std::cout << __COUT_HDR_FL__ << "\tCan't find KOC " << koc << " with TableGroupKey "
	          << TableGroupKey << " in the Configurations view" << std::endl;
	std::cout << __COUT_HDR_FL__
	          << "***********************************************************************"
	             "*****************************************************"
	          << std::endl;
	__THROW__("Could not find koc for TableGroupKey");
	return TableVersion();  // return INVALID
}

//==============================================================================
// setConditionVersion
// returns 1 if no change occurred (because new version was same as existing)
// returns 0 on change success
int Configurations::setConditionVersionForView(TableView*    cfgView,
                                               TableGroupKey TableGroupKey,
                                               std::string   koc,
                                               TableVersion  newKOCVersion)
{
	// find first match of KOCAlias and TableGroupKey
	unsigned int row = 0;
	unsigned int tmpTableGroupKey;  // this is type to extract from table
	std::string  tmpKOC;
	unsigned int tmpOldKOCVersion;  // this is type to extract from table
	for(row = 0; row < cfgView->getNumberOfRows(); row++)
	{
		cfgView->getValue(tmpTableGroupKey, row, Configurations::TableGroupKeyAlias);
		if(TableGroupKey == tmpTableGroupKey)
		{
			cfgView->getValue(tmpKOC, row, Configurations::KOC);
			if(tmpKOC == koc)
			{
				cfgView->getValue(
				    tmpOldKOCVersion, row, Configurations::ConditionVersion);
				std::cout << __COUT_HDR_FL__ << "Found ConfigKey(" << TableGroupKey
				          << ") and KOCAlias(" << koc << ") pair."
				          << " Current version is: " << tmpOldKOCVersion
				          << " New version is: " << newKOCVersion << std::endl;
				if(newKOCVersion == tmpOldKOCVersion)
					return 1;  // no change necessary
				break;         // found row! exit search loop
			}
		}
	}

	// at this point have row to change

	std::cout << __COUT_HDR_FL__ << "Row found is:" << row << std::endl;
	cfgView->setValue(newKOCVersion, row, Configurations::ConditionVersion);
	std::cout << __COUT_HDR_FL__ << "Version changed to:" << newKOCVersion << std::endl;
	return 0;
}

//==============================================================================
// getListOfKocs
//	return list of Kind of Conditions that match TableGroupKey for the active
// configuration view. 	for all KOCs regardless of TableGroupKey, set TableGroupKey = -1
//
std::set<std::string> Configurations::getListOfKocs(TableGroupKey TableGroupKey) const
{
	std::set<std::string> kocs;
	getListOfKocsForView(TableBase::activeTableView_, kocs, TableGroupKey);
	return kocs;
}

//==============================================================================
// getListOfKocsForView
//	return list of Kind of Conditions that match TableGroupKey for the active
// configuration view. 	for all KOCs regardless of TableGroupKey, set TableGroupKey = -1
//
void Configurations::getListOfKocsForView(TableView*             cfgView,
                                          std::set<std::string>& kocs,
                                          TableGroupKey          TableGroupKey) const
{
	if(!cfgView)
	{
		std::cout << __COUT_HDR_FL__
		          << "*******************************************************************"
		             "*********************************************************"
		          << std::endl;
		std::cout << __COUT_HDR_FL__
		          << "\tCan't find list of Kocs for null cfgView pointer" << std::endl;
		std::cout << __COUT_HDR_FL__
		          << "*******************************************************************"
		             "*********************************************************"
		          << std::endl;
		__THROW__("Null cfgView configuration view pointer");
	}

	std::string  tmpKOC;
	unsigned int tmpTableGroupKey;  // this is type to extract from table

	for(unsigned int row = 0; row < cfgView->getNumberOfRows(); row++)
	{
		cfgView->getValue(tmpTableGroupKey, row, TableGroupKeyAlias);
		if(TableGroupKey == TableGroupKey::INVALID || TableGroupKey == tmpTableGroupKey)
		{
			cfgView->getValue(tmpKOC, row, KOC);
			kocs.insert(tmpKOC);
		}
	}
}

DEFINE_OTS_CONFIGURATION(Configurations)
