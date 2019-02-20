#include <iostream>
#include "../../Macros/TablePluginMacros.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/FETable.h"

using namespace ots;

//==============================================================================
FEConfiguration::FEConfiguration(void) : TableBase("FEConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	// FEConfigurationInfo.xml
	//<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	// xsi:noNamespaceSchemaLocation="TableInfo.xsd">
	//  <CONFIGURATION Name="FEConfiguration">
	//    <VIEW Name="FE_CONFIGURATION" Type="File,Database,DatabaseTest">
	//      <COLUMN Name="SupervisorType"     StorageName="SUPERVISOR_TYPE"
	//      DataType="VARCHAR2"/> <COLUMN Name="SupervisorInstance"
	//      StorageName="SUPERVISOR_INSTANCE" DataType="NUMBER"  /> <COLUMN
	//      Name="FrontEndType"       StorageName="FRONT_END_TYPE" DataType="VARCHAR2"/>
	//      <COLUMN Name="FrontEndID"         StorageName="INTERFACE_ID"
	//      DataType="VARCHAR2"/>
	//    </VIEW>
	//  </CONFIGURATION>
	//</ROOT>
}

//==============================================================================
FEConfiguration::~FEConfiguration(void) {}

//==============================================================================
void FEConfiguration::init(ConfigurationManager* configManager)
{
	std::string  tmpType;
	unsigned int tmpInstance;
	std::string  tmpID;
	for(unsigned int row = 0; row < TableBase::activeTableView_->getNumberOfRows(); row++)
	{
		TableBase::activeTableView_->getValue(tmpType, row, SupervisorType);
		TableBase::activeTableView_->getValue(tmpInstance, row, SupervisorInstance);
		TableBase::activeTableView_->getValue(tmpID, row, FrontEndId);
		std::cout << __COUT_HDR_FL__ << "Type: " << tmpType << " Name: " << tmpInstance
		          << " row: " << row << std::endl;
		typeNameToRow_[composeUniqueName(tmpType, tmpInstance)][tmpID] = row;
	}
}
//
////==============================================================================
// std::vector<std::string> FEConfiguration::getListOfFEIDs(void) const
//{
//	std::vector<std::string> list;
//	for(const auto& itSupervisors : typeNameToRow_)
//		for(const auto& itFrontEnds : itSupervisors.second)
//			list.push_back(itFrontEnds.first);
//	return list;
//}
//
////==============================================================================
// std::vector<std::string> FEConfiguration::getListOfFEIDs(const std::string&
// supervisorType, unsigned int supervisorInstance) const
//{
//	std::string uniqueName = composeUniqueName(supervisorType, supervisorInstance);
//	std::vector<std::string> list;
//	if(typeNameToRow_.find(uniqueName) == typeNameToRow_.end())
//	{
//		std::cout << __COUT_HDR_FL__
//				<< "Couldn't find any supervisor of type " << supervisorType
//				<< " and instance " << supervisorInstance
//				<< std::endl;
//		//assert(0);  //RAR - 8/12/16 - empty list is ok
//	}
//	else
//	{
//		for(const auto& it : typeNameToRow_.find(uniqueName)->second)
//			list.push_back(it.first);
//	}
//
//	return list;
//}
//
////==============================================================================
// const std::string FEConfiguration::getFEInterfaceType(const std::string& frontEndID)
// const
//{
//	for(const auto& itSupervisors : typeNameToRow_)
//		for(const auto& itFrontEnds : itSupervisors.second)
//			if(itFrontEnds.first == frontEndID)
//				return
// TableBase::getView().getDataView()[typeNameToRow_.find(itSupervisors.first)->second.find(frontEndID)->second][FrontEndType];
//	std::cout << __COUT_HDR_FL__ << "Didn't find any interface with ID: " << frontEndID <<
// std::endl; 	assert(0);
//	__THROW__("FEID not found!");
//	return "";
//}
//
////==============================================================================
// const std::string FEConfiguration::getFEInterfaceType(const std::string&
// supervisorType, unsigned int supervisorInstance, const std::string& frontEndID) const
//{
//	std::string uniqueName = composeUniqueName(supervisorType, supervisorInstance);
//	if(typeNameToRow_.find(uniqueName) == typeNameToRow_.end())
//	{
//		std::cout << __COUT_HDR_FL__
//				<< "Couldn't find any supervisor of type " << supervisorType
//				<< " and instance " << supervisorInstance
//				<< std::endl;
//		assert(0);
//	}
//	if(typeNameToRow_.find(uniqueName)->second.find(frontEndID) ==
// typeNameToRow_.find(uniqueName)->second.end())
//	{
//		std::cout << __COUT_HDR_FL__
//				<< "Couldn't find any front end ID " << frontEndID
//				<< " for supervisor type " << supervisorType
//				<< " and instance " << supervisorInstance
//				<< std::endl;
//		assert(0);
//	}
//
//	return
// TableBase::getView().getDataView()[typeNameToRow_.find(uniqueName)->second.find(frontEndID)->second][FrontEndType];
//}

////==============================================================================
// std::vector<unsigned int> FEConfiguration::getListOfFEWRs(void) const
//{
//    return getListOfFEs("FEWR");
//}
//
////==============================================================================
// std::vector<unsigned int> FEConfiguration::getListOfFEWRs(unsigned int
// supervisorInstance) const
//{
//    return getListOfFEs("FEWR",supervisorInstance);
//}
//
////==============================================================================
// const std::string& FEConfiguration::getFEWRInterfaceName(unsigned int id) const
//{
//	return getFEInterfaceType("FEWR", id);
//}
//
////==============================================================================
// std::vector<unsigned int> FEConfiguration::getListOfFEWs(void) const
//{
//    return getListOfFEs("FEW");
//}
//
////==============================================================================
// std::vector<unsigned int> FEConfiguration::getListOfFEWs(unsigned int
// supervisorInstance) const
//{
//    return getListOfFEs("FEW",supervisorInstance);
//}
//
////==============================================================================
// const std::string& FEConfiguration::getFEWInterfaceName(unsigned int id) const
//{
//	return getFEInterfaceType("FEW", id);
//}
//
////==============================================================================
// std::vector<unsigned int> FEConfiguration::getListOfFERs(void) const
//{
//    return getListOfFEs("FER");
//}
//
////==============================================================================
// std::vector<unsigned int> FEConfiguration::getListOfFERs(unsigned int
// supervisorInstance) const
//{
//    return getListOfFEs("FER",supervisorInstance);
//}
//
////==============================================================================
// const std::string& FEConfiguration::getFERInterfaceName(unsigned int id) const
//{
//	return getFEInterfaceType("FER", id);
//}

DEFINE_OTS_CONFIGURATION(FEConfiguration)
