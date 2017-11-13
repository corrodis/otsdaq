#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationInfoReader.h"

#include <iostream>       // std::cout
#include <typeinfo>


using namespace ots;


#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "ConfigurationBase"
#undef 	__MF_HDR__
#define __MF_HDR__		__COUT_HDR_FL__ << getConfigurationName() << ": "


//==============================================================================
//ConfigurationBase
//	If a valid string pointer is passed in accumulatedExceptions
//	then allowIllegalColumns is set for InfoReader
//	If accumulatedExceptions pointer = 0, then illegal columns throw std::runtime_error exception
ConfigurationBase::ConfigurationBase(std::string configurationName,
		std::string *accumulatedExceptions)
:	MAX_VIEWS_IN_CACHE      (5)//This is done, so that inheriting configuration classes could have varying amounts of cache
,	configurationName_      (configurationName)
,	activeConfigurationView_(0)
{
	//info reader fills up the mockup view
	ConfigurationInfoReader configurationInfoReader(accumulatedExceptions);
	try //to read info
	{
		std::string returnedExceptions = configurationInfoReader.read(this);

		if(returnedExceptions != "")
			__COUT_ERR__ << returnedExceptions << std::endl;

		if(accumulatedExceptions) *accumulatedExceptions += std::string("\n") + returnedExceptions;
	}
	catch(...) //if accumulating exceptions, continue to and return, else throw
	{
		__SS__ << "Failure in configurationInfoReader.read(this)" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		if(accumulatedExceptions) *accumulatedExceptions += std::string("\n") +
				ss.str();
		else throw;
		return; //do not proceed with mockup check if this failed
	}

	//call init on mockup view to verify columns
	try
	{
		getMockupViewP()->init();
	}
	catch(std::runtime_error& e) //if accumulating exceptions, continue to and return, else throw
	{
		if(accumulatedExceptions) *accumulatedExceptions += std::string("\n") + e.what();
		else throw;
	}
}
//==============================================================================
//ConfigurationBase
//	Default constructor is only used  to create special tables
//		not based on an ...Info.xml file
//	e.g. the ConfigurationGroupMetadata table in ConfigurationManager
ConfigurationBase::ConfigurationBase(void)
:	MAX_VIEWS_IN_CACHE      (1)//This is done, so that inheriting configuration classes could have varying amounts of cache
,	configurationName_      ("")
,	activeConfigurationView_(0)
{
}

//==============================================================================
ConfigurationBase::~ConfigurationBase(void)
{
}

//==============================================================================
std::string ConfigurationBase::getTypeId()
{
	return typeid(this).name();
}

//==============================================================================
void ConfigurationBase::init(ConfigurationManager *configurationManager)
{
	//__COUT__ << "Default ConfigurationBase::init() called." << std::endl;
}

//==============================================================================
void ConfigurationBase::reset(bool keepTemporaryVersions)
{
	//std::cout << __COUT_HDR_FL__ << "resetting" << std::endl;
	deactivate();
	if(keepTemporaryVersions)
		trimCache(0);
	else //clear all
		configurationViews_.clear();
}

//==============================================================================
void ConfigurationBase::print(std::ostream &out) const
{
	//std::cout << __COUT_HDR_FL__ << "activeVersion_ " << activeVersion_ << " (INVALID_VERSION:=" << INVALID_VERSION << ")" << std::endl;
	if(!activeConfigurationView_)
	{
		__COUT_ERR__ << "ERROR: No active view set" << std::endl;
		return;
	}
	activeConfigurationView_->print(out);
}

//==============================================================================
// makes active version the specified configuration view version
//  if the version is not already stored, then creates a mockup version
void ConfigurationBase::setupMockupView(ConfigurationVersion version)
{
	if(!isStored(version))
	{
		configurationViews_[version].copy(mockupConfigurationView_,
				version,
				mockupConfigurationView_.getAuthor());
		trimCache();
		if(!isStored(version)) //the trim cache is misbehaving!
		{
			__SS__ << "\nsetupMockupView() IMPOSSIBLE ERROR: trimCache() is deleting the latest view version " <<
					version	<< "!" << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}
	}
	else
	{
		__SS__ << "\nsetupMockupView() ERROR: View to fill with mockup already exists: " << version
				<< ". Cannot overwrite!" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}
}


//==============================================================================
//trimCache
//	if there are more views than MAX_VIEWS_IN_CACHE, erase them.
//	choose wisely the view to delete
//		(by access time)
void ConfigurationBase::trimCache(unsigned int trimSize)
{
	//delete cached views, if necessary

	if(trimSize == (unsigned int)-1) //if -1, use MAX_VIEWS_IN_CACHE
		trimSize = MAX_VIEWS_IN_CACHE;

	int i=0;
	while(getNumberOfStoredViews() > trimSize)
	{
		ConfigurationVersion versionToDelete;
		time_t stalestTime = -1;

		for(auto &viewPair : configurationViews_)
			if(!viewPair.first.isTemporaryVersion())
			{
				if(stalestTime == -1 ||
						viewPair.second.getLastAccessTime() < stalestTime)
				{
					versionToDelete = viewPair.first;
					stalestTime = viewPair.second.getLastAccessTime();
					if(!trimSize) break; //if trimSize is 0, then just take first found
				}
			}

		if(versionToDelete.isInvalid())
		{
			__SS__ << "Can NOT have a stored view with an invalid version!";
			throw std::runtime_error(ss.str());
		}

		eraseView(versionToDelete);
	}
}

//==============================================================================
//trimCache
//	if there are more views than MAX_VIEWS_IN_CACHE, erase them.
//	choose wisely the view to delete
//		(by access time)
void ConfigurationBase::trimTemporary(ConfigurationVersion targetVersion)
{
	if(targetVersion.isInvalid()) //erase all temporary
	{
		for(auto it = configurationViews_.begin(); it != configurationViews_.end(); /*no increment*/)
		{
			if(it->first.isTemporaryVersion())
			{
				__COUT__ << "Trimming temporary version: " << it->first << std::endl;
				if(activeConfigurationView_ &&
					getViewVersion() == it->first) //if activeVersion is being erased!
					deactivate(); 		//deactivate active view, instead of guessing at next active view
				configurationViews_.erase(it++);
			}
			else
				++it;
		}
	}
	else if(targetVersion.isTemporaryVersion()) //erase target
	{
		__COUT__ << "Trimming temporary version: " << targetVersion << std::endl;
		eraseView(targetVersion);
	}
	else
	{
		//else this is a persistent version!
		__SS__ << "Temporary trim target was a persistent version: " <<
				targetVersion << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}
}

//==============================================================================
//checkForDuplicate
// look for a duplicate of the needleVersion in the haystack
//	which is the cached views in configurationViews_
//
//	Note: ignoreVersion is useful if you know another view is already identical
//		like when converting from temporary to persistent
//
//	Return invalid if no matches
ConfigurationVersion ConfigurationBase::checkForDuplicate(ConfigurationVersion needleVersion,
		ConfigurationVersion ignoreVersion) const
{
	auto needleIt = configurationViews_.find(needleVersion);
	if(needleIt == configurationViews_.end())
	{
		//else this is a persistent version!
		__SS__ << "needleVersion does not exist: " <<
				needleVersion << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}

	const ConfigurationView *needleView = &(needleIt->second);
	unsigned int rows = needleView->getNumberOfRows();
	unsigned int cols = needleView->getNumberOfColumns();

	bool match;
	unsigned int potentialMatchCount = 0;

	//needleView->print();

	//for each table in cache
	//	check each row,col
	for(auto &viewPair: configurationViews_)
	{
		if(viewPair.first == needleVersion) continue; //skip needle version
		if(viewPair.first == ignoreVersion) continue; //skip ignore version

		if(viewPair.second.getNumberOfRows() != rows)
			continue; //row mismatch

		if(viewPair.second.getDataColumnSize() != cols ||
				viewPair.second.getSourceColumnMismatch() != 0)
			continue; //col mismatch


		++potentialMatchCount;
		__COUT__ << "Checking version... " << viewPair.first << std::endl;

		//viewPair.second.print();

		match = true;

		auto viewColInfoIt = viewPair.second.getColumnsInfo().begin();
		for(unsigned int col=0; match && //note column size must already match
			viewPair.second.getColumnsInfo().size() > 3 &&
			col<viewPair.second.getColumnsInfo().size()-3;++col,viewColInfoIt++)
			if(viewColInfoIt->getName() !=
					needleView->getColumnsInfo()[col].getName())
			{
				match = false;
//				__COUT__ << "Column name mismatch " << col << ":" <<
//						viewColInfoIt->getName() << " vs " <<
//						needleView->getColumnsInfo()[col].getName() << std::endl;
			}

		for(unsigned int row=0;match && row<rows;++row)
		{
			for(unsigned int col=0;col<cols-2;++col) //do not consider author and timestamp
				if(viewPair.second.getDataView()[row][col] !=
						needleView->getDataView()[row][col])
				{
					match = false;

//					__COUT__ << "Value name mismatch " << col << ":" <<
//							viewPair.second.getDataView()[row][col] << "[" <<
//							viewPair.second.getDataView()[row][col].size() << "]" <<
//							" vs " <<
//							needleView->getDataView()[row][col] << "[" <<
//							needleView->getDataView()[row][col].size() << "]" <<
//							std::endl;

					break;
				}
		}
		if(match)
		{
			__COUT_INFO__ << "Duplicate version found: " << viewPair.first << std::endl;
			return viewPair.first;
		}
	}

	__COUT__ << "No duplicates found in " << potentialMatchCount << " potential matches." << std::endl;
	return ConfigurationVersion(); //return invalid if no matches
}

//==============================================================================
void ConfigurationBase::changeVersionAndActivateView(ConfigurationVersion temporaryVersion,
		ConfigurationVersion version)
{
	if(configurationViews_.find(temporaryVersion) == configurationViews_.end())
	{
		__SS__ << "ERROR: Temporary view version " << temporaryVersion << " doesn't exists!" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}
	if(version.isInvalid())
	{
		__SS__ << "ERROR: Attempting to create an invalid version " << version <<
				"! Did you really run out of versions? (this should never happen)" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}

	if(configurationViews_.find(version) != configurationViews_.end())
		__COUT_WARN__ << "WARNING: View version " << version << " already exists! Overwriting." << std::endl;

	configurationViews_[version].copy(configurationViews_[temporaryVersion],
			version,
			configurationViews_[temporaryVersion].getAuthor());
	setActiveView(version);
	eraseView(temporaryVersion);		//delete temp version from configurationViews_
}

//==============================================================================
bool ConfigurationBase::isStored(const ConfigurationVersion &version) const
{
	return (configurationViews_.find(version) != configurationViews_.end());
}

//==============================================================================
bool ConfigurationBase::eraseView(ConfigurationVersion version)
{
	if(!isStored(version))
		return false;

	if(activeConfigurationView_ &&
			getViewVersion() == version) //if activeVersion is being erased!
		deactivate(); 		//deactivate active view, instead of guessing at next active view

	configurationViews_.erase(version);

	return true;
}

//==============================================================================
const std::string& ConfigurationBase::getConfigurationName(void) const
{
	return configurationName_;
}

//==============================================================================
const std::string& ConfigurationBase::getConfigurationDescription(void) const
{
	return configurationDescription_;
}

//==============================================================================
const ConfigurationVersion& ConfigurationBase::getViewVersion(void) const
{
	return getView().getVersion();
}


//==============================================================================
//latestAndMockupColumnNumberMismatch
//	intended to check if the column count was recently changed
bool ConfigurationBase::latestAndMockupColumnNumberMismatch(void) const
{
	std::set<ConfigurationVersion> retSet = getStoredVersions();
	if(retSet.size() && !retSet.rbegin()->isTemporaryVersion())
	{
		return configurationViews_.find(*(retSet.rbegin()))->second.getNumberOfColumns() !=
				mockupConfigurationView_.getNumberOfColumns();
	}
	//there are no latest non-temporary tables so there is a mismatch (by default)
	return true;
}

//==============================================================================
std::set<ConfigurationVersion> ConfigurationBase::getStoredVersions(void) const
{
	std::set<ConfigurationVersion> retSet;
	for(auto &configs:configurationViews_)
		retSet.emplace(configs.first);
	return retSet;
}

//==============================================================================
//getNumberOfStoredViews
//	count number of stored views, not including temporary views
//	(invalid views should be impossible)
unsigned int ConfigurationBase::getNumberOfStoredViews(void) const
{
	unsigned int sz = 0;
	for(auto &viewPair : configurationViews_)
		if(viewPair.first.isTemporaryVersion()) continue;
		else if(viewPair.first.isInvalid())
		{
			//__SS__ << "Can NOT have a stored view with an invalid version!";
			//throw std::runtime_error(ss.str());

			//NOTE: if this starts happening a lot, could just auto-correct and remove the invalid version
			// but it would be better to fix the cause.

			//FIXME... for now just auto correcting
			__COUT__ << "There is an invalid version now!.. where did it come from?" << std::endl;
		}
		else ++sz;
	return sz;
}

//==============================================================================
const ConfigurationView& ConfigurationBase::getView(void) const
{
	if(!activeConfigurationView_)
	{
		__SS__ << "activeConfigurationView_ pointer is null! (...likely the active view was not setup properly. Check your system setup.)";
		throw std::runtime_error(ss.str());
	}
	return *activeConfigurationView_;
}

//==============================================================================
ConfigurationView* ConfigurationBase::getViewP(void)
{
	if(!activeConfigurationView_)
	{
		__SS__ << "activeConfigurationView_ pointer is null! (...likely the active view was not setup properly. Check your system setup.)";
		throw std::runtime_error(ss.str());
	}
	return activeConfigurationView_;
}

//==============================================================================
ConfigurationView* ConfigurationBase::getMockupViewP(void)
{
	return &mockupConfigurationView_;
}

//==============================================================================
void ConfigurationBase::setConfigurationName(const std::string &configurationName)
{
	configurationName_ = configurationName;
}

//==============================================================================
void ConfigurationBase::setConfigurationDescription(const std::string &configurationDescription)
{
	configurationDescription_ = configurationDescription;
}

//==============================================================================
//deactivate
//	reset the active view
void ConfigurationBase::deactivate()
{
	activeConfigurationView_ = 0;
}

//==============================================================================
//isActive
bool ConfigurationBase::isActive()
{
	return activeConfigurationView_?true:false;
}

//==============================================================================
bool ConfigurationBase::setActiveView(ConfigurationVersion version)
{
	if(!isStored(version))
	{ 	//we don't call else load for the user, because the configuration manager would lose track.. (I think?)
		//so load new versions for the first time through the configuration manager only. (I think??)
		__SS__ << "\nsetActiveView() ERROR: View with version " << version <<
				" has never been stored before!" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
		return false;
	}
	activeConfigurationView_ = &configurationViews_[version];

	if(configurationViews_[version].getVersion() !=  version)
	{
		__SS__ << "Something has gone very wrong with the version handling!" << std::endl;
		throw std::runtime_error(ss.str());
	}

	return true;
}


//==============================================================================
//copyView
//	copies source view (including version) and places in self
//	as destination temporary version.
//	if destination version is invalid, then next available temporary version is chosen
//	if conflict, throw exception
//
//	Returns version of new temporary view that was created.
ConfigurationVersion ConfigurationBase::copyView (const ConfigurationView &sourceView,
		ConfigurationVersion destinationVersion, const std::string &author)
throw(std::runtime_error)
{
	//check that column sizes match
	if(sourceView.getNumberOfColumns() !=
			mockupConfigurationView_.getNumberOfColumns())
	{
		__SS__ << "Error! Number of Columns of source view must match destination mock-up view." <<
				"Dimension of source is [" <<	sourceView.getNumberOfColumns() <<
				"] and of destination mockup is [" <<
				mockupConfigurationView_.getNumberOfColumns() << "]." << std::endl;
		throw std::runtime_error(ss.str());
	}

	//check for destination version confict
	if(!destinationVersion.isInvalid() &&
			configurationViews_.find(sourceView.getVersion()) != configurationViews_.end())
	{
		__SS__ << "Error! Asked to copy a view with a conflicting version: " <<
				sourceView.getVersion() << std::endl;
		throw std::runtime_error(ss.str());
	}

	//if destinationVersion is INVALID, creates next available temporary version
	destinationVersion = createTemporaryView(ConfigurationVersion(),
			destinationVersion);

	__COUT__ << "Copying from " << sourceView.getTableName() << "_v" <<
			sourceView.getVersion() << " to " << getConfigurationName() << "_v" <<
			destinationVersion << std::endl;

	try
	{
		configurationViews_[destinationVersion].copy(sourceView,destinationVersion,author);
	}
	catch(...) //if the copy fails then delete the destinationVersion view
	{
		__COUT_ERR__ << "Failed to copy from " << sourceView.getTableName() << "_v" <<
				sourceView.getVersion() << " to " << getConfigurationName() << "_v" <<
				destinationVersion << std::endl;
		__COUT_WARN__ << "Deleting the failed destination version " <<
				destinationVersion << std::endl;
		eraseView(destinationVersion);
		throw; //and rethrow
	}

	return destinationVersion;
}


//==============================================================================
//createTemporaryView
//	-1, from MockUp, else from valid view version
//	destTemporaryViewVersion is starting point for search for available temporary versions.
//	if destTemporaryViewVersion is invalid, starts search at ConfigurationVersion::getNextTemporaryVersion().
// 	returns new temporary version number (which is always negative)
ConfigurationVersion ConfigurationBase::createTemporaryView(ConfigurationVersion sourceViewVersion,
		ConfigurationVersion destTemporaryViewVersion)
{
	__COUT__ << "Configuration: " <<
			getConfigurationName()<< std::endl;

	__COUT__ << "Num of Views: " <<
			configurationViews_.size() << " (Temporary Views: " <<
			(configurationViews_.size() - getNumberOfStoredViews()) << ")" << std::endl;

	ConfigurationVersion tmpVersion = destTemporaryViewVersion;
	if(tmpVersion.isInvalid()) tmpVersion = ConfigurationVersion::getNextTemporaryVersion();
	while(isStored(tmpVersion) && //find a new valid temporary version
			!(tmpVersion = ConfigurationVersion::getNextTemporaryVersion(tmpVersion)).isInvalid());
	if(isStored(tmpVersion) || tmpVersion.isInvalid())
	{
		__SS__ << "Invalid destination temporary version: " <<
				destTemporaryViewVersion << ". Expected next temporary version < " << tmpVersion << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}

	if(sourceViewVersion == ConfigurationVersion::INVALID || 	//use mockup if sourceVersion is -1 or not found
			configurationViews_.find(sourceViewVersion) == configurationViews_.end())
	{
		if(sourceViewVersion != -1)
		{
			__SS__ << "ERROR: sourceViewVersion " << sourceViewVersion << " not found. " <<
					"Invalid source version. Version requested is not stored (yet?) or does not exist." << std::endl;
			__COUT_ERR__ << ss.str();
			throw std::runtime_error(ss.str());
		}
		__COUT__ << "Using Mock-up view" << std::endl;
		configurationViews_[tmpVersion].copy(mockupConfigurationView_,
				tmpVersion,
				mockupConfigurationView_.getAuthor());
	}
	else
	{
		try //do not allow init to throw an exception here..
		{	//it's ok to copy invalid data, the user may be trying to change it
			configurationViews_[tmpVersion].copy(configurationViews_[sourceViewVersion],
					tmpVersion,
					configurationViews_[sourceViewVersion].getAuthor());
		}
		catch(...)
		{
			__COUT_WARN__ << "createTemporaryView() Source view failed init(). " <<
					"This is being ignored (hopefully the new copy is being fixed)." << std::endl;
		}
	}

	return tmpVersion;
}

//==============================================================================
//getNextAvailableTemporaryView
//	ConfigurationVersion::INVALID is always MockUp
// returns next available temporary version number (which is always negative)
ConfigurationVersion ConfigurationBase::getNextTemporaryVersion() const
{
	ConfigurationVersion tmpVersion;

	//std::map guarantees versions are in increasing order!
	if(configurationViews_.size() != 0 && configurationViews_.begin()->first.isTemporaryVersion())
		tmpVersion =
				ConfigurationVersion::getNextTemporaryVersion(configurationViews_.begin()->first);
	else
		tmpVersion = ConfigurationVersion::getNextTemporaryVersion();

	//verify tmpVersion is ok
	if(isStored(tmpVersion) || tmpVersion.isInvalid() || !tmpVersion.isTemporaryVersion())
	{
		__SS__ << "Invalid destination temporary version: " <<
				tmpVersion << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}
	return tmpVersion;
}

//==============================================================================
//getNextVersion
// 	returns next available new version
//	the implication is any version number equal or greater is available.
ConfigurationVersion ConfigurationBase::getNextVersion() const
{
	ConfigurationVersion tmpVersion;

	//std::map guarantees versions are in increasing order!
	if(configurationViews_.size() != 0 && !configurationViews_.rbegin()->first.isTemporaryVersion())
		tmpVersion =
				ConfigurationVersion::getNextVersion(configurationViews_.rbegin()->first);
	else
		tmpVersion = ConfigurationVersion::getNextVersion();

	//verify tmpVersion is ok
	if(isStored(tmpVersion) || tmpVersion.isInvalid() || tmpVersion.isTemporaryVersion())
	{
		__SS__ << "Invalid destination next version: " <<
				tmpVersion << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}
	return tmpVersion;
}

//==============================================================================
//getTemporaryView
//	must be a valid temporary version, and the view must be stored in configuration.
// 	temporary version indicates it has not been saved to database and assigned a version number
ConfigurationView* ConfigurationBase::getTemporaryView(ConfigurationVersion temporaryVersion)
{
	if(!temporaryVersion.isTemporaryVersion() ||
			!isStored(temporaryVersion))
	{
		__SS__ << getConfigurationName() << ":: Error! Temporary version not found!" << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}
	return &configurationViews_[temporaryVersion];
}


//==============================================================================
//convertToCaps
//	static utility for converting configuration and column names to the caps version
//	throw std::runtime_error if not completely alpha-numeric input
std::string ConfigurationBase::convertToCaps(std::string& str, bool isConfigName)
throw(std::runtime_error)
{
	//append Configuration to be nice to user
	unsigned int configPos = (unsigned int)std::string::npos;
	if(isConfigName && (configPos = str.find("Configuration")) !=
			str.size() - strlen("Configuration"))
		str += "Configuration";

	//std::cout << str << std::endl;
	//std::cout << configPos << " " << (str.size() - strlen("Configuration")) << std::endl;

	//create all caps name and validate
	//	only allow alpha names with Configuration at end
	std::string capsStr = "";
	for(unsigned int c=0;c<str.size();++c)
		if(str[c] >= 'A' && str[c] <= 'Z')
		{
			//add _ before configuration and if lower case to uppercase
			if(c == configPos ||
					(c && str[c-1] >= 'a' && str[c-1] <= 'z') || //if this is a new start of upper case
				(c && str[c-1] >= 'A' && str[c-1] <= 'Z' &&	//if this is a new start from running caps
						c+1 < str.size() && str[c+1] >= 'a' && str[c+1] <= 'z')
				)
				capsStr += "_";
			capsStr += str[c];
		}
		else if(str[c] >= 'a' && str[c] <= 'z')
			capsStr += char(str[c] -32); //capitalize
		else if(str[c] >= '0' && str[c] <= '9')
			capsStr += str[c]; //allow numbers
		else //error! non-alpha
			throw std::runtime_error(std::string("ConfigurationBase::convertToCaps::") +
					"Invalid character found in name (allowed: A-Z, a-z, 0-9):" +
					str );

	return capsStr;
}




