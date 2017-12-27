#include "otsdaq-core/ConfigurationDataFormats/ConfigurationView.h"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <regex>

using namespace ots;

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "ConfigurationView"
#undef 	__MF_HDR__
#define __MF_HDR__		tableName_ << ":v" << version_ << ":" << __COUT_HDR_FL__

const unsigned int ConfigurationView::INVALID = -1;

//==============================================================================
ConfigurationView::ConfigurationView(const std::string &name)
:	uniqueStorageIdentifier_	(""),
	tableName_   				(name),
	version_					(ConfigurationVersion::INVALID),
	comment_					(""),
	author_ 					(""),
	creationTime_ 				(time(0)),
	lastAccessTime_				(0),
	colUID_ 					(INVALID),
	colStatus_					(INVALID),
	fillWithLooseColumnMatching_(false),
	sourceColumnMismatchCount_	(0),
	sourceColumnMissingCount_ 	(0)
{}

//==============================================================================
ConfigurationView::~ConfigurationView(void)
{}

//==============================================================================
//operator=
//	Do NOT allow!... use ConfigurationView::copy
//	copy is used to maintain consistency with version, creationTime, lastAccessTime, etc)
ConfigurationView& ConfigurationView::operator=(const ConfigurationView src)
{
	__SS__ << "Invalid use of operator=... Should not directly copy a ConfigurationView. Please use ConfigurationView::copy(sourceView,author,comment)";
	throw std::runtime_error(ss.str());
}

//==============================================================================
ConfigurationView& ConfigurationView::copy(const ConfigurationView &src,
		ConfigurationVersion destinationVersion,
		const std::string &author)
{
	tableName_   		= src.tableName_;
	version_			= destinationVersion;
	comment_			= src.comment_;
	author_ 			= author;	//take new author
	//creationTime_ 	= time(0); //don't change creation time
	lastAccessTime_		= time(0);
	columnsInfo_		= src.columnsInfo_;
	theDataView_		= src.theDataView_;
	sourceColumnNames_ 	= src.sourceColumnNames_;
	init(); // verify consistency
	return *this;
}

//==============================================================================
//init
//	Should be called after being filled to setup special members
//		and verify consistency.
//	e.g. identifying the UID column
//
// 	Note: this function also sanitizes yes/no, on/off, and true/false types
void ConfigurationView::init(void)
{
	try
	{
		//verify column names are unique
		//	make set of names,.. and CommentDescription == COMMENT
		std::set<std::string> colNameSet;
		std::string capsColName, colName;
		for(auto &colInfo: columnsInfo_)
		{
			colName = colInfo.getStorageName();
			if(colName == "COMMENT_DESCRIPTION")
				colName = "COMMENT";
			capsColName = "";
			for(unsigned int i=0;i<colName.size(); ++i)
			{
				if(colName[i] == '_') continue;
				capsColName += colName[i];
			}

			colNameSet.emplace(capsColName);
		}

		if(colNameSet.size() != columnsInfo_.size())
		{
			__SS__ << "Configuration Error:\t" <<
					" Columns names must be unique! There are " << columnsInfo_.size() <<
					" columns and the unique name count is " << colNameSet.size() << std::endl;
			__COUT_ERR__ << "\n" << ss.str();

			throw std::runtime_error(ss.str());
		}

		getOrInitColUID(); //setup UID column
		try
		{
			getOrInitColStatus(); //setup Status column
		}
		catch(...){} //ignore no Status column

		//require one comment column
		unsigned int colPos;
		if((colPos = findColByType(ViewColumnInfo::TYPE_COMMENT)) != INVALID)
		{
			if(columnsInfo_[colPos].getName() != "CommentDescription")
			{
				__SS__ << "Configuration Error:\t" << ViewColumnInfo::TYPE_COMMENT <<
						" data type column must have name=" <<
						"CommentDescription" << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}

			if(findColByType(ViewColumnInfo::TYPE_COMMENT,colPos+1) != INVALID) //found two!
			{
				__SS__ << "Configuration Error:\t" << ViewColumnInfo::TYPE_COMMENT <<
						" data type in column " <<
						columnsInfo_[colPos].getName() <<
						" is repeated. This is not allowed." << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}

			if(colPos != getNumberOfColumns()-3)
			{
				__SS__ << "Configuration Error:\t" << ViewColumnInfo::TYPE_COMMENT <<
						" data type column must be 3rd to last (in column " <<
						getNumberOfColumns()-3 << ")." << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}
		}
		else
		{
			__SS__ << "Configuration Error:\t" << ViewColumnInfo::TYPE_COMMENT <<
					" data type column " <<" is missing. This is not allowed." << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}

		//require one author column
		if((colPos = findColByType(ViewColumnInfo::TYPE_AUTHOR)) != INVALID)
		{
			if(findColByType(ViewColumnInfo::TYPE_AUTHOR,colPos+1) != INVALID) //found two!
			{
				__SS__ << "Configuration Error:\t" << ViewColumnInfo::TYPE_AUTHOR <<
						" data type in column " <<
						columnsInfo_[colPos].getName() <<
						" is repeated. This is not allowed." << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}

			if(colPos != getNumberOfColumns()-2)
			{
				__SS__ << "Configuration Error:\t" << ViewColumnInfo::TYPE_AUTHOR <<
						" data type column must be 2nd to last (in column " <<
						getNumberOfColumns()-2 << ")." << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}
		}
		else
		{
			__SS__ << "Configuration Error:\t" << ViewColumnInfo::TYPE_AUTHOR <<
					" data type column " <<" is missing. This is not allowed." << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}

		//require one timestamp column
		if((colPos = findColByType(ViewColumnInfo::TYPE_TIMESTAMP)) != INVALID)
		{
			if(findColByType(ViewColumnInfo::TYPE_TIMESTAMP,colPos+1) != INVALID) //found two!
			{
				__SS__ << "Configuration Error:\t" << ViewColumnInfo::TYPE_TIMESTAMP <<
						" data type in column " <<
						columnsInfo_[colPos].getName() << " is repeated. This is not allowed." <<
						std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}

			if(colPos != getNumberOfColumns()-1)
			{
				__SS__ << "Configuration Error:\t" << ViewColumnInfo::TYPE_TIMESTAMP <<
						" data type column must be last (in column " <<
						getNumberOfColumns()-1 << ")." << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}
		}
		else
		{
			__SS__ << "Configuration Error:\t" << ViewColumnInfo::TYPE_TIMESTAMP <<
					" data type column " <<" is missing. This is not allowed." << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}

		//check that UID is really unique ID (no repeats)
		// and ... allow letters, numbers, dash, underscore
		// and ... force size 1
		std::set<std::string /*uid*/> uidSet;
		for(unsigned int row = 0; row < getNumberOfRows(); ++row)
		{
			if(uidSet.find(theDataView_[row][colUID_]) != uidSet.end())
			{
				__SS__ << ("Entries in UID are not unique. Specifically at row=" +
						std::to_string(row) + " value=" + theDataView_[row][colUID_])<< std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}

			if(theDataView_[row][colUID_].size() == 0)
			{
				__SS__ << "An invalid UID '" << theDataView_[row][colUID_] << "' " <<
						" was identified. UIDs must contain at least 1 character." <<
						std::endl;
				throw std::runtime_error(ss.str());
			}

			for(unsigned int i=0;i<theDataView_[row][colUID_].size();++i)
				if(!(
						(theDataView_[row][colUID_][i] >= 'A' && theDataView_[row][colUID_][i] <= 'Z') ||
						(theDataView_[row][colUID_][i] >= 'a' && theDataView_[row][colUID_][i] <= 'z') ||
						(theDataView_[row][colUID_][i] >= '0' && theDataView_[row][colUID_][i] <= '9') ||
						(theDataView_[row][colUID_][i] == '-' ||
								theDataView_[row][colUID_][i] <= '_')
				))
				{
					__SS__ << "An invalid UID '" << theDataView_[row][colUID_] << "' " <<
							" was identified. UIDs must contain only letters, numbers," <<
							"dashes, and underscores." << std::endl;
					throw std::runtime_error(ss.str());
				}

			uidSet.insert(theDataView_[row][colUID_]);
		}
		if(uidSet.size() != getNumberOfRows())
		{
			__SS__ << "Entries in UID are not unique!" <<
					"There are " << getNumberOfRows() <<
					" rows and the unique UID count is " << uidSet.size() << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}

		//check that any TYPE_UNIQUE_DATA columns are really unique (no repeats)
		colPos = (unsigned int)-1;
		while((colPos = findColByType(ViewColumnInfo::TYPE_UNIQUE_DATA,colPos+1)) != INVALID)
		{
			std::set<std::string /*unique data*/> uDataSet;
			for(unsigned int row = 0; row < getNumberOfRows(); ++row)
			{
				if(uDataSet.find(theDataView_[row][colPos]) != uDataSet.end())
				{
					__SS__ << "Entries in Unique Data column " <<
							columnsInfo_[colPos].getName() <<
							(" are not unique. Specifically at row=" +
							std::to_string(row) + " value=" + theDataView_[row][colPos]) <<
							std::endl;
					__COUT_ERR__ << "\n" << ss.str();
					throw std::runtime_error(ss.str());
				}
				uDataSet.insert(theDataView_[row][colPos]);
			}
			if(uDataSet.size() != getNumberOfRows())
			{
				__SS__ << "Entries in  Unique Data column " <<
							columnsInfo_[colPos].getName() << " are not unique!" <<
						"There are " << getNumberOfRows() <<
						" rows and the unique data count is " << uDataSet.size() << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}
		}

		auto rowDefaults = getDefaultRowValues();

		//check that column types are well behaved
		//	- check that fixed choice data is one of choices
		//	- sanitize booleans
		//	- check that child link I are unique
		//		note: childLinkId refers to childLinkGroupIDs AND childLinkUIDs
		std::set<std::string> groupIdIndexes, childLinkIndexes, childLinkIdLabels;
		unsigned int	groupIdIndexesCount = 0, childLinkIndexesCount = 0, childLinkIdLabelsCount = 0;
		for(unsigned int col = 0; col < getNumberOfColumns(); ++col)
		{
			if(columnsInfo_[col].getType() == ViewColumnInfo::TYPE_FIXED_CHOICE_DATA)
			{
				const std::vector<std::string>& theDataChoices =
						columnsInfo_[col].getDataChoices();

				//check if arbitrary values allowed
				if(theDataChoices.size() && theDataChoices[0] ==
						"arbitraryBool=1")
					continue; //arbitrary values allowed

				bool found;
				for(unsigned int row = 0; row < getNumberOfRows(); ++row)
				{
					found = false;
					//check against default value first
					if(theDataView_[row][col] == rowDefaults[col])
						continue; //default is always ok

					for(const auto &choice:theDataChoices)
					{
						if(theDataView_[row][col] == choice)
						{
							found = true;
							break;
						}
					}
					if(!found)
					{
						__SS__ << "Configuration Error:\t'" << theDataView_[row][col] << "' in column " <<
								columnsInfo_[col].getName() << " is not a valid Fixed Choice option. " <<
								"Possible values are as follows: ";

						for(unsigned int i = 0; i < columnsInfo_[col].getDataChoices().size(); ++i)
						{
							if(i) ss << ", ";
							ss << columnsInfo_[col].getDataChoices()[i];
						}
						ss << "." << std::endl;
						__COUT_ERR__ << "\n" << ss.str();
						throw std::runtime_error(ss.str());
					}
				}
			}
			if(columnsInfo_[col].isChildLink())
			{
				//check if forcing fixed choices

				const std::vector<std::string>& theDataChoices =
						columnsInfo_[col].getDataChoices();

				//check if arbitrary values allowed
				if(!theDataChoices.size() ||
						theDataChoices[0] == "arbitraryBool=1")
					continue; //arbitrary values allowed

				//skip one if arbitrary setting is embedded as first value
				bool skipOne = (theDataChoices.size() &&
						theDataChoices[0] == "arbitraryBool=0");
				bool hasSkipped;

				bool found;
				for(unsigned int row = 0; row < getNumberOfRows(); ++row)
				{
					found = false;

					hasSkipped = false;
					for(const auto &choice:theDataChoices)
					{
						if(skipOne && !hasSkipped) {hasSkipped = true; continue;}

						if(theDataView_[row][col] == choice)
						{
							found = true;
							break;
						}
					}
					if(!found)
					{
						__SS__ << "Configuration Error:\t'" << theDataView_[row][col] << "' in column " <<
								columnsInfo_[col].getName() << " is not a valid Fixed Choice option. " <<
								"Possible values are as follows: ";

						for(unsigned int i = skipOne?1:0; i < columnsInfo_[col].getDataChoices().size(); ++i)
						{
							if(i == 1 + (skipOne?1:0)) ss << ", ";
							ss << columnsInfo_[col].getDataChoices()[i];
						}
						ss << "." << std::endl;
						__COUT_ERR__ << "\n" << ss.str();
						throw std::runtime_error(ss.str());
					}
				}
			}
			else if(columnsInfo_[col].getType() == ViewColumnInfo::TYPE_ON_OFF)
				for(unsigned int row = 0; row < getNumberOfRows(); ++row)
				{
					if (theDataView_[row][col] == "1" || theDataView_[row][col] == "on" || theDataView_[row][col] == "On" || theDataView_[row][col] == "ON")
						theDataView_[row][col] = ViewColumnInfo::TYPE_VALUE_ON;
					else if (theDataView_[row][col] == "0" || theDataView_[row][col] == "off" || theDataView_[row][col] == "Off" || theDataView_[row][col] == "OFF")
						theDataView_[row][col] = ViewColumnInfo::TYPE_VALUE_OFF;
					else
					{
						__SS__ << "Configuration Error:\t" << theDataView_[row][col] << " in column " <<
								columnsInfo_[col].getName() << " is not a valid Type (On/Off) std::string. Possible values are 1, on, On, ON, 0, off, Off, OFF." << std::endl;
						__COUT_ERR__ << "\n" << ss.str();
						throw std::runtime_error(ss.str());
					}
				}
			else if(columnsInfo_[col].getType() == ViewColumnInfo::TYPE_TRUE_FALSE)
				for(unsigned int row = 0; row < getNumberOfRows(); ++row)
				{
					if (theDataView_[row][col] == "1" || theDataView_[row][col] == "true" || theDataView_[row][col] == "True" || theDataView_[row][col] == "TRUE")
						theDataView_[row][col] = ViewColumnInfo::TYPE_VALUE_TRUE;
					else if (theDataView_[row][col] == "0" || theDataView_[row][col] == "false" || theDataView_[row][col] == "False" || theDataView_[row][col] == "FALSE")
						theDataView_[row][col] = ViewColumnInfo::TYPE_VALUE_FALSE;
					else
					{
						__SS__ << "Configuration Error:\t" << theDataView_[row][col] << " in column " <<
								columnsInfo_[col].getName() << " is not a valid Type (True/False) std::string. Possible values are 1, true, True, TRUE, 0, false, False, FALSE." << std::endl;
						__COUT_ERR__ << "\n" << ss.str();
						throw std::runtime_error(ss.str());
					}
				}
			else if(columnsInfo_[col].getType() == ViewColumnInfo::TYPE_YES_NO)
				for(unsigned int row = 0; row < getNumberOfRows(); ++row)
				{
					if (theDataView_[row][col] == "1" || theDataView_[row][col] == "yes" || theDataView_[row][col] == "Yes" || theDataView_[row][col] == "YES")
						theDataView_[row][col] = ViewColumnInfo::TYPE_VALUE_YES;
					else if (theDataView_[row][col] == "0" || theDataView_[row][col] == "no" || theDataView_[row][col] == "No" || theDataView_[row][col] == "NO")
						theDataView_[row][col] = ViewColumnInfo::TYPE_VALUE_NO;
					else
					{
						__SS__ << "Configuration Error:\t" << theDataView_[row][col] << " in column " <<
								columnsInfo_[col].getName() << " is not a valid Type (Yes/No) std::string. Possible values are 1, yes, Yes, YES, 0, no, No, NO." << std::endl;
						__COUT_ERR__ << "\n" << ss.str();
						throw std::runtime_error(ss.str());
					}
				}
			else if(columnsInfo_[col].isGroupID())	//GroupID type
			{
				colLinkGroupIDs_[columnsInfo_[col].getChildLinkIndex()] = col; //add to groupid map
				//check uniqueness
				groupIdIndexes.emplace(columnsInfo_[col].getChildLinkIndex());
				++groupIdIndexesCount;
			}
			else if(columnsInfo_[col].isChildLink())	//Child Link type
			{
				//sanitize no link to default
				for(unsigned int row = 0; row < getNumberOfRows(); ++row)
					if(theDataView_[row][col] == "NoLink" ||
							theDataView_[row][col] == "No_Link" ||
							theDataView_[row][col] == "NOLINK" ||
							theDataView_[row][col] == "NO_LINK" ||
							theDataView_[row][col] == "Nolink" ||
							theDataView_[row][col] == "nolink" ||
							theDataView_[row][col] == "noLink")
						theDataView_[row][col] = ViewColumnInfo::DATATYPE_LINK_DEFAULT;

				//check uniqueness
				childLinkIndexes.emplace(columnsInfo_[col].getChildLinkIndex());
				++childLinkIndexesCount;

				//force data type to ViewColumnInfo::DATATYPE_STRING
				if(columnsInfo_[col].getDataType() != ViewColumnInfo::DATATYPE_STRING)
				{
					__SS__ << "Configuration Error:\t" << "Column " << col <<
							" with name " << columnsInfo_[col].getName() <<
							" is a Child Link column and has an illegal data type of '" <<
							columnsInfo_[col].getDataType() <<
							"'. The data type for Child Link columns must be " <<
							ViewColumnInfo::DATATYPE_STRING << std::endl;
					__COUT_ERR__ << "\n" << ss.str();
					throw std::runtime_error(ss.str());
				}

			}
			else if(columnsInfo_[col].isChildLinkUID() || 	//Child Link ID type
					columnsInfo_[col].isChildLinkGroupID())
			{
				//check uniqueness
				childLinkIdLabels.emplace(columnsInfo_[col].getChildLinkIndex());
				++childLinkIdLabelsCount;

				//check that the Link ID is not empty, and force to default
				for(unsigned int row = 0; row < getNumberOfRows(); ++row)
					if(theDataView_[row][col] == "")
						theDataView_[row][col] = rowDefaults[col];

			}
		}

		//verify child link index uniqueness
		if(groupIdIndexes.size() != groupIdIndexesCount)
		{
			__SS__ << ("GroupId Labels are not unique!") <<
					"There are " << groupIdIndexesCount <<
					" GroupId Labels and the unique count is " << groupIdIndexes.size() << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}
		if(childLinkIndexes.size() != childLinkIndexesCount)
		{
			__SS__ << ("Child Link Labels are not unique!") <<
					"There are " << childLinkIndexesCount <<
					" Child Link Labels and the unique count is " << childLinkIndexes.size() << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}
		if(childLinkIdLabels.size() != childLinkIdLabelsCount)
		{
			__SS__ << ("Child Link ID Labels are not unique!") <<
					"There are " << childLinkIdLabelsCount <<
					" Child Link ID Labels and the unique count is " << childLinkIdLabels.size() << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}

	}
	catch(...)
	{
		__COUT__ << "Error occured in ConfigurationView::init() for version=" << version_ << std::endl;
		throw;
	}
}

//==============================================================================
//getValue
//	string version
//	Note: necessary because types of std::basic_string<char> cause compiler problems if no string specific function
void ConfigurationView::getValue(std::string& value, unsigned int row, unsigned int col, bool convertEnvironmentVariables) const
{
	//getValue<std::string>(value,row,col,convertEnvironmentVariables);

	if(!(col < columnsInfo_.size() && row < getNumberOfRows()))
	{
		__SS__ << "Invalid row col requested" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}

	value = validateValueForColumn(theDataView_[row][col],col,convertEnvironmentVariables);

//	if(!(col < columnsInfo_.size() && row < getNumberOfRows()))
//	{
//		__SS__ << "Invalid row col requested" << std::endl;
//		__COUT_ERR__ << "\n" << ss.str();
//		throw std::runtime_error(ss.str());
//	}
//
//
//	if(columnsInfo_[col].getDataType() == ViewColumnInfo::DATATYPE_STRING)
//	{
//		value = theDataView_[row][col];
//		if(convertEnvironmentVariables)
//			value = convertEnvVariables(value);
//	}
//	else if(columnsInfo_[col].getDataType() == ViewColumnInfo::DATATYPE_TIME)
//	{
//		value.resize(30); //known fixed size: Thu Aug 23 14:55:02 2001 CST
//		time_t timestamp(strtol(theDataView_[row][col].c_str(),0,10));
//		struct tm tmstruct;
//		::localtime_r(&timestamp, &tmstruct);
//		::strftime(&value[0], 30, "%c %Z", &tmstruct);
//		value.resize(strlen(value.c_str()));
//	}
//	else
//	{
//		__SS__ << "\tUnrecognized column data type: " << columnsInfo_[col].getDataType()
//								<< " in configuration " << tableName_
//								<< " at column=" << columnsInfo_[col].getName()
//								<< " for getValue with type '" << ots_demangle(typeid(value).name())
//								<< "'" << std::endl;
//		throw std::runtime_error(ss.str());
//	}


}


//==============================================================================
//validateValueForColumn
//	string version
//	Note: necessary because types of std::basic_string<char> cause compiler problems if no string specific function
std::string ConfigurationView::validateValueForColumn(const std::string& value, unsigned int col, bool convertEnvironmentVariables) const
{
		//return validateValueForColumn<std::string>(value,col,convertEnvironmentVariables);
		if(col >= columnsInfo_.size())
		{
			__SS__ << "Invalid col requested" << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}

		std::string retValue;

		if(columnsInfo_[col].getDataType() == ViewColumnInfo::DATATYPE_STRING)
			retValue = convertEnvironmentVariables?convertEnvVariables(value):value;
		else if(columnsInfo_[col].getDataType() == ViewColumnInfo::DATATYPE_TIME)
		{
			retValue.resize(30); //known fixed size: Thu Aug 23 14:55:02 2001 CST
			time_t timestamp(
					strtol((convertEnvironmentVariables?convertEnvVariables(value):value).c_str(),
							0,10));
			struct tm tmstruct;
			::localtime_r(&timestamp, &tmstruct);
			::strftime(&retValue[0], 30, "%c %Z", &tmstruct);
			retValue.resize(strlen(retValue.c_str()));
		}
		else
		{
			__SS__ << "\tUnrecognized column data type: " << columnsInfo_[col].getDataType()
				<< " in configuration " << tableName_
				<< " at column=" << columnsInfo_[col].getName()
				<< " for getValue with type '" << ots_demangle(typeid(retValue).name())
				<< "'" << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}

		return retValue;
	}


//==============================================================================
//getValueAsString
//	gets the value with the proper data type and converts to string
//	as though getValue was called.
std::string ConfigurationView::getValueAsString(unsigned int row, unsigned int col,
		bool convertEnvironmentVariables) const
{
	if(!(col < columnsInfo_.size() && row < getNumberOfRows()))
	{
		__SS__ << ("Invalid row col requested") << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}

	//__COUT__ << columnsInfo_[col].getType() << " " << col << std::endl;

	if(columnsInfo_[col].getType() == ViewColumnInfo::TYPE_ON_OFF)
	{
		if (theDataView_[row][col] == "1" || theDataView_[row][col] == "on" || theDataView_[row][col] == "On" || theDataView_[row][col] == "ON")
			return ViewColumnInfo::TYPE_VALUE_ON;
		else
			return ViewColumnInfo::TYPE_VALUE_OFF;
	}
	else if(columnsInfo_[col].getType() == ViewColumnInfo::TYPE_TRUE_FALSE)
	{
		if (theDataView_[row][col] == "1" || theDataView_[row][col] == "true" || theDataView_[row][col] == "True" || theDataView_[row][col] == "TRUE")
			return ViewColumnInfo::TYPE_VALUE_TRUE;
		else
			return ViewColumnInfo::TYPE_VALUE_FALSE;
	}
	else if(columnsInfo_[col].getType() == ViewColumnInfo::TYPE_YES_NO)
	{
		if (theDataView_[row][col] == "1" || theDataView_[row][col] == "yes" || theDataView_[row][col] == "Yes" || theDataView_[row][col] == "YES")
			return ViewColumnInfo::TYPE_VALUE_YES;
		else
			return ViewColumnInfo::TYPE_VALUE_NO;
	}

	//__COUT__ << std::endl;
	return convertEnvironmentVariables?convertEnvVariables(theDataView_[row][col]):
			theDataView_[row][col];
}


//==============================================================================
//getEscapedValueAsString
//	gets the value with the proper data type and converts to string
//	as though getValue was called.
//	then escapes all special characters with slash.
//	Note: this should be useful for values placed in double quotes, i.e. JSON.
std::string ConfigurationView::getEscapedValueAsString(unsigned int row, unsigned int col,
		bool convertEnvironmentVariables) const
{
	std::string val = getValueAsString(row,col,convertEnvironmentVariables);
	std::string retVal = "";
	retVal.reserve(val.size()); //reserve roughly right size
	for(unsigned int i=0;i<val.size();++i)
	{
		if(val[i] == '\n')
			retVal += "\\n";
		else if(val[i] == '\t')
			retVal += "\\t";
		else if(val[i] == '\r')
			retVal += "\\r";
		else
		{
			//escaped characters need a
			if(val[i] == '"' ||
					val[i] == '\\')
				retVal += '\\';
			retVal += val[i];
		}
	}
	return retVal;
}

//==============================================================================
std::string ConfigurationView::convertEnvVariables(const std::string& data) const
{
	std::string converted = data;
	if(data.find("${") != std::string::npos)
	{
		unsigned int begin = data.find("${");
		unsigned int end   = data.find("}");
		std::string envVariable = data.substr(begin+2, end-begin-2);
		//std::cout << "Converted Value: " << envVariable << std::endl;
		if(getenv(envVariable.c_str()) != nullptr)
		{
			return convertEnvVariables(converted.replace(begin,end-begin+1,getenv(envVariable.c_str())));
		}
		else
		{
			__SS__ << ("In configuration " + tableName_ + " the environmental variable: " + envVariable +
					" is not set! Please make sure you set it before continuing!") << std::endl;
			__COUT_ERR__ << ss.str();
			throw std::runtime_error(ss.str());
		}
	}
	return converted;
}

//==============================================================================
//setValue
//	string version
void ConfigurationView::setValue(const std::string &value, unsigned int row, unsigned int col)
{
	if(!(col < columnsInfo_.size() && row < getNumberOfRows()))
	{
		__SS__ << "Invalid row (" << row << ") col (" << col << ") requested!" << std::endl;
		throw std::runtime_error(ss.str());
	}

	if(columnsInfo_[col].getDataType() == ViewColumnInfo::DATATYPE_STRING)
		theDataView_[row][col] = value;
	else // dont allow ViewColumnInfo::DATATYPE_TIME to be set as string.. force use as time_t to standardize string result
	{

		__SS__ << "\tUnrecognized column data type: " << columnsInfo_[col].getDataType()
								<< " in configuration " << tableName_
								<< " at column=" << columnsInfo_[col].getName()
								<< " for setValue with type '" << ots_demangle(typeid(value).name())
								<< "'" << std::endl;
		throw std::runtime_error(ss.str());
	}
}
void ConfigurationView::setValue(const char *value, unsigned int row, unsigned int col)
{ setValue(std::string(value),row,col); }

//==============================================================================
//setValue
//	string version
void ConfigurationView::setValueAsString(const std::string &value, unsigned int row, unsigned int col)
{
	if(!(col < columnsInfo_.size() && row < getNumberOfRows()))
	{
		__SS__ << "Invalid row (" << row << ") col (" << col << ") requested!" << std::endl;
		throw std::runtime_error(ss.str());
	}

	theDataView_[row][col] = value;
}

//==============================================================================
//getOrInitColUID
//	if column not found throw error
const unsigned int ConfigurationView::getOrInitColUID(void)
{
	if(colUID_ != INVALID) return colUID_;

	//if doesn't exist throw error! each view must have a UID column
	colUID_ = findColByType(ViewColumnInfo::TYPE_UID);
	if(colUID_ == INVALID)
	{
		__COUT__ << "Column Types: " << std::endl;
		for(unsigned int col=0; col<columnsInfo_.size(); ++col)
			std::cout << columnsInfo_[col].getType() << "() " << columnsInfo_[col].getName() << std::endl;
		__SS__ << "\tMissing UID Column in table named '" << tableName_ << "'" << std::endl;
		__COUT_ERR__ << "\n" << ss.str() << std::endl;
		throw std::runtime_error(ss.str());
	}
	return colUID_;
}
//==============================================================================
//getColOfUID
//	const version, so don't attempt to lookup
//	if column not found throw error
const unsigned int ConfigurationView::getColUID(void) const
{
	if(colUID_ != INVALID) return colUID_;

	__COUT__ << "Column Types: " << std::endl;
	for(unsigned int col=0; col<columnsInfo_.size(); ++col)
		std::cout << columnsInfo_[col].getType() << "() " << columnsInfo_[col].getName() << std::endl;

	__SS__ << ("Missing UID Column in config named " + tableName_ +
			". (Possibly ConfigurationView was just not initialized?"  +
			"This is the const call so can not alter class members)") << std::endl;
	__COUT_ERR__ << "\n" << ss.str() << std::endl;
	throw std::runtime_error(ss.str());
}

//==============================================================================
//getOrInitColStatus
//	if column not found throw error
const unsigned int ConfigurationView::getOrInitColStatus(void)
{
	if(colStatus_ != INVALID) return colStatus_;

	//if doesn't exist throw error! each view must have a UID column
	colStatus_ = findCol(ViewColumnInfo::COL_NAME_STATUS);
	if(colStatus_ == INVALID)
	{
		__SS__ << "\tMissing Status Column in table named '" << tableName_ << "'" << std::endl;
		ss << "Column Types: " << std::endl;
		for(unsigned int col=0; col<columnsInfo_.size(); ++col)
			ss << columnsInfo_[col].getType() << "() " << columnsInfo_[col].getName() << std::endl;

		//__COUT_ERR__ << "\n" << ss.str() << std::endl;
		throw std::runtime_error(ss.str());
	}
	return colStatus_;
}
//==============================================================================
//getColStatus
//	const version, so don't attempt to lookup
//	if column not found throw error
const unsigned int ConfigurationView::getColStatus(void) const
{
	if(colStatus_ != INVALID) return colStatus_;

	__COUT__ << "Column Types: " << std::endl;
	for(unsigned int col=0; col<columnsInfo_.size(); ++col)
		std::cout << columnsInfo_[col].getType() << "() " << columnsInfo_[col].getName() << std::endl;

	__SS__ << ("Missing Status Column in config named " + tableName_ +
			". (Possibly ConfigurationView was just not initialized?"  +
			"This is the const call so can not alter class members)") << std::endl;
	__COUT_ERR__ << "\n" << ss.str() << std::endl;
	throw std::runtime_error(ss.str());
}

//==============================================================================
//addRowToGroup
//	Group entry can include | to place a record in multiple groups
void ConfigurationView::addRowToGroup(const unsigned int &row,
		const unsigned int &col,
		const std::string &groupID)//,
		//const std::string &colDefault)
{
	if(isEntryInGroupCol(row,col,groupID))
	{
		__SS__ << "GroupID (" << groupID <<
				") added to row (" << row
				<< " is already present!" << std::endl;
		throw std::runtime_error(ss.str());
	}

	//not in group, so
	//	if no groups
	//		set groupid
	//	if other groups
	//		prepend groupId |
	if(getDataView()[row][col] == "" ||
			getDataView()[row][col] == getDefaultRowValues()[col]) //colDefault)
		setValue(
				groupID,
				row,col);
	else
		setValue(
				groupID + " | " + getDataView()[row][col],
				row,col);

	//__COUT__ << getDataView()[row][col] << std::endl;
}

//==============================================================================
//removeRowFromGroup
//	Group entry can include | to place a record in multiple groups
//
//	returns true if row was deleted because it had no group left
bool ConfigurationView::removeRowFromGroup(const unsigned int &row,
		const unsigned int &col,
		const std::string &groupNeedle,
		bool deleteRowIfNoGroupLeft)
{
	__COUT__ << "groupNeedle " << groupNeedle << std::endl;
	std::set<std::string> groupIDList;
	if(!isEntryInGroupCol(row,col,groupNeedle,&groupIDList))
	{
		__SS__ << "GroupID (" << groupNeedle <<
				") removed from row (" << row
				<< ") was already removed!" << std::endl;
		throw std::runtime_error(ss.str());
	}

	//is in group, so
	//	create new string based on set of groupids
	//	but skip groupNeedle

	std::string newValue = "";
	unsigned int cnt = 0;
	for(const auto & groupID : groupIDList)
	{
		//__COUT__ << groupID << " " << groupNeedle << " " << newValue << std::endl;
		if(groupID == groupNeedle) continue; //skip group to be removed

		if(cnt) newValue += " | ";
		newValue += groupID;
	}

	bool wasDeleted = false;
	if(deleteRowIfNoGroupLeft && newValue == "")
	{
		__COUT__ << "Delete row since it no longer part of any group." << std::endl;
		deleteRow(row);
		wasDeleted = true;
	}
	else
		setValue(newValue,row,col);

	//__COUT__ << getDataView()[row][col] << std::endl;

	return wasDeleted;
}

//==============================================================================
//isEntryInGroup
//	All group link checking should use this function
// 	so that handling is consistent
//
//	Group entry can include | to place a record in multiple groups
bool ConfigurationView::isEntryInGroup(const unsigned int &r,
		const std::string &childLinkIndex,
		const std::string &groupNeedle) const
{
	unsigned int c = getColLinkGroupID(childLinkIndex); //column in question

	return isEntryInGroupCol(r,c,groupNeedle);
}

//==============================================================================
//	isEntryInGroupCol
//		private function to prevent users from treating any column like a GroupID column
//
//	if *groupIDList != 0 return vector of groupIDs found
//		useful for removing groupIDs.
//
//	Group entry can include | to place a record in multiple groups
//
// Note: should mirror what happens in ConfigurationView::getSetOfGroupIDs
bool ConfigurationView::isEntryInGroupCol(const unsigned int &r,
		const unsigned int &c, const std::string &groupNeedle,
		std::set<std::string> *groupIDList) const
{
	unsigned int i=0;
	unsigned int j=0;
	bool found = false;

	//__COUT__ << "groupNeedle " << groupNeedle << std::endl;

	//go through the full groupString extracting groups and comparing to groupNeedle
	for(;j<theDataView_[r][c].size();++j)
		if((theDataView_[r][c][j] == ' ' || //ignore leading white space or |
				theDataView_[r][c][j] == '|')
				&& i == j)
			++i;
		else if((theDataView_[r][c][j] == ' ' || //trailing white space or | indicates group
				theDataView_[r][c][j] == '|')
				&& i != j) // assume end of group name
		{
			if(groupIDList) groupIDList->emplace(theDataView_[r][c].substr(i,j-i));

			//__COUT__ << "Group found to compare: " <<
			//		theDataView_[r][c].substr(i,j-i) << std::endl;
			if(groupNeedle == theDataView_[r][c].substr(i,j-i))
			{
				if(!groupIDList) //dont return if caller is trying to get group list
					return true;
				found = true;
			}
			//if no match, setup i and j for next find
			i = j+1;
		}

	if(i != j) //last group check (for case when no ' ' or '|')
	{
		if(groupIDList) groupIDList->emplace(theDataView_[r][c].substr(i,j-i));

		//__COUT__ << "Group found to compare: " <<
		//		theDataView_[r][c].substr(i,j-i) << std::endl;
		if(groupNeedle == theDataView_[r][c].substr(i,j-i))
			return true;
	}

	return found;
}

//==============================================================================
//getSetOfGroupIDs
//	if row == -1, then considers all rows
//	else just that row
//	returns unique set of groupIds in GroupID column
//		associate with childLinkIndex
//
// Note: should mirror what happens in ConfigurationView::isEntryInGroupCol
std::set<std::string> ConfigurationView::getSetOfGroupIDs(const std::string &childLinkIndex,
		unsigned int r) const
{
	unsigned int c = getColLinkGroupID(childLinkIndex);

	//__COUT__ << "GroupID col=" << (int)c << std::endl;

	std::set<std::string> retSet;

	unsigned int i=0;
	unsigned int j=0;

	if(r != (unsigned int)-1)
	{
		if(r >= getNumberOfRows())
		{
			__SS__ << "Invalid row requested!" << std::endl;
			throw std::runtime_error(ss.str());
		}

		//go through the full groupString extracting groups
		//add each found groupId to set
		for(;j<theDataView_[r][c].size();++j)
			if((theDataView_[r][c][j] == ' ' || //ignore leading white space or |
					theDataView_[r][c][j] == '|')
					&& i == j)
				++i;
			else if((theDataView_[r][c][j] == ' ' || //trailing white space or | indicates group
					theDataView_[r][c][j] == '|')
					&& i != j) // assume end of group name
			{
				//__COUT__ << "Group found: " <<
				//		theDataView_[r][c].substr(i,j-i) << std::endl;


				retSet.emplace(theDataView_[r][c].substr(i,j-i));

				//setup i and j for next find
				i = j+1;
			}

		if(i != j) //last group check (for case when no ' ' or '|')
			retSet.emplace(theDataView_[r][c].substr(i,j-i));
	}
	else
	{
		//do all rows
		for(r=0;r<getNumberOfRows();++r)
		{
			i=0;
			j=0;

			//__COUT__ << (int)r << ": " << theDataView_[r][c] << std::endl;

			//go through the full groupString extracting groups
			//add each found groupId to set
			for(;j<theDataView_[r][c].size();++j)
			{
				//__COUT__ << "i:" << i << " j:" << j << std::endl;

				if((theDataView_[r][c][j] == ' ' || //ignore leading white space or |
						theDataView_[r][c][j] == '|')
						&& i == j)
					++i;
				else if((theDataView_[r][c][j] == ' ' || //trailing white space or | indicates group
						theDataView_[r][c][j] == '|')
						&& i != j) // assume end of group name
				{
					//__COUT__ << "Group found: " <<
					//		theDataView_[r][c].substr(i,j-i) << std::endl;

					retSet.emplace(theDataView_[r][c].substr(i,j-i));

					//setup i and j for next find
					i = j+1;
				}
			}

			if(i != j) //last group (for case when no ' ' or '|')
			{
				//__COUT__ << "Group found: " <<
				//		theDataView_[r][c].substr(i,j-i) << std::endl;
				retSet.emplace(theDataView_[r][c].substr(i,j-i));
			}
		}
	}

	return retSet;
}

//==============================================================================
//getColOfLinkGroupID
//	const version, if column not found throw error
const unsigned int ConfigurationView::getColLinkGroupID(const std::string &childLinkIndex) const
{
	std::map<std::string, unsigned int>::const_iterator it = colLinkGroupIDs_.find(childLinkIndex);
	if(it != //if already known, return it
			colLinkGroupIDs_.end())
		return it->second;

	__COUT__ << "Existing Column Types: " << std::endl;
	for(unsigned int col=0; col<columnsInfo_.size(); ++col)
		std::cout << columnsInfo_[col].getType() << "() " << columnsInfo_[col].getName() << std::endl;

	__SS__ << ("Incompatible table for this group link. Table '" +
			tableName_ + "' is missing a GroupID column with data type '" +
			(ViewColumnInfo::TYPE_START_GROUP_ID + "-" + childLinkIndex) +
			"'." ) << std::endl;
	__COUT_ERR__ << "\n" << ss.str();
	throw std::runtime_error(ss.str());
}

//==============================================================================
unsigned int ConfigurationView::findRow(unsigned int col, const std::string& value,	unsigned int offsetRow) const
{
	for(unsigned int row=offsetRow; row<theDataView_.size(); ++row)
	{
		if(theDataView_[row][col] == value)
			return row;
	}

	__SS__ << "\tIn view: " << tableName_
			<< ", Can't find value=" <<	value
			<< " in column named " << columnsInfo_[col].getName()
			<< " with type=" <<	columnsInfo_[col].getType()
			<< std::endl;
	//Note: findRow gets purposely called by configuration GUI a lot looking for exceptions
	//	so may not want to print out
	//__COUT__ << "\n" << ss.str();
	throw std::runtime_error(ss.str());
}

//==============================================================================
unsigned int ConfigurationView::findRowInGroup(unsigned int col, const std::string& value,
		const std::string &groupId, const std::string &childLinkIndex, unsigned int offsetRow) const
{
	unsigned int groupIdCol = getColLinkGroupID(childLinkIndex);
	for(unsigned int row=offsetRow; row<theDataView_.size(); ++row)
	{
		if(theDataView_[row][col] == value &&
				isEntryInGroupCol(row,groupIdCol,groupId))
			return row;
	}

	__SS__ << "\tIn view: " << tableName_ << ", Can't find in group the value=" <<
			value << " in column named '" <<
			columnsInfo_[col].getName() <<  "' with type=" <<
			columnsInfo_[col].getType() << " and GroupID: '" <<
			groupId << "' in column '" << groupIdCol <<
			"' with GroupID child link index '" << childLinkIndex << "'" << std::endl;
	//Note: findRowInGroup gets purposely called by configuration GUI a lot looking for exceptions
	//	so may not want to print out
	//__COUT__ << "\n" << ss.str();
	throw std::runtime_error(ss.str());
}

//==============================================================================
//findCol
//	throws exception if column not found by name
unsigned int ConfigurationView::findCol(const std::string& name) const
{
	for(unsigned int col=0; col<columnsInfo_.size(); ++col)
		if(columnsInfo_[col].getName() == name)
			return col;

	__SS__ << "\tIn view: " << tableName_ <<
			", Can't find column named '" << name << "'" << std::endl;
	ss << "Existing columns:\n";
	for(unsigned int col=0; col<columnsInfo_.size(); ++col)
		ss << "\t" << columnsInfo_[col].getName() << "\n";
	throw std::runtime_error(ss.str());
}

//==============================================================================
//findColByType
//	return invalid if type not found
unsigned int ConfigurationView::findColByType(const std::string& type, int startingCol) const
{
	for(unsigned int col=startingCol; col<columnsInfo_.size(); ++col)
		if(columnsInfo_[col].getType() == type)
			return col;

	return INVALID;
}

//Getters
//==============================================================================
const std::string& ConfigurationView::getUniqueStorageIdentifier(void) const
{
	return uniqueStorageIdentifier_;
}

//==============================================================================
const std::string& ConfigurationView::getTableName(void) const
{
	return tableName_;
}

//==============================================================================
const ConfigurationVersion& ConfigurationView::getVersion(void) const
{
	return version_;
}

//==============================================================================
const std::string& ConfigurationView::getComment(void) const
{
	return comment_;
}

//==============================================================================
const std::string& ConfigurationView::getAuthor(void) const
{
	return author_;
}

//==============================================================================
const time_t& ConfigurationView::getCreationTime(void) const
{
	return creationTime_;
}

//==============================================================================
const time_t& ConfigurationView::getLastAccessTime(void) const
{
	return lastAccessTime_;
}

//==============================================================================
const bool& ConfigurationView::getLooseColumnMatching(void) const
{
	return fillWithLooseColumnMatching_;
}

//==============================================================================
//getDataColumnSize
const unsigned int ConfigurationView::getDataColumnSize(void) const
{
	//if no data, give benefit of the doubt that phantom data has mockup column size
	if(!getNumberOfRows()) return getNumberOfColumns();
	return theDataView_[0].size(); //number of columns in first row of data
}

//==============================================================================
//getSourceColumnMismatch
//	The source information is only valid after modifying the table with ::fillFromJSON
const unsigned int& ConfigurationView::getSourceColumnMismatch(void) const
{
	return sourceColumnMismatchCount_;
}

//==============================================================================
//getSourceColumnMissing
//	The source information is only valid after modifying the table with ::fillFromJSON
const unsigned int& ConfigurationView::getSourceColumnMissing(void) const
{
	return sourceColumnMissingCount_;
}

//==============================================================================
//getSourceColumnNames
//	The source information is only valid after modifying the table with ::fillFromJSON
const std::set<std::string>& ConfigurationView::getSourceColumnNames(void) const
{
	return sourceColumnNames_;
}

//==============================================================================
std::set<std::string> ConfigurationView::getColumnNames(void) const
{
	std::set<std::string> retSet;
	for(auto &colInfo: columnsInfo_)
		retSet.emplace(colInfo.getName());
	return retSet;
}

//==============================================================================
std::set<std::string> ConfigurationView::getColumnStorageNames(void) const
{
	std::set<std::string> retSet;
	for(auto &colInfo: columnsInfo_)
		retSet.emplace(colInfo.getStorageName());
	return retSet;
}

//==============================================================================
std::vector<std::string> ConfigurationView::getDefaultRowValues(void) const
{
	std::vector<std::string> retVec;

	//fill each col of new row with default values
	for(unsigned int col=0;col<getNumberOfColumns();++col)
	{
		//if this is a fixed choice Link, and NO_LINK is not in list,
		//	take first in list to avoid creating illegal rows.
		//NOTE: this is not a problem for standard fixed choice fields
		//	because the default value is always required.

		if(columnsInfo_[col].isChildLink())
		{
			const std::vector<std::string>& theDataChoices =
					columnsInfo_[col].getDataChoices();

			//check if arbitrary values allowed
			if(!theDataChoices.size() || //if so, use default
					theDataChoices[0] == "arbitraryBool=1")
				retVec.push_back(columnsInfo_[col].getDefaultValue());
			else
			{
				bool skipOne = (theDataChoices.size() &&
						theDataChoices[0] == "arbitraryBool=0");
				bool hasSkipped;

				//look for default value in list

				bool foundDefault = false;
				hasSkipped = false;
				for(const auto &choice:theDataChoices)
					if(skipOne && !hasSkipped) {hasSkipped = true; continue;}
					else if(choice == columnsInfo_[col].getDefaultValue())
					{
						foundDefault = true;
						break;
					}

				//use first choice if possible
				if(!foundDefault && theDataChoices.size() > (skipOne?1:0))
					retVec.push_back(theDataChoices[(skipOne?1:0)]);
				else //else stick with default
					retVec.push_back(columnsInfo_[col].getDefaultValue());
			}
		}
		else
			retVec.push_back(columnsInfo_[col].getDefaultValue());
	}

	return retVec;
}

//==============================================================================
unsigned int ConfigurationView::getNumberOfRows(void) const
{
	return theDataView_.size();
}

//==============================================================================
unsigned int ConfigurationView::getNumberOfColumns(void) const
{
	return columnsInfo_.size();
}

//==============================================================================
const ConfigurationView::DataView& ConfigurationView::getDataView(void) const
{
	return theDataView_;
}

//==============================================================================
//ConfigurationView::DataView* ConfigurationView::getDataViewP(void)
//{
//	return &theDataView_;
//}

//==============================================================================
const std::vector<ViewColumnInfo>& ConfigurationView::getColumnsInfo (void) const
{
	return columnsInfo_;
}

//==============================================================================
std::vector<ViewColumnInfo>* ConfigurationView::getColumnsInfoP(void)
{
	return &columnsInfo_;
}
//==============================================================================
const ViewColumnInfo& ConfigurationView::getColumnInfo(unsigned int column) const
{
	if(column >= columnsInfo_.size())
	{
		std::stringstream errMsg;
		errMsg << __COUT_HDR_FL__ << "\nCan't find column " << column <<
				"\n\n\n\nThe column info is likely missing due to incomplete Configuration View filling.\n\n"
				<< std::endl;
		throw std::runtime_error(errMsg.str().c_str());
	}
	return columnsInfo_[column];
}

//Setters
//==============================================================================
void ConfigurationView::setUniqueStorageIdentifier(const std::string &storageUID)
{
	uniqueStorageIdentifier_ = storageUID;
}

//==============================================================================
void ConfigurationView::setTableName(const std::string &name)
{
	tableName_ = name;
}

//==============================================================================
void ConfigurationView::setComment(const std::string &comment)
{
	comment_ = comment;
}

//==============================================================================
void ConfigurationView::setURIEncodedComment(const std::string &uriComment)
{
	comment_ = decodeURIComponent(uriComment);
}

//==============================================================================
void ConfigurationView::setAuthor(const std::string &author)
{
	author_ = author;
}

//==============================================================================
void ConfigurationView::setCreationTime(time_t t)
{
	creationTime_ = t;
}

//==============================================================================
void ConfigurationView::setLastAccessTime(time_t t)
{
	lastAccessTime_ = t;
}

//==============================================================================
void ConfigurationView::setLooseColumnMatching(bool setValue)
{
	fillWithLooseColumnMatching_ = setValue;
}

//==============================================================================
void ConfigurationView::reset (void)
{
	version_ = -1;
	comment_ = "";
	author_  + "";
	columnsInfo_.clear();
	theDataView_.clear();
}

//==============================================================================
void ConfigurationView::print (std::ostream &out) const
{
	out << "==============================================================================" << std::endl;
	out << "Print: " << tableName_ << " Version: " << version_ << " Comment: " << comment_ <<
			" Author: " << author_ << " Creation Time: " << ctime(&creationTime_) << std::endl;
	out << "\t\tNumber of Cols " << getNumberOfColumns() << std::endl;
	out << "\t\tNumber of Rows " << getNumberOfRows() << std::endl;

	out << "Columns:\t";
	for(int i=0;i<(int)columnsInfo_.size();++i)
		out << i << ":" << columnsInfo_[i].getName() << ":" << columnsInfo_[i].getStorageName() << ":" << columnsInfo_[i].getDataType() << "\t ";
	out << std::endl;

	out << "Rows:" << std::endl;
	int num;
	std::string val;
	for(int r=0;r<(int)getNumberOfRows();++r)
	{
		out << (int)r << ":\t";
		for(int c=0;c<(int)getNumberOfColumns();++c)
		{
			out << (int)c << ":";

			//if fixed choice type, print index in choice
			if(columnsInfo_[c].getType() == ViewColumnInfo::TYPE_FIXED_CHOICE_DATA)
			{
				int choiceIndex = -1;
				std::vector<std::string> choices = columnsInfo_[c].getDataChoices();
				val = convertEnvVariables(theDataView_[r][c]);

				if(val == columnsInfo_[c].getDefaultValue())
					choiceIndex = 0;
				else
				{
					for(int i=0;i<(int)choices.size();++i)
						if(val == choices[i])
							choiceIndex = i+1;
				}

				out << "ChoiceIndex=" << choiceIndex << ":";
			}


			out << theDataView_[r][c];
			//stopped using below, because it is called sometimes during debugging when
			//	numbers are set to environment variables:
			//			if(columnsInfo_[c].getDataType() == "NUMBER")
			//			{
			//				getValue(num,r,c,false);
			//				out << num;
			//			}
			//			else
			//			{
			//				getValue(val,r,c,false);
			//				out << val;
			//			}
			out << "\t\t";
		}
		out << std::endl;
	}
}


//==============================================================================
void ConfigurationView::printJSON (std::ostream &out) const
{
	out << "{\n";
	out << "\"NAME\" : \"" << tableName_ << "\",\n";

	//out << "\"VERSION\" : \"" << version_ <<  "\"\n";

	out << "\"COMMENT\" : ";

	//output escaped comment
	std::string val;
	val = comment_;
	out << "\"";
	for(unsigned int i=0;i<val.size();++i)
	{

		if(val[i] == '\n')
			out << "\\n";
		else if(val[i] == '\t')
			out << "\\t";
		else if(val[i] == '\r')
			out << "\\r";
		else
		{
			//escaped characters need a
			if(val[i] == '"' ||
					val[i] == '\\')
				out << '\\';
			out << val[i];
		}
	}
	out << "\",\n";

	out << "\"AUTHOR\" : \"" << author_ << "\",\n";
	out << "\"CREATION_TIME\" : " << creationTime_ << ",\n";

	//USELESS... out << "\"NUM_OF_COLS\" : " << getNumberOfColumns() << ",\n";
	//USELESS... out << "\"NUM_OF_ROWS\" : " <<  getNumberOfRows() << ",\n";



	out << "\"COL_TYPES\" : {\n";
	for(int c=0;c<(int)getNumberOfColumns();++c)
	{
		out << "\t\t\"" << columnsInfo_[c].getStorageName() << "\" : ";
		out << "\"" << columnsInfo_[c].getDataType() << "\"";
		if(c+1 < (int)getNumberOfColumns())
			out << ",";
		out << "\n";
	}
	out << "},\n"; //close COL_TYPES

	out << "\"DATA_SET\" : [\n";
	int num;
	for(int r=0;r<(int)getNumberOfRows();++r)
	{
		out << "\t{\n";
		for(int c=0;c<(int)getNumberOfColumns();++c)
		{
			out << "\t\t\"" << columnsInfo_[c].getStorageName() << "\" : ";

			out << "\"" << getEscapedValueAsString(r,c,false) << "\""; //do not convert env variables

			if(c+1 < (int)getNumberOfColumns())
				out << ",";
			out << "\n";
		}
		out << "\t}";
		if(r+1 < (int)getNumberOfRows())
			out << ",";
		out << "\n";
	}
	out << "]\n"; //close DATA_SET

	out << "}";
}

//==============================================================================
//restoreJSONStringEntities
//	returns string with literals \n \t \" \r \\ replaced with char
std::string restoreJSONStringEntities(const std::string &str)
{
	unsigned int sz = str.size();
	if(!sz) return ""; //empty string, returns empty string

	std::stringstream retStr;
	unsigned int i=0;
	for(;i<sz-1;++i)
	{
		if(str[i] == '\\') //if 2 char escape sequence, replace with char
			switch(str[i+1])
			{
			case 'n':
				retStr << '\n'; ++i; break;
			case '"':
				retStr << '"';  ++i; break;
			case 't':
				retStr << '\t'; ++i; break;
			case 'r':
				retStr << '\r'; ++i; break;
			case '\\':
				retStr << '\\'; ++i; break;
			default:
				retStr << str[i];
			}
		else
			retStr << str[i];
	}
	if(i == sz-1)
		retStr << str[sz-1]; //output last character (which can't escape anything)

	return retStr.str();
}

//==============================================================================
//fillFromJSON
//	Clears and fills the view from the JSON string.
//	Returns -1 on failure
//
//	first level keys:
//		NAME
//		DATA_SET

int ConfigurationView::fillFromJSON(const std::string &json)
{
	std::vector<std::string> keys;
	keys.push_back ("NAME");
	keys.push_back ("COMMENT");
	keys.push_back ("AUTHOR");
	keys.push_back ("CREATION_TIME");
	//keys.push_back ("COL_TYPES");
	keys.push_back ("DATA_SET");
	enum {
		CV_JSON_FILL_NAME,
		CV_JSON_FILL_COMMENT,
		CV_JSON_FILL_AUTHOR,
		CV_JSON_FILL_CREATION_TIME,
		//	CV_JSON_FILL_COL_TYPES,
		CV_JSON_FILL_DATA_SET
	};

	//__COUT__ << json << std::endl;

	sourceColumnMismatchCount_ = 0;
	sourceColumnMissingCount_ = 0;
	sourceColumnNames_.clear(); //reset
	unsigned int colFoundCount = 0;
	unsigned int i = 0;
	unsigned int row = -1;
	unsigned int colSpeedup = 0;
	unsigned int startString, startNumber, endNumber = -1;
	unsigned int bracketCount = 0;
	unsigned int sqBracketCount = 0;
	bool inQuotes = 0;
	bool newString = 0;
	bool newValue = 0;
	bool isDataArray = 0;
	bool keyIsMatch, keyIsComment;
	unsigned int keyIsMatchIndex, keyIsMatchStorageIndex, keyIsMatchCommentIndex;
	const std::string COMMENT_ALT_KEY = "COMMENT";

	std::string extractedString = "", currKey = "", currVal = "";
	unsigned int currDepth;

	std::vector<std::string> jsonPath;
	std::vector<char> jsonPathType; // indicator of type in jsonPath: { [ K
	char lastPopType = '_';	// either: _ { [ K
	// _ indicates reset pop (this happens when a new {obj} starts)
	unsigned int matchedKey = -1;
	unsigned int lastCol = -1;


	//find all depth 1 matching keys
	for(;i<json.size();++i)
	{
		switch(json[i])
		{
		case '"':
			if(i-1 < json.size() && //ignore if escaped
					json[i-1] == '\\') break;

			inQuotes = !inQuotes; //toggle in quotes if not escaped
			if(inQuotes)
				startString = i;
			else
			{
				extractedString = restoreJSONStringEntities(
						json.substr(startString+1,i-startString-1));
				newString = 1; //have new string!
			}
			break;
		case ':':
			if(inQuotes) break; //skip if in quote

			//must be a json object level to have a key
			if(jsonPathType[jsonPathType.size()-1] != '{'
					|| !newString) //and must have a string for key
			{
				__COUT__ << "Invalid ':' position" << std::endl;
				return -1;
			}

			//valid, so take key
			jsonPathType.push_back('K');
			jsonPath.push_back(extractedString);
			startNumber = i;
			newString = 0; //clear flag
			endNumber = -1; //reset end number index
			break;

			//    		if(isKey ||
			//    			isDataArray)
			//    		{
			//            	std::cout << "Invalid ':' position" << std::endl;
			//            	return -1;
			//    		}
			//    		isKey = 1;	//new value is a key
			//    		newValue = 1;
			//    		startNumber = i;
			//    		break;
		case ',':
			if(inQuotes) break; //skip if in quote
			if(lastPopType == '{')  //don't need value again of nested object
			{
				//check if the nested object was the value to a key, if so, pop key
				if(jsonPathType[jsonPathType.size()-1] == 'K')
				{
					lastPopType = 'K';
					jsonPath.pop_back();
					jsonPathType.pop_back();
				}
				break; //skip , handling if {obj} just ended
			}

			if(newString)
				currVal = extractedString;
			else //number value
			{
				if(endNumber == (unsigned int)-1 || //take i as end number if needed
						endNumber <= startNumber)
					endNumber = i;
				//extract number value
				if(endNumber <= startNumber) //empty data, could be {}
					currVal = "";
				else
					currVal = json.substr(startNumber+1,endNumber-startNumber-1);
			}

			currDepth = bracketCount;

			if(jsonPathType[jsonPathType.size()-1] == 'K') //this is the value to key
			{
				currKey = jsonPath[jsonPathType.size()-1];
				newValue = 1; //new value to consider!

				//pop key
				lastPopType = 'K';
				jsonPath.pop_back();
				jsonPathType.pop_back();
			}
			else if(jsonPathType[jsonPathType.size()-1] == '[') //this is a value in array
			{
				//key is last key
				for(unsigned int k=jsonPathType.size()-2; k<jsonPathType.size(); --k)
					if(jsonPathType[k] == 'K')
					{
						currKey = jsonPath[k];
						break;
					}
					else if(k == 0)
					{
						__COUT__ << "Invalid array position" << std::endl;
						return -1;
					}

				newValue = 1; //new value to consider!
				isDataArray = 1;
			}
			else // { is an error
			{
				__COUT__ << "Invalid ',' position" << std::endl;
				return -1;
			}

			startNumber = i;
			break;

		case '{':
			if(inQuotes) break; //skip if in quote
			lastPopType = '_'; //reset because of new object
			jsonPathType.push_back('{');
			jsonPath.push_back("{");
			++bracketCount;
			break;

			//    		++bracketCount;
			//    		isDataArray = 0;
			//    		isKey = 0;
			//    		endingObject = 0;
			//    		break;
		case '}':
			if(inQuotes) break; //skip if in quote

			if(lastPopType != '{' && //don't need value again of nested object
					jsonPathType[jsonPathType.size()-1] == 'K') //this is the value to key
			{
				currDepth = bracketCount;
				currKey = jsonPath[jsonPathType.size()-1];
				if(newString)
					currVal = extractedString;
				else //number value
				{
					if(endNumber == (unsigned int)-1 || //take i as end number if needed
							endNumber <= startNumber)
						endNumber = i;
					//extract val
					if(endNumber <= startNumber) //empty data, could be {}
						currVal = "";
					else
						currVal = json.substr(startNumber+1,endNumber-startNumber-1);
				}
				newValue = 1; //new value to consider!
				//pop key
				jsonPath.pop_back();
				jsonPathType.pop_back();
			}
			//pop {
			if(jsonPathType[jsonPathType.size()-1] != '{')
			{
				__COUT__ << "Invalid '}' position" << std::endl;
				return -1;
			}
			lastPopType = '{';
			jsonPath.pop_back();
			jsonPathType.pop_back();
			--bracketCount;
			break;
		case '[':
			if(inQuotes) break; //skip if in quote
			jsonPathType.push_back('[');
			jsonPath.push_back("[");
			++sqBracketCount;
			startNumber = i;
			break;
		case ']':
			if(inQuotes) break; //skip if in quote

			//must be an array at this level (in order to close it)
			if(jsonPathType[jsonPathType.size()-1] != '[')
			{
				__COUT__ << "Invalid ']' position" << std::endl;
				return -1;
			}

			currDepth = bracketCount;

			//This is an array value
			if(newString)
				currVal = extractedString;
			else //number value
			{
				if(endNumber == (unsigned int)-1 || //take i as end number if needed
						endNumber <= startNumber)
					endNumber = i;
				//extract val
				if(endNumber <= startNumber) //empty data, could be {}
					currVal = "";
				else
					currVal = json.substr(startNumber+1,endNumber-startNumber-1);
			}
			isDataArray = 1;

			//key is last key
			for(unsigned int k=jsonPathType.size()-2; k<jsonPathType.size(); --k)
				if(jsonPathType[k] == 'K')
				{
					currKey = jsonPath[k];
					break;
				}
				else if(k == 0)
				{
					__COUT__ << "Invalid array position" << std::endl;
					return -1;
				}

			//pop [
			if(jsonPathType[jsonPathType.size()-1] != '[')
			{
				__COUT__ << "Invalid ']' position" << std::endl;
				return -1;
			}
			lastPopType = '[';
			jsonPath.pop_back();
			jsonPathType.pop_back();
			--sqBracketCount;
			break;
		case ' ':		//white space handling for numbers
		case '\t':
		case '\n':
		case '\r':
			if(inQuotes) break; //skip if in quote
			if(startNumber != (unsigned int)-1 &&
					endNumber == (unsigned int)-1)
				endNumber = i;
			startNumber = i;
			break;
		default:;
		}

		if(0) //for debugging
		{
			std::cout << i << ":\t" << json[i] << " - ";

			std::cout << "ExtKey=";
			for(unsigned int k=0;k<jsonPath.size();++k)
				std::cout << jsonPath[k] << "/";
			std::cout << " - ";
			std::cout << lastPopType << " ";
			std::cout << bracketCount << " ";
			std::cout << sqBracketCount << " ";
			std::cout << inQuotes << " ";
			std::cout << newValue << "-";
			std::cout << currKey << "-{" << currDepth << "}:" ;
			std::cout << currVal << " ";
			std::cout << startNumber << "-";
			std::cout << endNumber << " ";
			std::cout << "\n";
		}

		//continue;

		//handle a new completed value
		if(newValue)
		{

			//			std::cout << "ExtKey=";
			//			for(unsigned int k=0;k<jsonPath.size();++k)
			//	        	 std::cout << jsonPath[k] << "/";
			//
			//			std::cout << currKey << "-{" << currDepth << "}: - ";
			//
			//    		if(isDataArray)
			//    			std::cout << "Array:: ";
			//    		if(newString)
			//				std::cout << "New String:: ";
			//    		else
			//				std::cout << "New Number:: ";
			//
			//    		std::cout << currVal << "\n";


			//extract only what we care about
			// for ConfigurationView only care about matching depth 1

			//handle matching depth 1 keys

			matchedKey = -1; //init to unfound
			for(unsigned int k=0;k<keys.size();++k)
				if((currDepth == 1 && keys[k] == currKey) ||
						(currDepth > 1 && keys[k] == jsonPath[1]))
					matchedKey = k;

			if(matchedKey != (unsigned int)-1)
			{
				//				std::cout << "New Data for:: key[" << matchedKey << "]-" <<
				//						keys[matchedKey] << "\n";

				switch(matchedKey)
				{
				case CV_JSON_FILL_NAME:
					if(currDepth == 1) setTableName(currVal);
					break;
				case CV_JSON_FILL_COMMENT:
					if(currDepth == 1) setComment(currVal);
					break;
				case CV_JSON_FILL_AUTHOR:
					if(currDepth == 1) setAuthor(currVal);
					break;
				case CV_JSON_FILL_CREATION_TIME:
					if(currDepth == 1) setCreationTime(strtol(currVal.c_str(),0,10));
					break;
					//case CV_JSON_FILL_COL_TYPES:
					//
					//break;
				case CV_JSON_FILL_DATA_SET:
					//					std::cout << "CV_JSON_FILL_DATA_SET New Data for:: " << matchedKey << "]-" <<
					//						keys[matchedKey] << "/" << currDepth << ".../" <<
					//						currKey << "\n";

					if(currDepth == 2) //second level depth
					{
						//if matches first column name.. then add new row
						//else add to current row
						unsigned int col, ccnt = 0;
						unsigned int noc = getNumberOfColumns();
						for(;ccnt<noc;++ccnt)
						{
							//use colSpeedup to change the first column we search
							//for each iteration.. since we expect the data to
							//be arranged in column order

							if(fillWithLooseColumnMatching_)
							{
								//loose column matching makes no attempt to
								// match the column names
								//just assumes the data is in the correct order

								col = colSpeedup;

								//auto matched
								if(col <= lastCol) //add row (use lastCol in case new column-0 was added
									row = addRow();
								lastCol = col;
								if(getNumberOfRows() == 1) //only for first row
									sourceColumnNames_.emplace(currKey);

								//add value to row and column

								if(row >= getNumberOfRows())
								{
									__SS__ << "Invalid row" << std::endl; //should be impossible?
									std::cout << ss.str();
									throw std::runtime_error(ss.str());
									return -1;
								}

								theDataView_[row][col] = currVal;//THERE IS NO CHECK FOR WHAT IS READ FROM THE DATABASE. IT SHOULD BE ALREADY CONSISTENT
								break;
							}
							else
							{
								col = (ccnt + colSpeedup) % noc;

								//match key by ignoring '_'
								//	also accept COMMENT == COMMENT_DESCRIPTION
								//	(this is for backwards compatibility..)
								keyIsMatch = true;
								keyIsComment = true;
								for(keyIsMatchIndex=0, keyIsMatchStorageIndex=0, keyIsMatchCommentIndex=0;
										keyIsMatchIndex<currKey.size();++keyIsMatchIndex)
								{
									if(columnsInfo_[col].getStorageName()[keyIsMatchStorageIndex] == '_')
										++keyIsMatchStorageIndex; 	//skip to next storage character
									if(currKey[keyIsMatchIndex] == '_')
										continue;  					//skip to next character

									//match to storage name
									if(keyIsMatchStorageIndex >= columnsInfo_[col].getStorageName().size() ||
											currKey[keyIsMatchIndex] !=
													columnsInfo_[col].getStorageName()[keyIsMatchStorageIndex])
									{
										//size mismatch or character mismatch
										keyIsMatch = false;
										if(!keyIsComment)
											break;
									}

									//check also if alternate comment is matched
									if(keyIsComment && keyIsMatchCommentIndex < COMMENT_ALT_KEY.size())
									{
										if(currKey[keyIsMatchIndex] != COMMENT_ALT_KEY[keyIsMatchCommentIndex])
										{
											//character mismatch with COMMENT
											keyIsComment = false;
										}
									}

									++keyIsMatchStorageIndex; //go to next character
								}

								if(keyIsMatch || keyIsComment) //currKey == columnsInfo_[c].getStorageName())
								{
									//matched
									if(col <= lastCol) //add row (use lastCol in case new column-0 was added
									{
										if(getNumberOfRows()) //skip first time
											sourceColumnMissingCount_ += getNumberOfColumns() - colFoundCount;

										colFoundCount = 0; //reset column found count
										row = addRow();
									}
									lastCol = col;
									++colFoundCount;

									if(getNumberOfRows() == 1) //only for first row
										sourceColumnNames_.emplace(currKey);

									//add value to row and column

									if(row >= getNumberOfRows())
									{
										__SS__ << "Invalid row" << std::endl; //should be impossible?!
										__COUT__ << "\n" << ss.str();
										throw std::runtime_error(ss.str());
										return -1; //never gets here
									}

									theDataView_[row][col] = currVal;
									break;
								}
							}
						}

						colSpeedup = (colSpeedup + 1) % noc; //short cut to proper column hopefully in next search

						if(ccnt >= getNumberOfColumns())
						{
							__SS__ << "\n\nInvalid column in JSON source data: " <<
									currKey << " not found in column names of table named " <<
									getTableName() << "." <<
									std::endl; //input data doesn't match config description
							__COUT__ << "\n" << ss.str();
							//CHANGED on 11/10/2016
							//	to.. try just not populating data instead of error
							++sourceColumnMismatchCount_; //but count errors
							if(getNumberOfRows() == 1) //only for first row
								sourceColumnNames_.emplace(currKey);

							//throw std::runtime_error(ss.str());
						}

					}
					break;
				default:; //unknown match?
				}

			}

			//clean up handling of new value

			newString = 0; //toggle flag
			newValue = 0; //toggle flag
			isDataArray = 0;
			endNumber = -1; //reset end number index
		}

		//if(i>200) break; //185
	}

	//__COUT__ << "Done!" << std::endl;

	//print();

	return 0; //success
}



//==============================================================================
bool ConfigurationView::isURIEncodedCommentTheSame(const std::string &comment) const
{
	std::string compareStr = decodeURIComponent(comment);
	return comment_ == compareStr;
}
//
////==============================================================================
//bool ConfigurationView::isValueTheSame(const std::string &valueStr,
//		unsigned int r, unsigned int c) const
//{
//	__COUT__ << "valueStr " << valueStr << std::endl;
//
//	if(!(c < columnsInfo_.size() && r < getNumberOfRows()))
//	{
//		__SS__ << "Invalid row (" << (int)r << ") col (" << (int)c << ") requested!" << std::endl;
//		throw std::runtime_error(ss.str());
//	}
//
//	__COUT__ << "originalValueStr " << theDataView_[r][c] << std::endl;
//
//	if(columnsInfo_[c].getDataType() == ViewColumnInfo::DATATYPE_TIME)
//	{
//		time_t valueTime(strtol(valueStr.c_str(),0,10));
//		time_t originalValueTime;
//		getValue(originalValueTime,r,c);
//		__COUT__ << "time_t valueStr " << valueTime << std::endl;
//		__COUT__ << "time_t originalValueStr " << originalValueTime << std::endl;
//		return valueTime == originalValueTime;
//	}
//	else
//	{
//		return valueStr == theDataView_[r][c];
//	}
//}

//==============================================================================
//fillFromCSV
//	Fills the view from the CSV string.
//
//	Note: converts all %## to the ascii character, # is hex nibble
// (e.g. '%' must be represented as "%25")
//
//	dataOffset := starting destination row
//
//	while there are row entries in the data.. replace
// data range from [dataOffset, dataOffset+chunkSize-1]
// ... note if less rows, this means rows were deleted
// ... if more, then rows were added.
//
//	',' next cell delimiter
//  ';' next row delimiter
//
//
//	if author == "", do nothing special for author and timestamp column
//	if author != "", assign author for any row that has been modified, and assign now as timestamp
//
//	Returns -1 if data was same and pre-existing content
//	otherwise 0
int ConfigurationView::fillFromCSV(const std::string &data, const int &dataOffset,
		const std::string &author)
throw(std::runtime_error)
{
	int r = dataOffset;
	int c = 0;

	int i = 0; //use to parse data std::string
	int j = data.find(',',i); //find next cell delimiter
	int k = data.find(';',i); //find next row delimiter

	bool rowWasModified;
	unsigned int countRowsModified = 0;
	int authorCol = findColByType(ViewColumnInfo::TYPE_AUTHOR);
	int timestampCol = findColByType(ViewColumnInfo::TYPE_TIMESTAMP);
	//std::string valueStr, tmpTimeStr, originalValueStr;

	while(k != (int)(std::string::npos))
	{
		rowWasModified = false;
		if(r >= (int)getNumberOfRows())
		{
			addRow();
			//__COUT__ << "Row added" << std::endl;
			rowWasModified = true;
		}

		while(j < k && j != (int)(std::string::npos))
		{
			//__COUT__ << "Col " << (int)c << std::endl;

			//skip last 2 columns
			if(c >= (int)getNumberOfColumns()-2)
			{
				i=j+1;
				j = data.find(',',i); //find next cell delimiter
				++c;
				continue;
			}

			if(setURIEncodedValue(data.substr(i,j-i),r,c))
				rowWasModified = true;

			i=j+1;
			j = data.find(',',i); //find next cell delimiter
			++c;
		}

		//if row was modified, assign author and timestamp
		if(author != "" && rowWasModified)
		{
			__COUT__ << "Row=" << (int)r << " was modified!" << std::endl;
			setValue(author,r,authorCol);
			setValue(time(0),r,timestampCol);
		}

		if(rowWasModified) ++countRowsModified;

		++r;
		c = 0;


		i = k+1;
		j = data.find(',',i); //find next cell delimiter
		k = data.find(';',i); //find new row delimiter
	}

	//delete excess rows
	while(r < (int)getNumberOfRows())
	{
		deleteRow(r);
		__COUT__ << "Row deleted: " << (int)r << std::endl;
		++countRowsModified;
	}

	__COUT_INFO__ << "countRowsModified=" <<
			countRowsModified << std::endl;

	if(!countRowsModified)
	{
		__SS__ << "No rows were modified! No reason to fill a view with same content." << std::endl;
		__COUT__ << "\n" << ss.str();
		return -1;
	}

	//print(); //for debugging

	//setup sourceColumnNames_ to be correct
	sourceColumnNames_.clear();
	for(unsigned int i=0;i<getNumberOfColumns();++i)
		sourceColumnNames_.emplace(getColumnsInfo()[i].getStorageName());

	init(); //verify new table (throws runtime_errors)

	//printout for debugging
	//	__SS__ << "\n";
	//	print(ss);
	//	__COUT__ << "\n" << ss.str() << std::endl;

	return 0;
}

//==============================================================================
//setURIEncodedValue
//	converts all %## to the ascii character
//	returns true if value was different than original value
//
//
//	if author == "", do nothing special for author and timestamp column
//	if author != "", assign author for any row that has been modified, and assign now as timestamp
bool ConfigurationView::setURIEncodedValue(const std::string &value, const unsigned int &r,
		const unsigned int &c, const std::string &author)
{
	if(!(c < columnsInfo_.size() && r < getNumberOfRows()))
	{
		__SS__ << "Invalid row (" << (int)r << ") col (" << (int)c << ") requested!" <<
				"Number of Rows = " << getNumberOfRows() <<
				"Number of Columns = " << columnsInfo_.size() << std::endl;
		print(ss);
		throw std::runtime_error(ss.str());
	}

	std::string valueStr = decodeURIComponent(value);
	std::string originalValueStr = getValueAsString(r,c,false); //do not convert env variables


	//__COUT__ << "valueStr " << valueStr << std::endl;
	//__COUT__ << "originalValueStr " << originalValueStr << std::endl;

	if(columnsInfo_[c].getDataType() == ViewColumnInfo::DATATYPE_NUMBER)
	{
		//check if valid number
		std::string convertedString = convertEnvVariables(valueStr);
		if(!isNumber(convertedString))
		{
			__SS__ << "\tIn configuration " << tableName_
					<< " at column=" << columnsInfo_[c].getName()
					<< " the value set (" << convertedString << ")"
					<< " is not a number! Please fix it or change the column type..." << std::endl;
			throw std::runtime_error(ss.str());
		}
		theDataView_[r][c] = valueStr;
	}
	else if(columnsInfo_[c].getDataType() == ViewColumnInfo::DATATYPE_TIME)
	{
		//				valueStr = decodeURIComponent(data.substr(i,j-i));
		//
		//				getValue(tmpTimeStr,r,c);
		//				if(valueStr != tmpTimeStr)//theDataView_[r][c])
		//				{
		//					__COUT__ << "valueStr=" << valueStr <<
		//							" theDataView_[r][c]=" << tmpTimeStr << std::endl;
		//					rowWasModified = true;
		//				}

		setValue(time_t(strtol(valueStr.c_str(),0,10)),
				r,c);
	}
	else
		theDataView_[r][c] = valueStr;

	bool rowWasModified = (originalValueStr != getValueAsString(r,c,false));  //do not convert env variables

	//if row was modified, assign author and timestamp
	if(author != "" && rowWasModified)
	{
		__COUT__ << "Row=" << (int)r << " was modified!" << std::endl;
		int authorCol = findColByType(ViewColumnInfo::TYPE_AUTHOR);
		int timestampCol = findColByType(ViewColumnInfo::TYPE_TIMESTAMP);
		setValue(author,r,authorCol);
		setValue(time(0),r,timestampCol);
	}

	return rowWasModified;
}

//==============================================================================
//decodeURIComponent
//	converts all %## to the ascii character
std::string ConfigurationView::decodeURIComponent(const std::string &data)
{
	std::string decodeURIString(data.size(),0); //init to same size
	unsigned int j=0;
	for(unsigned int i=0;i<data.size();++i,++j)
	{
		if(data[i] == '%')
		{
			//high order hex nibble digit
			if(data[i+1] > '9') //then ABCDEF
				decodeURIString[j] += (data[i+1]-55)*16;
			else
				decodeURIString[j] += (data[i+1]-48)*16;

			//low order hex nibble digit
			if(data[i+2] > '9')	//then ABCDEF
				decodeURIString[j] += (data[i+2]-55);
			else
				decodeURIString[j] += (data[i+2]-48);

			i+=2; //skip to next char
		}
		else
			decodeURIString[j] = data[i];
	}
	decodeURIString.resize(j);
	return decodeURIString;
}

//==============================================================================
void ConfigurationView::resizeDataView(unsigned int nRows, unsigned int nCols)
{
	//FIXME This maybe should disappear but I am using it in ConfigurationHandler still...
	theDataView_.resize(nRows, std::vector<std::string>(nCols));
}

//==============================================================================
//addRow
//	returns index of added row, always is last row
//	return -1 on failure (throw error)
//
//	if baseNameAutoUID != "", creates a UID based on this base name
//		and increments and appends an integer relative to the previous last row
int ConfigurationView::addRow(const std::string &author,
		bool incrementUniqueData,
		std::string baseNameAutoUID)
{
	int row = getNumberOfRows();
	theDataView_.resize(getNumberOfRows()+1,std::vector<std::string>(getNumberOfColumns()));

	std::vector<std::string> defaultRowValues =
			getDefaultRowValues();

	char indexString[1000];
	std::string tmpString, baseString;
	bool foundAny;
	unsigned int index;
	unsigned int maxUniqueData;
	std::string numString;

	//fill each col of new row with default values
	//	if a row is a unique data row, increment last row in attempt to make a legal column
	for(unsigned int col=0;col<getNumberOfColumns();++col)
	{
		//__COUT__ << col << " " << columnsInfo_[col].getType() << " == " <<
		//		ViewColumnInfo::TYPE_UNIQUE_DATA << __E__;

		//baseNameAutoUID indicates to attempt to make row unique
		//	add index to max number
		if(incrementUniqueData &&
				(col == getColUID() || (row && columnsInfo_[col].getType() ==
				ViewColumnInfo::TYPE_UNIQUE_DATA)))
		{

			//__COUT__ << "New unique data entry is '" << theDataView_[row][col] << "'" << __E__;

			maxUniqueData = 0;
			tmpString = "";
			baseString = "";

			//find max in rows

			this->print();

			for(unsigned int r=0;r<getNumberOfRows()-1;++r)
			{
				//find last non numeric character

				foundAny = false;
				tmpString = theDataView_[r][col];

				//__COUT__ << "tmpString " << tmpString << __E__;

				for(index = tmpString.length()-1;index < tmpString.length(); --index)
				{
					__COUT__ << index << " tmpString[index] " << tmpString[index] << __E__;
					if(!(tmpString[index] >= '0' && tmpString[index] <= '9')) break; //if not numeric, break
					foundAny = true;
				}

				//__COUT__ << "index " << index << __E__;

				if(tmpString.length() &&
						foundAny) //then found a numeric substring
				{
					//create numeric substring
					numString = tmpString.substr(index+1);
					tmpString = tmpString.substr(0,index+1);

					//__COUT__ << "Found unique data base string '" <<
					//		tmpString << "' and number string '" << numString <<
					//		"' in last record '" << theDataView_[r][col] << "'" << __E__;

					//extract number
					sscanf(numString.c_str(),"%u",&index);

					if(index > maxUniqueData)
					{
						maxUniqueData = index;
						baseString = tmpString;
					}
				}
			}

			++maxUniqueData; //increment

			sprintf(indexString,"%u",maxUniqueData);
			//__COUT__ << "indexString " << indexString << __E__;

			//__COUT__ << "baseNameAutoUID " << baseNameAutoUID << __E__;
			if(col == getColUID())
			{
				//handle UID case
				if(baseNameAutoUID != "")
					theDataView_[row][col] = baseNameAutoUID + indexString;
				else
					theDataView_[row][col] = baseString + indexString;
			}
			else
				theDataView_[row][col] = baseString + indexString;

			__COUT__ << "New unique data entry is '" << theDataView_[row][col] << "'" << __E__;

			this->print();
		}
		else
			theDataView_[row][col] = defaultRowValues[col];
	}

	if(author != "")
	{
		__COUT__ << "Row=" << row << " was created!" << std::endl;
		int authorCol = findColByType(ViewColumnInfo::TYPE_AUTHOR);
		int timestampCol = findColByType(ViewColumnInfo::TYPE_TIMESTAMP);
		setValue(author,row,authorCol);
		setValue(time(0),row,timestampCol);
	}

//	if(baseNameAutoUID != "")
//	{
//		std::string indexSubstring = "0";
//		//if there is a last row with baseName in it
//		if(theDataView_.size() > 1 &&
//				0 ==
//				theDataView_[theDataView_.size()-2][getColUID()].find(baseNameAutoUID))
//			//extract last index
//			indexSubstring = theDataView_[theDataView_.size()-2][getColUID()].substr(
//					baseNameAutoUID.size());
//
//		unsigned int index;
//		sscanf(indexSubstring.c_str(),"%u",&index);
//		++index; //increment
//		char indexString[100];
//		sprintf(indexString,"%u",index);
//
//		baseNameAutoUID += indexString;
//		setValue(baseNameAutoUID,row,getColUID());
//	}

	return row;
}

//==============================================================================
//deleteRow
//	throws exception on failure
void ConfigurationView::deleteRow(int r)
{
	if(r >= (int)getNumberOfRows())
	{
		//out of bounds
		__SS__ << "Row " << (int)r << " is out of bounds (Row Count = " <<
				getNumberOfRows() << ") and can not be deleted." <<
				std::endl;
		throw std::runtime_error(ss.str());
	}

	theDataView_.erase(theDataView_.begin()+r);
}

//==============================================================================
//isColChildLinkUnique ~
//	find the pair of columns associated with a child link.
//
//	c := a member column of the pair
//
//	returns:
//		isGroup := indicates pair found is a group link
//		linkPair := pair of columns that are part of the link
//
//	a unique link is defined by two columns: TYPE_START_CHILD_LINK, TYPE_START_CHILD_LINK_UID
//  a group link is defined by two columns: TYPE_START_CHILD_LINK, TYPE_START_CHILD_LINK_GROUP_ID
//
//	returns true if column is member of a group or unique link.
const bool ConfigurationView::getChildLink(const unsigned int& c, bool& isGroup,
		std::pair<unsigned int /*link col*/, unsigned int /*link id col*/>& linkPair) const
{

	if(!(c < columnsInfo_.size()))
	{
		__SS__ << "Invalid col (" << (int)c << ") requested!" << std::endl;
		throw std::runtime_error(ss.str());
	}

	//__COUT__ << "getChildLink for col: " << (int)c << "-" <<
	//		columnsInfo_[c].getType() << "-" << columnsInfo_[c].getName() << std::endl;

	//check if column is a child link UID
	if((isGroup = columnsInfo_[c].isChildLinkGroupID()) ||
			columnsInfo_[c].isChildLinkUID())
	{
		//must be part of unique link, (or invalid table?)
		//__COUT__ << "col: " << (int)c << std::endl;
		linkPair.second = c;
		std::string index = columnsInfo_[c].getChildLinkIndex();

		//__COUT__ << "index: " << index << std::endl;

		//find pair link
		for(unsigned int col=0; col<columnsInfo_.size(); ++col)
		{
			//__COUT__ << "try: " << col << "-" << columnsInfo_[col].getType() << "-" << columnsInfo_[col].getName() << std::endl;
			if(col == c) continue; //skip column c that we know
			else if(columnsInfo_[col].isChildLink() &&
					index == columnsInfo_[col].getChildLinkIndex())
			{
				//found match!
				//__COUT__ << "getChildLink Found match for col: " << (int)c << " at " << col << std::endl;
				linkPair.first = col;
				return true;
			}
		}

		//if here then invalid table!
		__SS__ << "\tIn view: " << tableName_ <<
				", Can't find complete child link for column name " << columnsInfo_[c].getName() << std::endl;
		throw std::runtime_error(ss.str());
	}

	if(!columnsInfo_[c].isChildLink())
		return false; //cant be unique link

	//this is child link, so find pair link uid or gid column
	linkPair.first = c;
	std::string index = columnsInfo_[c].getChildLinkIndex();

	//__COUT__ << "index: " << index << std::endl;

	//find pair link
	for(unsigned int col=0; col<columnsInfo_.size(); ++col)
	{
		//__COUT__ << "try: " << col << "-" << columnsInfo_[col].getType() << "-" << columnsInfo_[col].getName() << std::endl;
		if(col == c) continue; //skip column c that we know
		//		__COUT__ << "try: " << col << "-" << columnsInfo_[col].getType() <<
		//				"-" << columnsInfo_[col].getName() <<
		//				"-u" << columnsInfo_[col].isChildLinkUID() <<
		//				"-g" << columnsInfo_[col].isChildLinkGroupID() << std::endl;
		//
		//		if(columnsInfo_[col].isChildLinkUID())
		//			__COUT__ << "-L" << columnsInfo_[col].getChildLinkIndex() << std::endl;
		//
		//		if(columnsInfo_[col].isChildLinkGroupID())
		//			__COUT__ << "-L" << columnsInfo_[col].getChildLinkIndex() << std::endl;

		if(((columnsInfo_[col].isChildLinkUID() && !(isGroup = false)) ||
				(columnsInfo_[col].isChildLinkGroupID() && (isGroup = true)))
				&& index == columnsInfo_[col].getChildLinkIndex())
		{
			//found match!
			//__COUT__ << "getChildLink Found match for col: " << (int)c << " at " << col << std::endl;
			linkPair.second = col;
			return true;
		}
	}

	//if here then invalid table!
	__SS__ << "\tIn view: " << tableName_ <<
			", Can't find complete child link id for column name " << columnsInfo_[c].getName() << std::endl;
	throw std::runtime_error(ss.str());
}

//==============================================================================
//isNumber ~~
//	returns true if hex ("0x.."), binary("b..."), or base10 number
bool ConfigurationView::isNumber(const std::string& s) const
{
	//__COUT__ << "string " << s << std::endl;
	if(s.find("0x") == 0) //indicates hex
	{
		//__COUT__ << "0x found" << std::endl;
		for(unsigned int i=2;i<s.size();++i)
		{
			if(!((s[i] >= '0' && s[i] <= '9') ||
					(s[i] >= 'A' && s[i] <= 'F') ||
					(s[i] >= 'a' && s[i] <= 'f')
			))
			{
				//__COUT__ << "prob " << s[i] << std::endl;
				return false;
			}
		}
		//return std::regex_match(s.substr(2), std::regex("^[0-90-9a-fA-F]+"));
	}
	else if(s[0] == 'b') //indicates binary
	{
		//__COUT__ << "b found" << std::endl;

		for(unsigned int i=1;i<s.size();++i)
		{
			if(!((s[i] >= '0' && s[i] <= '1')
			))
			{
				//__COUT__ << "prob " << s[i] << std::endl;
				return false;
			}
		}
	}
	else
	{
		//__COUT__ << "base 10 " << std::endl;
		for(unsigned int i=0;i<s.size();++i)
			if(!((s[i] >= '0' && s[i] <= '9') ||
					s[i] == '.' ||
					s[i] == '+' ||
					s[i] == '-'))
				return false;
		//Note: std::regex crashes in unresolvable ways (says Ryan.. also, stop using libraries)
		//return std::regex_match(s, std::regex("^(\\-|\\+)?[0-9]*(\\.[0-9]+)?"));
	}

	return true;
}




#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>

std::string ots_demangle(const char* name) {

	int status = -4; // some arbitrary value to eliminate the compiler warning

	// enable c++11 by passing the flag -std=c++11 to g++
	std::unique_ptr<char, void(*)(void*)> res {
		abi::__cxa_demangle(name, NULL, NULL, &status),
				std::free
	};

	return (status==0) ? res.get() : name ;
}

#else

// does nothing if not g++
std::string ots_demangle(const char* name) {
	return name;
}


#endif
