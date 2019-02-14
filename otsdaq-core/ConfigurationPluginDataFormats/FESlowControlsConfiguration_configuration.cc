#include "otsdaq-core/ConfigurationPluginDataFormats/FESlowControlsConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

#include <iostream>

using namespace ots;

//==============================================================================
FESlowControlsConfiguration::FESlowControlsConfiguration(void)
    : ConfigurationBase("FESlowControlsConfiguration")
{
	//	<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//		<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
	//			<CONFIGURATION Name="FESlowControlsConfiguration">
	//				<VIEW Name="SLOW_CONTROLS_OTS_FE_CHANNELS_CONFIGURATION" Type="File,Database,DatabaseTest">
	//					<COLUMN Type="GroupID-SlowControls" 	 Name="FEGroupID" 	 StorageName="FE_GROUP_ID" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="UID" 	 Name="ChannelName" 	 StorageName="CHANNEL_NAME" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="Data" 	 Name="ChannelDataType" 	 StorageName="CHANNEL_DATA_TYPE" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="Data" 	 Name="UniversalInterfaceAddress" 	 StorageName="UNIVERSAL_INTERFACE_ADDRESS" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="YesNo" 	 Name="ReadAccess" 	 StorageName="READ_ACCESS" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="YesNo" 	 Name="WriteAccess" 	 StorageName="WRITE_ACCESS" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="YesNo" 	 Name="MonitoringEnabled" 	 StorageName="MONITORING_ENABLED" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="TrueFalse" 	 Name="TransmitChangesOnly" 	 StorageName="TRANSMIT_CHANGES_ONLY" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="Data" 	 Name="DelayBetweenSamples" 	 StorageName="DELAY_BETWEEN_SAMPLES" 		DataType="NUMBER"/>
	//					<COLUMN Type="YesNo" 	 Name="AlarmsEnabled" 	 StorageName="ALARMS_ENABLED" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="Data" 	 Name="LowThreshold" 	 StorageName="LOW_THRESHOLD" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="Data" 	 Name="LowLowThreshold" 	 StorageName="LOW_LOW_THRESHOLD" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="Data" 	 Name="HighThreshold" 	 StorageName="HIGH_THRESHOLD" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="Data" 	 Name="HighHighThreshold" 	 StorageName="HIGH_HIGH_THRESHOLD" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="Comment" 	 Name="CommentDescription" 	 StorageName="COMMENT_DESCRIPTION" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="Author" 	 Name="Author" 	 StorageName="AUTHOR" 		DataType="VARCHAR2"/>
	//					<COLUMN Type="Timestamp" 	 Name="RecordInsertionTime" 	 StorageName="RECORD_INSERTION_TIME" 		DataType="TIMESTAMP WITH TIMEZONE"/>
	//				</VIEW>
	//			</CONFIGURATION>
	//		</ROOT>
}

//==============================================================================
FESlowControlsConfiguration::~FESlowControlsConfiguration(void)
{
}

//==============================================================================
void FESlowControlsConfiguration::init(ConfigurationManager *configManager)
{
	//check for valid data types
	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
	__COUT__ << configManager->__SELF_NODE__ << std::endl;

	//	__COUT__ << configManager->getNode(this->getConfigurationName()).getValueAsString()
	//		  											  << std::endl;

	std::string childType;
	auto        childrenMap = configManager->__SELF_NODE__.getChildren();
	for (auto &childPair : childrenMap)
	{
		//check each row in table
		__COUT__ << childPair.first << std::endl;
		childPair.second.getNode(colNames_.colDataType_).getValue(childType);
		__COUT__ << "childType=" << childType << std::endl;

		if (childType[childType.size() - 1] == 'b')  //if ends in 'b' then take that many bits
		{
			unsigned int sz;
			sscanf(&childType[0], "%u", &sz);
			if (sz < 1 || sz > 64)
			{
				__SS__ << "Data type '" << childType << "' for UID=" << childPair.first << " is invalid. "
				       << " The bit size given was " << sz << " and it must be between 1 and 64." << std::endl;
				__COUT_ERR__ << "\n"
				             << ss.str();
				__SS_THROW__;
			}
		}
		else if (childType != "char" &&
		         childType != "short" &&
		         childType != "int" &&
		         childType != "unsigned int" &&
		         childType != "long long " &&
		         childType != "unsigned long long" &&
		         childType != "float" &&
		         childType != "double")
		{
			__SS__ << "Data type '" << childType << "' for UID=" << childPair.first << " is invalid. "
			       << "Valid data types (w/size in bytes) are as follows: "
			       << "#b (# bits)"
			       << ", char (" << sizeof(char) << "B), unsigned char (" << sizeof(unsigned char) << "B), short (" << sizeof(short) << "B), unsigned short (" << sizeof(unsigned short) << "B), int (" << sizeof(int) << "B), unsigned int (" << sizeof(unsigned int) << "B), long long (" << sizeof(long long) << "B), unsigned long long (" << sizeof(unsigned long long) << "B), float (" << sizeof(float) << "B), double (" << sizeof(double) << "B)." << std::endl;
			__COUT_ERR__ << "\n"
			             << ss.str();
			__SS_THROW__;
		}
	}
}

DEFINE_OTS_CONFIGURATION(FESlowControlsConfiguration)
