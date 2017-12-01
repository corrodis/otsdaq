#include "otsdaq-core/ConfigurationPluginDataFormats/DesktopIconConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

#include <iostream>
#include <fstream>      // std::fstream
#include <stdio.h>
using namespace ots;

#define DESKTOP_ICONS_FILE	std::string(getenv("SERVICE_DATA_PATH")) + "/OtsWizardData/iconList.dat"

//DesktopIconConfiguration Column names
#define COL_NAME						"IconName"
#define COL_STATUS						ViewColumnInfo::COL_NAME_STATUS
#define COL_CAPTION 					"Caption"
#define COL_ALTERNATE_TEXT				"AlternateText"
#define COL_FORCE_ONLY_ONE_INSTANCE		"ForceOnlyOneInstance"
#define COL_REQUIRED_PERMISSION_LEVEL	"RequiredPermissionLevel"
#define COL_IMAGE_URL					"ImageURL"
#define COL_WINDOW_CONTENT_URL			"WindowContentURL"
#define COL_APP_LINK					"LinkToApplicationConfiguration"
#define COL_PARAMETER_LINK				"LinkToParameterConfiguration"
#define COL_PARAMETER_KEY				"windowParameterKey"
#define COL_PARAMETER_VALUE				"windowParameterValue"
#define COL_FOLDER_PATH					"FolderPath"

//XDAQ App Column names
#define COL_APP_ID						"Id"



//==============================================================================
DesktopIconConfiguration::DesktopIconConfiguration(void) :
		ConfigurationBase("DesktopIconConfiguration")
{
 	//////////////////////////////////////////////////////////////////////
	//WARNING: the names used in C++ MUST match the Configuration INFO  //
	//////////////////////////////////////////////////////////////////////

	//	<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
//		<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
//			<CONFIGURATION Name="DesktopIconConfiguration">
//				<VIEW Name="DESKTOP_ICON_CONFIGURATION" Type="File,Database,DatabaseTest" Description="This%20table%20is%20used%20to%20specify%20the%20Icons%20available%20on%20the%20otsdaq%20Desktop.%20%0A%0AUsually%20a%20Desktop%20Icon%20opens%20the%20GUI%20to%20an%20otsdaq%20app%2C%20but%20an%20Icon%20may%20be%20configured%20to%20open%20any%20content%20accessible%20through%20the%20user's%20browser%20(Note%3A%20the%20target%20server%20may%20have%20to%20allow%20cross-origin%20requests%20for%20full%20functionality).%20%0A%0AHere%20is%20an%20explanation%20of%20the%20features%20associated%20with%20each%20column%3A%0A%3CINDENT%3E%0A-%20IconName%3A%0A%3CINDENT%3EThis%20is%20the%20unique%20ID%20value%20for%20each%20row.%3C%2FINDENT%3E%0A-%20Status%3A%0A%3CINDENT%3EIf%20On%2C%20the%20Icon%20will%20be%20shown%20on%20the%20Desktop.%20When%20Off%2C%20the%20Icon%20will%20not%20be%20displayed.%3C%2FINDENT%3E%0A-%20Caption%3A%0A%3CINDENT%3EThis%20is%20the%20text%20shown%20underneath%20the%20Icon.%3C%2FINDENT%3E%0A-%20AlternateText%3A%0A%3CINDENT%3EIf%20the%20ImageURL%20is%20omitted%20(left%20as%20default%20or%20blank)%2C%20then%20the%20alternate%20text%20is%20used%20in%20place%20of%20the%20Icon%20image.%20This%20is%20useful%20if%20you%20have%20not%20yet%20found%20an%20image%20to%20use%20for%20a%20particular%20icon.%3C%2FINDENT%3E%0A-%20ForceOnlyOneInstance%3A%0A%3CINDENT%3EIf%20True%2C%20then%20only%20window%20is%20allowed%20on%20the%20Desktop.%20If%20False%2C%20then%20each%20time%20the%20Icon%20is%20clicked%20a%20new%20instance%20of%20the%20window%20will%20open.%20For%20example%2C%20it%20is%20common%20to%20force%20only%20one%20instance%20of%20the%20state%20machine%20window.%3C%2FINDENT%3E%0A-%20RequiredPermissionLevel%3A%0A%3CINDENT%3EThis%20value%20is%20the%20minimum%20permission%20level%20for%20a%20user%20to%20have%20access%20to%20this%20Icon.%20If%20a%20user%20has%20insufficient%20access%2C%20the%20Icon%20will%20not%20appear%20on%20their%20Desktop.%20As%20a%20reminder%2C%20permission%20levels%20go%20from%201%20(lowest)%20to%20255%20(admin-level)%3B%20and%20only%20admins%20can%20modify%20the%20access%20level%20of%20users.%3C%2FINDENT%3E%0A-%20ImageURL%3A%0A%3CINDENT%3EThis%20is%20the%20URL%20to%20the%20image%20used%20for%20the%20Icon.%20The%20native%20resolution%20for%20Icons%20is%2064%20x%2064%20pixels.%20If%20left%20as%20the%20default%20value%20or%20blank%2C%20the%20the%20AlternateText%20field%20will%20be%20used%20for%20the%20Icon%20image.%3C%2FINDENT%3E%0A-%20WindowContentURL%3A%0A%3CINDENT%3EThis%20is%20the%20URL%20of%20the%20window%20content%20that%20will%20be%20opened%20when%20the%20user%20clicks%20this%20Icon.%20The%20window%20content%20is%20usually%20an%20otsdaq%20app%20but%20may%20be%20any%20web%20content%20accessible%20through%20the%20user's%20browser.%3C%2FINDENT%3E%0A-%20LinkToApplicationConfiguration%2FApplicationUID%3A%0A%3CINDENT%3EWhen%20the%20Icon%20refers%20to%20an%20otsdaq%20app%2C%20these%20two%20fields%20comprise%20the%20configuration%20link%20that%20connects%20the%20Icon%20to%20the%20app's%20ID%20-%20the%20app%20ID%20is%20used%20to%20target%20the%20app%20when%20sending%20requests.%20The%20LinkToApplicationConfiguration%20field%20is%20the%20configuration%20table%20part%20of%20the%20link%20and%20the%20ApplicationUID%20field%20is%20the%20UID%20part%20of%20the%20link.%3C%2FINDENT%3E%0A-%20FolderPath%3A%0A%3CINDENT%3EThis%20field%20is%20used%20to%20organize%20Icons%20into%20folders%20on%20the%20Desktop.%20For%20example%2C%20a%20value%20of%20%22myFolder%2FmySubfolder%22%20places%20this%20Icon%20inside%20a%20folder%20named%20%22mySubFolder%22%20which%20is%20inside%20the%20folder%20%22myFolder%22%20on%20the%20Desktop.%3C%2FINDENT%3E%0A%3C%2FINDENT%3E">
//					<COLUMN Type="UID" 	 Name="IconName" 	 StorageName="ICON_NAME" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="OnOff" 	 Name="Status" 	 StorageName="STATUS" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="Data" 	 Name="Caption" 	 StorageName="CAPTION" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="Data" 	 Name="AlternateText" 	 StorageName="ALTERNATE_TEXT" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="TrueFalse" 	 Name="ForceOnlyOneInstance" 	 StorageName="FORCE_ONLY_ONE_INSTANCE" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="Data" 	 Name="RequiredPermissionLevel" 	 StorageName="REQUIRED_PERMISSION_LEVEL" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="Data" 	 Name="ImageURL" 	 StorageName="IMAGE_URL" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="Data" 	 Name="WindowContentURL" 	 StorageName="WINDOW_CONTENT_URL" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="ChildLink-0" 	 Name="LinkToApplicationConfiguration" 	 StorageName="LINK_TO_APPLICATION_CONFIGURATION" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="ChildLinkUID-0" 	 Name="ApplicationUID" 	 StorageName="APPLICATION_UID" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="Data" 	 Name="FolderPath" 	 StorageName="FOLDER_PATH" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="Comment" 	 Name="CommentDescription" 	 StorageName="COMMENT_DESCRIPTION" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="Author" 	 Name="Author" 	 StorageName="AUTHOR" 		DataType="VARCHAR2" 		DataChoices=""/>
//					<COLUMN Type="Timestamp" 	 Name="RecordInsertionTime" 	 StorageName="RECORD_INSERTION_TIME" 		DataType="TIMESTAMP WITH TIMEZONE" 		DataChoices=""/>
//				</VIEW>
//			</CONFIGURATION>
//		</ROOT>

}

//==============================================================================
DesktopIconConfiguration::~DesktopIconConfiguration(void)
{}

//==============================================================================
void DesktopIconConfiguration::init(ConfigurationManager *configManager)
{
	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
	__COUT__ << configManager->__SELF_NODE__ << std::endl;


	bool 			status;
	std::string 	val;
	unsigned int	intVal;

	bool 			first = true;

	auto childrenMap = configManager->__SELF_NODE__.getChildren();


	//generate icons file
	std::fstream fs;
	fs.open(DESKTOP_ICONS_FILE, std::fstream::out | std::fstream::trunc);
	if(fs.fail())
	{
		__SS__ << "Failed to open Desktop Icons run file: " << DESKTOP_ICONS_FILE << std::endl;
		throw std::runtime_error(ss.str());
	}

	for(auto &child:childrenMap)
	{
		child.second.getNode(COL_STATUS	).getValue(status);
		if(!status) continue;

		if(first) first = false;
		else fs << ",";

		child.second.getNode(COL_CAPTION	).getValue(val);
		fs << removeCommas(val, false, true);
		//__COUT__ << "Icon caption: " << val << std::endl;

		fs << ",";
		child.second.getNode(COL_ALTERNATE_TEXT	).getValue(val);
		fs << removeCommas(val, false, true);

		fs << ",";
		child.second.getNode(COL_FORCE_ONLY_ONE_INSTANCE	).getValue(status);
		fs << (status?"1":"0");

		fs << ",";
		child.second.getNode(COL_REQUIRED_PERMISSION_LEVEL	).getValue(val);
		fs << removeCommas(val);

		fs << ",";
		child.second.getNode(COL_IMAGE_URL	).getValue(val);
		fs << removeCommas(val,true);

		fs << ",";
		child.second.getNode(COL_WINDOW_CONTENT_URL	).getValue(val);
		val = removeCommas(val,true);
		fs << val;

		bool addedAppId = false;
		//add URN/LID if link is given
		if(!child.second.getNode(COL_APP_LINK	).isDisconnected())
		{
			//if last character is not '='
			//	then assume need to add "?urn="
			if(val[val.size()-1] != '=')
				fs << "?urn=";

			//__COUT__ << "Following Application link." << std::endl;
			child.second.getNode(COL_APP_LINK	).getNode(COL_APP_ID		).getValue(intVal);

			//__COUT__ << "URN/LID=" << intVal << std::endl;
			fs << intVal; //append number
			addedAppId = true;
		}

		//add parameters if link is given
		if(!child.second.getNode(COL_PARAMETER_LINK	).isDisconnected())
		{
			//if there is no '?' found
			//	then assume need to add "?"
			if(val.find('?') == std::string::npos)
				fs << '?';
			else if(addedAppId ||
					val[val.size()-1] != '?') //if not first parameter, add &
				fs << '&';

			//now add each paramter separated by &
			auto paramGroupMap = child.second.getNode(COL_PARAMETER_LINK	).getChildren();
			bool notFirst = false;
			for(const auto param:paramGroupMap)
			{
				if(notFirst)
					fs << '&';
				else
					notFirst = true;
				fs << ConfigurationManager::encodeURIComponent(
						param.second.getNode(COL_PARAMETER_KEY).getValue<std::string>()) << "=" <<
								ConfigurationManager::encodeURIComponent(
										param.second.getNode(COL_PARAMETER_VALUE).getValue<std::string>());
			}
		}

		fs << ",";
		child.second.getNode(COL_FOLDER_PATH	).getValue(val);
		if(val == ViewColumnInfo::DATATYPE_STRING_DEFAULT) val = "";
 		fs << removeCommas(val,true);
	}

	//close icons file
	fs.close();
}

std::string DesktopIconConfiguration::removeCommas(const std::string &str,
		bool andHexReplace, bool andHTMLReplace)
{
	std::string retStr = "";
	retStr.reserve(str.length());

	for(unsigned int i=0;i<str.length();++i)
		if(str[i] != ',')
			retStr += str[i];
		else if(andHexReplace)
			retStr += "%2C";
		else if(andHTMLReplace)
			retStr += "&#44;";

	return retStr;
}

DEFINE_OTS_CONFIGURATION(DesktopIconConfiguration)
