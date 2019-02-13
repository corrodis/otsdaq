#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationInfoReader.h"

#include <iostream>  // std::cout
#include <typeinfo>

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "ConfigurationBase-" + getConfigurationName()

//==============================================================================
// ConfigurationBase
//	If a valid string pointer is passed in accumulatedExceptions
//	then allowIllegalColumns is set for InfoReader
//	If accumulatedExceptions pointer = 0, then illegal columns throw std::runtime_error exception
ConfigurationBase::ConfigurationBase(std::string configurationName,
                                     std::string* accumulatedExceptions)
    : MAX_VIEWS_IN_CACHE(
          20)  // This is done, so that inheriting configuration classes could have varying amounts of cache
      ,
      configurationName_(configurationName),
      activeConfigurationView_(0) {
  // info reader fills up the mockup view
  ConfigurationInfoReader configurationInfoReader(accumulatedExceptions);
  try  // to read info
  {
    std::string returnedExceptions = configurationInfoReader.read(this);

    if (returnedExceptions != "") __COUT_ERR__ << returnedExceptions << __E__;

    if (accumulatedExceptions) *accumulatedExceptions += std::string("\n") + returnedExceptions;
  } catch (...)  // if accumulating exceptions, continue to and return, else throw
  {
    __SS__ << "Failure in configurationInfoReader.read(this)" << __E__;
    __COUT_ERR__ << "\n" << ss.str();
    if (accumulatedExceptions)
      *accumulatedExceptions += std::string("\n") + ss.str();
    else
      throw;
    return;  // do not proceed with mockup check if this failed
  }

  // call init on mockup view to verify columns
  try {
    getMockupViewP()->init();
  } catch (std::runtime_error& e)  // if accumulating exceptions, continue to and return, else throw
  {
    if (accumulatedExceptions)
      *accumulatedExceptions += std::string("\n") + e.what();
    else
      throw;
  }
}
//==============================================================================
// ConfigurationBase
//	Default constructor is only used  to create special tables
//		not based on an ...Info.xml file
//	e.g. the ConfigurationGroupMetadata table in ConfigurationManager
ConfigurationBase::ConfigurationBase(void)
    : MAX_VIEWS_IN_CACHE(
          1)  // This is done, so that inheriting configuration classes could have varying amounts of cache
      ,
      configurationName_(""),
      activeConfigurationView_(0) {}

//==============================================================================
ConfigurationBase::~ConfigurationBase(void) {}

//==============================================================================
std::string ConfigurationBase::getTypeId() { return typeid(this).name(); }

//==============================================================================
void ConfigurationBase::init(ConfigurationManager* configurationManager) {
  //__COUT__ << "Default ConfigurationBase::init() called." << __E__;
}

//==============================================================================
void ConfigurationBase::reset(bool keepTemporaryVersions) {
  // std::cout << __COUT_HDR_FL__ << "resetting" << __E__;
  deactivate();
  if (keepTemporaryVersions)
    trimCache(0);
  else  // clear all
    configurationViews_.clear();
}

//==============================================================================
void ConfigurationBase::print(std::ostream& out) const {
  // std::cout << __COUT_HDR_FL__ << "activeVersion_ " << activeVersion_ << " (INVALID_VERSION:=" << INVALID_VERSION <<
  // ")" << __E__;
  if (!activeConfigurationView_) {
    __COUT_ERR__ << "ERROR: No active view set" << __E__;
    return;
  }
  activeConfigurationView_->print(out);
}

//==============================================================================
// makes active version the specified configuration view version
//  if the version is not already stored, then creates a mockup version
void ConfigurationBase::setupMockupView(ConfigurationVersion version) {
  if (!isStored(version)) {
    configurationViews_[version].copy(mockupConfigurationView_, version, mockupConfigurationView_.getAuthor());
    trimCache();
    if (!isStored(version))  // the trim cache is misbehaving!
    {
      __SS__ << "\nsetupMockupView() IMPOSSIBLE ERROR: trimCache() is deleting the latest view version " << version
             << "!" << __E__;
      __COUT_ERR__ << "\n" << ss.str();
      __SS_THROW__;
    }
  } else {
    __SS__ << "\nsetupMockupView() ERROR: View to fill with mockup already exists: " << version << ". Cannot overwrite!"
           << __E__;
    __COUT_ERR__ << "\n" << ss.str();
    __SS_THROW__;
  }
}

//==============================================================================
// trimCache
//	if there are more views than MAX_VIEWS_IN_CACHE, erase them.
//	choose wisely the view to delete
//		(by access time)
void ConfigurationBase::trimCache(unsigned int trimSize) {
  // delete cached views, if necessary

  if (trimSize == (unsigned int)-1)  // if -1, use MAX_VIEWS_IN_CACHE
    trimSize = MAX_VIEWS_IN_CACHE;

  int i = 0;
  while (getNumberOfStoredViews() > trimSize) {
    ConfigurationVersion versionToDelete;
    time_t stalestTime = -1;

    for (auto& viewPair : configurationViews_)
      if (!viewPair.first.isTemporaryVersion()) {
        if (stalestTime == -1 || viewPair.second.getLastAccessTime() < stalestTime) {
          versionToDelete = viewPair.first;
          stalestTime = viewPair.second.getLastAccessTime();
          if (!trimSize) break;  // if trimSize is 0, then just take first found
        }
      }

    if (versionToDelete.isInvalid()) {
      __SS__ << "Can NOT have a stored view with an invalid version!";
      __SS_THROW__;
    }

    eraseView(versionToDelete);
  }
}

//==============================================================================
// trimCache
//	if there are more views than MAX_VIEWS_IN_CACHE, erase them.
//	choose wisely the view to delete
//		(by access time)
void ConfigurationBase::trimTemporary(ConfigurationVersion targetVersion) {
  if (targetVersion.isInvalid())  // erase all temporary
  {
    for (auto it = configurationViews_.begin(); it != configurationViews_.end(); /*no increment*/) {
      if (it->first.isTemporaryVersion()) {
        __COUT__ << "Trimming temporary version: " << it->first << __E__;
        if (activeConfigurationView_ && getViewVersion() == it->first)  // if activeVersion is being erased!
          deactivate();  // deactivate active view, instead of guessing at next active view
        configurationViews_.erase(it++);
      } else
        ++it;
    }
  } else if (targetVersion.isTemporaryVersion())  // erase target
  {
    __COUT__ << "Trimming temporary version: " << targetVersion << __E__;
    eraseView(targetVersion);
  } else {
    // else this is a persistent version!
    __SS__ << "Temporary trim target was a persistent version: " << targetVersion << __E__;
    __COUT_ERR__ << "\n" << ss.str();
    __SS_THROW__;
  }
}

//==============================================================================
// checkForDuplicate
// look for a duplicate of the needleVersion in the haystack
//	which is the cached views in configurationViews_
//
//	Note: ignoreVersion is useful if you know another view is already identical
//		like when converting from temporary to persistent
//
//	Return invalid if no matches
ConfigurationVersion ConfigurationBase::checkForDuplicate(ConfigurationVersion needleVersion,
                                                          ConfigurationVersion ignoreVersion) const {
  auto needleIt = configurationViews_.find(needleVersion);
  if (needleIt == configurationViews_.end()) {
    // else this is a persistent version!
    __SS__ << "needleVersion does not exist: " << needleVersion << __E__;
    __COUT_ERR__ << "\n" << ss.str();
    __SS_THROW__;
  }

  const ConfigurationView* needleView = &(needleIt->second);
  unsigned int rows = needleView->getNumberOfRows();
  unsigned int cols = needleView->getNumberOfColumns();

  bool match;
  unsigned int potentialMatchCount = 0;

  // needleView->print();

  // for each table in cache
  //	check each row,col
  auto viewPairReverseIterator = configurationViews_.rbegin();
  for (; viewPairReverseIterator != configurationViews_.rend(); ++viewPairReverseIterator) {
    if (viewPairReverseIterator->first == needleVersion) continue;      // skip needle version
    if (viewPairReverseIterator->first == ignoreVersion) continue;      // skip ignore version
    if (viewPairReverseIterator->first.isTemporaryVersion()) continue;  // skip temporary versions

    if (viewPairReverseIterator->second.getNumberOfRows() != rows) continue;  // row mismatch

    if (viewPairReverseIterator->second.getDataColumnSize() != cols ||
        viewPairReverseIterator->second.getSourceColumnMismatch() != 0)
      continue;  // col mismatch

    ++potentialMatchCount;
    __COUT__ << "Checking version... " << viewPairReverseIterator->first << __E__;

    // viewPairReverseIterator->second.print();

    // match = true;

    // if column source names do not match then skip
    //	source names are potentially different from getColumnsInfo()/getColumnStorageNames

    match = viewPairReverseIterator->second.getSourceColumnNames().size() == needleView->getSourceColumnNames().size();
    if (match) {
      for (auto& haystackColName : viewPairReverseIterator->second.getSourceColumnNames())
        if (needleView->getSourceColumnNames().find(haystackColName) == needleView->getSourceColumnNames().end()) {
          __COUT__ << "Found column name mismach for '" << haystackColName << "'... So allowing same data!" << __E__;

          match = false;
          break;
        }
    }

    // checking columnsInfo seems to be wrong approach, use getSourceColumnNames (above)
    //		auto viewColInfoIt = viewPairReverseIterator->second.getColumnsInfo().begin();
    //		for(unsigned int col=0; match && //note column size must already match
    //			viewPairReverseIterator->second.getColumnsInfo().size() > 3 &&
    //			col<viewPairReverseIterator->second.getColumnsInfo().size()-3;++col,viewColInfoIt++)
    //			if(viewColInfoIt->getName() !=
    //					needleView->getColumnsInfo()[col].getName())
    //			{
    //				match = false;
    ////				__COUT__ << "Column name mismatch " << col << ":" <<
    ////						viewColInfoIt->getName() << " vs " <<
    ////						needleView->getColumnsInfo()[col].getName() << __E__;
    //			}

    for (unsigned int row = 0; match && row < rows; ++row) {
      for (unsigned int col = 0; col < cols - 2; ++col)  // do not consider author and timestamp
        if (viewPairReverseIterator->second.getDataView()[row][col] != needleView->getDataView()[row][col]) {
          match = false;

          //					__COUT__ << "Value name mismatch " << col << ":" <<
          //							viewPairReverseIterator->second.getDataView()[row][col] << "["
          //<<
          //							viewPairReverseIterator->second.getDataView()[row][col].size() << "]"
          //<< 							" vs " <<
          //							needleView->getDataView()[row][col] << "[" <<
          //							needleView->getDataView()[row][col].size() << "]" <<
          //							__E__;

          break;
        }
    }
    if (match) {
      __COUT_INFO__ << "Duplicate version found: " << viewPairReverseIterator->first << __E__;
      return viewPairReverseIterator->first;
    }
  }

  __COUT__ << "No duplicates found in " << potentialMatchCount << " potential matches." << __E__;
  return ConfigurationVersion();  // return invalid if no matches
}

//==============================================================================
void ConfigurationBase::changeVersionAndActivateView(ConfigurationVersion temporaryVersion,
                                                     ConfigurationVersion version) {
  if (configurationViews_.find(temporaryVersion) == configurationViews_.end()) {
    __SS__ << "ERROR: Temporary view version " << temporaryVersion << " doesn't exists!" << __E__;
    __COUT_ERR__ << "\n" << ss.str();
    __SS_THROW__;
  }
  if (version.isInvalid()) {
    __SS__ << "ERROR: Attempting to create an invalid version " << version
           << "! Did you really run out of versions? (this should never happen)" << __E__;
    __COUT_ERR__ << "\n" << ss.str();
    __SS_THROW__;
  }

  if (configurationViews_.find(version) != configurationViews_.end())
    __COUT_WARN__ << "WARNING: View version " << version << " already exists! Overwriting." << __E__;

  configurationViews_[version].copy(configurationViews_[temporaryVersion], version,
                                    configurationViews_[temporaryVersion].getAuthor());
  setActiveView(version);
  eraseView(temporaryVersion);  // delete temp version from configurationViews_
}

//==============================================================================
bool ConfigurationBase::isStored(const ConfigurationVersion& version) const {
  return (configurationViews_.find(version) != configurationViews_.end());
}

//==============================================================================
bool ConfigurationBase::eraseView(ConfigurationVersion version) {
  if (!isStored(version)) return false;

  if (activeConfigurationView_ && getViewVersion() == version)  // if activeVersion is being erased!
    deactivate();  // deactivate active view, instead of guessing at next active view

  configurationViews_.erase(version);

  return true;
}

//==============================================================================
const std::string& ConfigurationBase::getConfigurationName(void) const { return configurationName_; }

//==============================================================================
const std::string& ConfigurationBase::getConfigurationDescription(void) const { return configurationDescription_; }

//==============================================================================
const ConfigurationVersion& ConfigurationBase::getViewVersion(void) const { return getView().getVersion(); }

//==============================================================================
// latestAndMockupColumnNumberMismatch
//	intended to check if the column count was recently changed
bool ConfigurationBase::latestAndMockupColumnNumberMismatch(void) const {
  std::set<ConfigurationVersion> retSet = getStoredVersions();
  if (retSet.size() && !retSet.rbegin()->isTemporaryVersion()) {
    return configurationViews_.find(*(retSet.rbegin()))->second.getNumberOfColumns() !=
           mockupConfigurationView_.getNumberOfColumns();
  }
  // there are no latest non-temporary tables so there is a mismatch (by default)
  return true;
}

//==============================================================================
std::set<ConfigurationVersion> ConfigurationBase::getStoredVersions(void) const {
  std::set<ConfigurationVersion> retSet;
  for (auto& configs : configurationViews_) retSet.emplace(configs.first);
  return retSet;
}

//==============================================================================
// getNumberOfStoredViews
//	count number of stored views, not including temporary views
//	(invalid views should be impossible)
unsigned int ConfigurationBase::getNumberOfStoredViews(void) const {
  unsigned int sz = 0;
  for (auto& viewPair : configurationViews_)
    if (viewPair.first.isTemporaryVersion())
      continue;
    else if (viewPair.first.isInvalid()) {
      //__SS__ << "Can NOT have a stored view with an invalid version!";
      //__SS_THROW__;

      // NOTE: if this starts happening a lot, could just auto-correct and remove the invalid version
      // but it would be better to fix the cause.

      // FIXME... for now just auto correcting
      __COUT__ << "There is an invalid version now!.. where did it come from?" << __E__;
    } else
      ++sz;
  return sz;
}

//==============================================================================
const ConfigurationView& ConfigurationBase::getView(void) const {
  if (!activeConfigurationView_) {
    __SS__ << "activeConfigurationView_ pointer is null! (...likely the active view was not setup properly. Check your "
              "system setup.)";
    __SS_THROW__;
  }
  return *activeConfigurationView_;
}

//==============================================================================
ConfigurationView* ConfigurationBase::getViewP(void) {
  if (!activeConfigurationView_) {
    __SS__ << "activeConfigurationView_ pointer is null! (...likely the active view was not setup properly. Check your "
              "system setup.)";
    __SS_THROW__;
  }
  return activeConfigurationView_;
}

//==============================================================================
ConfigurationView* ConfigurationBase::getMockupViewP(void) { return &mockupConfigurationView_; }

//==============================================================================
void ConfigurationBase::setConfigurationName(const std::string& configurationName) {
  configurationName_ = configurationName;
}

//==============================================================================
void ConfigurationBase::setConfigurationDescription(const std::string& configurationDescription) {
  configurationDescription_ = configurationDescription;
}

//==============================================================================
// deactivate
//	reset the active view
void ConfigurationBase::deactivate() { activeConfigurationView_ = 0; }

//==============================================================================
// isActive
bool ConfigurationBase::isActive() { return activeConfigurationView_ ? true : false; }

//==============================================================================
bool ConfigurationBase::setActiveView(ConfigurationVersion version) {
  if (!isStored(version)) {  // we don't call else load for the user, because the configuration manager would lose
                             // track.. (I think?)
    // so load new versions for the first time through the configuration manager only. (I think??)
    __SS__ << "\nsetActiveView() ERROR: View with version " << version << " has never been stored before!" << __E__;
    __COUT_ERR__ << "\n" << ss.str();
    __SS_THROW__;
    return false;
  }
  activeConfigurationView_ = &configurationViews_[version];

  if (configurationViews_[version].getVersion() != version) {
    __SS__ << "Something has gone very wrong with the version handling!" << __E__;
    __SS_THROW__;
  }

  return true;
}

//==============================================================================
// mergeViews
//	merges source view A and B and places in
//	destination temporary version.
//	if destination version is invalid, then next available temporary version is chosen
//	one error, throw exception
//
//	Returns version of new temporary view that was created.
ConfigurationVersion ConfigurationBase::mergeViews(
    const ConfigurationView& sourceViewA, const ConfigurationView& sourceViewB, ConfigurationVersion destinationVersion,
    const std::string& author, const std::string& mergeApproach /*Rename,Replace,Skip*/,
    std::map<std::pair<std::string /*original table*/, std::string /*original uidB*/>, std::string /*converted uidB*/>&
        uidConversionMap,
    std::map<std::pair<std::string /*original table*/,
                       std::pair<std::string /*group linkid*/, std::string /*original gidB*/> >,
             std::string /*converted gidB*/>& groupidConversionMap,
    bool fillRecordConversionMaps, bool applyRecordConversionMaps, bool generateUniqueDataColumns) {
  __COUT__ << "mergeViews starting..." << __E__;

  // There 3 modes:
  //	rename		-- All records from both groups are maintained, but conflicts from B are renamed.
  //					Must maintain a map of UIDs that are remapped to new name for groupB,
  //					because linkUID fields must be preserved.
  //	replace		-- Any UID conflicts for a record are replaced by the record from group B.
  //	skip		-- Any UID conflicts for a record are skipped so that group A record remains

  // check valid mode
  if (!(mergeApproach == "Rename" || mergeApproach == "Replace" || mergeApproach == "Skip")) {
    __SS__ << "Error! Invalid merge approach '" << mergeApproach << ".'" << __E__;
    __SS_THROW__;
  }

  // check that column sizes match
  if (sourceViewA.getNumberOfColumns() != mockupConfigurationView_.getNumberOfColumns()) {
    __SS__ << "Error! Number of Columns of source view A must match destination mock-up view."
           << "Dimension of source is [" << sourceViewA.getNumberOfColumns() << "] and of destination mockup is ["
           << mockupConfigurationView_.getNumberOfColumns() << "]." << __E__;
    __SS_THROW__;
  }
  // check that column sizes match
  if (sourceViewB.getNumberOfColumns() != mockupConfigurationView_.getNumberOfColumns()) {
    __SS__ << "Error! Number of Columns of source view B must match destination mock-up view."
           << "Dimension of source is [" << sourceViewB.getNumberOfColumns() << "] and of destination mockup is ["
           << mockupConfigurationView_.getNumberOfColumns() << "]." << __E__;
    __SS_THROW__;
  }

  // fill conversion map based on merge approach

  sourceViewA.print();
  sourceViewB.print();

  if (fillRecordConversionMaps && mergeApproach == "Rename") {
    __COUT__ << "Filling record conversion map." << __E__;

    //	rename		-- All records from both groups are maintained, but conflicts from B are renamed.
    //					Must maintain a map of UIDs that are remapped to new name for groupB,
    //					because linkUID fields must be preserved.

    // for each B record
    //	if there is a conflict, rename

    unsigned int uniqueId;
    std::string uniqueIdString, uniqueIdBase;
    char indexString[1000];
    unsigned int ra;
    unsigned int numericStartIndex;
    bool found;

    for (unsigned int cb = 0; cb < sourceViewB.getNumberOfColumns(); ++cb) {
      // skip columns that are not UID or GroupID columns
      if (!(sourceViewA.getColumnInfo(cb).isUID() || sourceViewA.getColumnInfo(cb).isGroupID())) continue;

      __COUT__ << "Have an ID column: " << cb << " " << sourceViewA.getColumnInfo(cb).getType() << __E__;

      // at this point we have an ID column, verify B and mockup are the same
      if (sourceViewA.getColumnInfo(cb).getType() != sourceViewB.getColumnInfo(cb).getType() ||
          sourceViewA.getColumnInfo(cb).getType() != mockupConfigurationView_.getColumnInfo(cb).getType()) {
        __SS__ << "Error! " << sourceViewA.getColumnInfo(cb).getType() << " column " << cb
               << " of source view A must match source B and destination mock-up view."
               << " Column of source B is [" << sourceViewA.getColumnInfo(cb).getType()
               << "] and of destination mockup is [" << mockupConfigurationView_.getColumnInfo(cb).getType() << "]."
               << __E__;
        __SS_THROW__;
      }

      // getColLinkGroupID(childLinkIndex)

      std::vector<std::string /*converted uidB*/> localConvertedIds;  // used for conflict completeness check

      if (sourceViewA.getColumnInfo(cb).isGroupID()) {
        std::set<std::string> aGroupids = sourceViewA.getSetOfGroupIDs(cb);
        std::set<std::string> bGroupids = sourceViewB.getSetOfGroupIDs(cb);

        for (const auto& bGroupid : bGroupids) {
          if (aGroupids.find(bGroupid) == aGroupids.end()) continue;

          // if here, found conflict
          __COUT__ << "found conflict: " << getConfigurationName() << "/" << bGroupid << __E__;

          // extract starting uniqueId number
          {
            const std::string& str = bGroupid;
            numericStartIndex = str.size();

            // find first non-numeric character
            while (numericStartIndex - 1 < str.size() && str[numericStartIndex - 1] >= '0' &&
                   str[numericStartIndex - 1] <= '9')
              --numericStartIndex;

            if (numericStartIndex < str.size()) {
              uniqueId = atoi(str.substr(numericStartIndex).c_str()) + 1;
              uniqueIdBase = str.substr(0, numericStartIndex);
            } else {
              uniqueId = 0;
              uniqueIdBase = str;
            }

            __COUTV__(uniqueIdBase);
            __COUTV__(uniqueId);
          }  // end //extract starting uniqueId number

          // find unique id string
          {
            sprintf(indexString, "%u", uniqueId);
            uniqueIdString = uniqueIdBase + indexString;
            __COUTV__(uniqueIdString);

            found = false;
            // check converted records and source A and B for conflicts
            if (aGroupids.find(uniqueIdString) != aGroupids.end()) found = true;
            if (!found && bGroupids.find(uniqueIdString) != bGroupids.end()) found = true;
            if (!found && bGroupids.find(uniqueIdString) != bGroupids.end()) found = true;
            for (ra = 0; !found && ra < localConvertedIds.size(); ++ra)
              if (localConvertedIds[ra] == uniqueIdString) found = true;

            while (found)  // while conflict, change id
            {
              ++uniqueId;
              sprintf(indexString, "%u", uniqueId);
              uniqueIdString = uniqueIdBase + indexString;
              __COUTV__(uniqueIdString);

              found = false;
              // check converted records and source A and B for conflicts
              if (aGroupids.find(uniqueIdString) != aGroupids.end()) found = true;
              if (!found && bGroupids.find(uniqueIdString) != bGroupids.end()) found = true;
              if (!found && bGroupids.find(uniqueIdString) != bGroupids.end()) found = true;
              for (ra = 0; !found && ra < localConvertedIds.size(); ++ra)
                if (localConvertedIds[ra] == uniqueIdString) found = true;
            }
          }  // end find unique id string

          // have unique id string now
          __COUTV__(uniqueIdString);

          groupidConversionMap[std::pair<std::string /*original table*/,
                                         std::pair<std::string /*group linkid*/, std::string /*original gidB*/> >(
              getConfigurationName(), std::pair<std::string /*group linkid*/, std::string /*original gidB*/>(
                                          sourceViewB.getColumnInfo(cb).getChildLinkIndex(), bGroupid))] =
              uniqueIdString;
          localConvertedIds.push_back(uniqueIdString);  // save to vector for future conflict checking within table

        }  // end row find unique id string loop for groupid

        // done creating conversion map
        __COUTV__(StringMacros::mapToString(groupidConversionMap));

      }     // end group id conversion map fill
      else  // start uid conversion map fill
      {
        for (unsigned int rb = 0; rb < sourceViewB.getNumberOfRows(); ++rb) {
          found = false;

          for (ra = 0; ra < sourceViewA.getDataView().size(); ++ra)
            if (sourceViewA.getValueAsString(ra, cb) == sourceViewB.getValueAsString(rb, cb)) {
              found = true;
              break;
            }

          if (!found) continue;

          // found conflict
          __COUT__ << "found conflict: " << getConfigurationName() << "/" << sourceViewB.getDataView()[rb][cb] << __E__;

          // extract starting uniqueId number
          {
            const std::string& str = sourceViewB.getDataView()[rb][cb];
            numericStartIndex = str.size();

            // find first non-numeric character
            while (numericStartIndex - 1 < str.size() && str[numericStartIndex - 1] >= '0' &&
                   str[numericStartIndex - 1] <= '9')
              --numericStartIndex;

            if (numericStartIndex < str.size()) {
              uniqueId = atoi(str.substr(numericStartIndex).c_str()) + 1;
              uniqueIdBase = str.substr(0, numericStartIndex);
            } else {
              uniqueId = 0;
              uniqueIdBase = str;
            }

            __COUTV__(uniqueIdBase);
            __COUTV__(uniqueId);
          }  // end //extract starting uniqueId number

          // find unique id string
          {
            sprintf(indexString, "%u", uniqueId);
            uniqueIdString = uniqueIdBase + indexString;
            __COUTV__(uniqueIdString);

            found = false;
            // check converted records and source A and B for conflicts
            for (ra = 0; !found && ra < sourceViewA.getDataView().size(); ++ra)
              if (sourceViewA.getValueAsString(ra, cb) == uniqueIdString) found = true;
            for (ra = 0; !found && ra < sourceViewB.getDataView().size(); ++ra)
              if (ra == rb)
                continue;  // skip record in question
              else if (sourceViewB.getValueAsString(ra, cb) == uniqueIdString)
                found = true;
            for (ra = 0; !found && ra < localConvertedIds.size(); ++ra)
              if (localConvertedIds[ra] == uniqueIdString) found = true;

            while (found)  // while conflict, change id
            {
              ++uniqueId;
              sprintf(indexString, "%u", uniqueId);
              uniqueIdString = uniqueIdBase + indexString;
              __COUTV__(uniqueIdString);

              found = false;
              // check converted records and source A and B for conflicts
              for (ra = 0; !found && ra < sourceViewA.getDataView().size(); ++ra)
                if (sourceViewA.getValueAsString(ra, cb) == uniqueIdString) found = true;
              for (ra = 0; !found && ra < sourceViewB.getDataView().size(); ++ra)
                if (ra == rb)
                  continue;  // skip record in question
                else if (sourceViewB.getValueAsString(ra, cb) == uniqueIdString)
                  found = true;
              for (ra = 0; !found && ra < localConvertedIds.size(); ++ra)
                if (localConvertedIds[ra] == uniqueIdString) found = true;
            }
          }  // end find unique id string

          // have unique id string now
          __COUTV__(uniqueIdString);

          uidConversionMap[std::pair<std::string /*original table*/, std::string /*original uidB*/>(
              getConfigurationName(), sourceViewB.getValueAsString(rb, cb))] = uniqueIdString;
          localConvertedIds.push_back(uniqueIdString);  // save to vector for future conflict checking within table

        }  // end row find unique id string loop

        // done creating conversion map
        __COUTV__(StringMacros::mapToString(uidConversionMap));
      }  /// end uid conversion map

    }  // end column find unique id string loop

  }  // end rename conversion map create
  else
    __COUT__ << "Not filling record conversion map." << __E__;

  if (!applyRecordConversionMaps) {
    __COUT__ << "Not applying record conversion map." << __E__;
    return ConfigurationVersion();  // return invalid
  } else {
    __COUT__ << "Applying record conversion map." << __E__;
    __COUTV__(StringMacros::mapToString(uidConversionMap));
    __COUTV__(StringMacros::mapToString(groupidConversionMap));
  }

  // if destinationVersion is INVALID, creates next available temporary version
  destinationVersion = createTemporaryView(ConfigurationVersion(), destinationVersion);

  __COUT__ << "Merging from (A) " << sourceViewA.getTableName() << "_v" << sourceViewA.getVersion() << " and (B) "
           << sourceViewB.getTableName() << "_v" << sourceViewB.getVersion() << "  to " << getConfigurationName()
           << "_v" << destinationVersion << " with approach '" << mergeApproach << ".'" << __E__;

  // if the merge fails then delete the destinationVersion view
  try {
    // start with a copy of source view A

    ConfigurationView* destinationView =
        &(configurationViews_[destinationVersion].copy(sourceViewA, destinationVersion, author));

    unsigned int destRow, destSize = destinationView->getDataView().size();
    unsigned int cb;
    bool found;
    std::map<std::pair<std::string /*original table*/, std::string /*original uidB*/>,
             std::string /*converted uidB*/>::iterator uidConversionIt;
    std::map<std::pair<std::string /*original table*/,
                       std::pair<std::string /*group linkid*/, std::string /*original gidB*/> >,
             std::string /*converted uidB*/>::iterator groupidConversionIt;

    bool linkIsGroup;
    std::pair<unsigned int /*link col*/, unsigned int /*link id col*/> linkPair;
    std::string strb;
    size_t stri;

    unsigned int colUID = mockupConfigurationView_.getColUID();  // setup UID column

    // handle merger with conflicts consideration
    for (unsigned int rb = 0; rb < sourceViewB.getNumberOfRows(); ++rb) {
      if (mergeApproach == "Rename") {
        //	rename		-- All records from both groups are maintained, but conflicts from B are renamed.
        //					Must maintain a map of UIDs that are remapped to new name for groupB,
        //					because linkUID fields must be preserved.

        // conflict does not matter (because record conversion map is already created, always take and append the B
        // record  copy row from B to new row
        destRow = destinationView->copyRows(author, sourceViewB, rb, 1 /*srcRowsToCopy*/, -1 /*destOffsetRow*/,
                                            generateUniqueDataColumns /*generateUniqueDataColumns*/);

        // check every column and remap conflicting names

        for (cb = 0; cb < sourceViewB.getNumberOfColumns(); ++cb) {
          if (sourceViewB.getColumnInfo(cb).isChildLink())
            continue;  // skip link columns that have table name
          else if (sourceViewB.getColumnInfo(cb).isChildLinkUID()) {
            __COUT__ << "Checking UID link... col=" << cb << __E__;
            sourceViewB.getChildLink(cb, linkIsGroup, linkPair);

            // if table and uid are in conversion map, convert
            if ((uidConversionIt =
                     uidConversionMap.find(std::pair<std::string /*original table*/, std::string /*original uidB*/>(
                         sourceViewB.getValueAsString(rb, linkPair.first),
                         sourceViewB.getValueAsString(rb, linkPair.second)))) != uidConversionMap.end()) {
              __COUT__ << "Found entry to remap: " << sourceViewB.getDataView()[rb][linkPair.second] << " ==> "
                       << uidConversionIt->second << __E__;
              destinationView->setValueAsString(uidConversionIt->second, destRow, linkPair.second);
            }
          } else if (sourceViewB.getColumnInfo(cb).isChildLinkGroupID()) {
            __COUT__ << "Checking GroupID link... col=" << cb << __E__;
            sourceViewB.getChildLink(cb, linkIsGroup, linkPair);

            // if table and uid are in conversion map, convert
            if ((groupidConversionIt = groupidConversionMap.find(
                     std::pair<std::string /*original table*/,
                               std::pair<std::string /*group linkid*/, std::string /*original gidB*/> >(
                         sourceViewB.getValueAsString(rb, linkPair.first),
                         std::pair<std::string /*group linkid*/, std::string /*original gidB*/>(
                             sourceViewB.getColumnInfo(cb).getChildLinkIndex(),
                             sourceViewB.getValueAsString(rb, linkPair.second))))) != groupidConversionMap.end()) {
              __COUT__ << "Found entry to remap: " << sourceViewB.getDataView()[rb][linkPair.second] << " ==> "
                       << groupidConversionIt->second << __E__;
              destinationView->setValueAsString(groupidConversionIt->second, destRow, linkPair.second);
            }
          } else if (sourceViewB.getColumnInfo(cb).isUID()) {
            __COUT__ << "Checking UID... col=" << cb << __E__;
            if ((uidConversionIt =
                     uidConversionMap.find(std::pair<std::string /*original table*/, std::string /*original uidB*/>(
                         getConfigurationName(), sourceViewB.getValueAsString(rb, cb)))) != uidConversionMap.end()) {
              __COUT__ << "Found entry to remap: " << sourceViewB.getDataView()[rb][cb] << " ==> "
                       << uidConversionIt->second << __E__;
              destinationView->setValueAsString(uidConversionIt->second, destRow, cb);
            }
          } else if (sourceViewB.getColumnInfo(cb).isGroupID()) {
            __COUT__ << "Checking GroupID... col=" << cb << __E__;
            if ((groupidConversionIt = groupidConversionMap.find(
                     std::pair<std::string /*original table*/,
                               std::pair<std::string /*group linkid*/, std::string /*original gidB*/> >(
                         getConfigurationName(), std::pair<std::string /*group linkid*/, std::string /*original gidB*/>(
                                                     sourceViewB.getColumnInfo(cb).getChildLinkIndex(),
                                                     sourceViewB.getValueAsString(rb, cb))))) !=
                groupidConversionMap.end()) {
              __COUT__ << "Found entry to remap: " << sourceViewB.getDataView()[rb][cb] << " ==> "
                       << groupidConversionIt->second << __E__;
              destinationView->setValueAsString(groupidConversionIt->second, destRow, cb);
            }
          } else {
            // look for text link to a Table/UID in the map
            strb = sourceViewB.getValueAsString(rb, cb);
            if (strb.size() > getConfigurationName().size() + 2 && strb[0] == '/') {
              // check for linked name
              __COUT__ << "Checking col" << cb << " " << strb << __E__;

              // see if there is an entry in p
              for (const auto& mapPairToPair : uidConversionMap) {
                if ((stri = strb.find(mapPairToPair.first.first + "/" + mapPairToPair.first.second)) !=
                    std::string::npos) {
                  __COUT__ << "Found a text link match (stri=" << stri << ")! "
                           << (mapPairToPair.first.first + "/" + mapPairToPair.first.second) << " ==> "
                           << mapPairToPair.second << __E__;

                  // insert mapped substitution into string
                  destinationView->setValueAsString(
                      strb.substr(0, stri) + (mapPairToPair.first.first + "/" + mapPairToPair.first.second) +
                          strb.substr(stri + (mapPairToPair.first.first + "/" + mapPairToPair.first.second).size()),
                      destRow, cb);

                  __COUT__ << "Found entry to remap: " << sourceViewB.getDataView()[rb][cb] << " ==> "
                           << destinationView->getDataView()[destRow][cb] << __E__;
                  break;
                }
              }  // end uid conversion map loop
            }
          }
        }  // end column loop over B record

        continue;
      }  // end rename, no-conflict handling

      // if here, then not doing rename, so conflicts matter

      found = false;

      for (destRow = 0; destRow < destSize; ++destRow)
        if (destinationView->getValueAsString(destRow, colUID) == sourceViewB.getValueAsString(rb, colUID)) {
          found = true;
          break;
        }
      if (!found)  // no conflict
      {
        __COUT__ << "No " << mergeApproach << " conflict: " << __E__;

        if (mergeApproach == "replace" || mergeApproach == "skip") {
          // no conflict so append the B record
          // copy row from B to new row
          destinationView->copyRows(author, sourceViewB, rb, 1 /*srcRowsToCopy*/);
        } else

          continue;
      }  // end no-conflict handling

      // if here, then there was a conflict

      __COUT__ << "found " << mergeApproach << " conflict: " << sourceViewB.getDataView()[rb][colUID] << __E__;

      if (mergeApproach == "replace") {
        //	replace		-- Any UID conflicts for a record are replaced by the record from group B.

        // delete row in destination
        destinationView->deleteRow(destRow--);  // delete row and back up pointer
        --destSize;

        // append the B record now
        // copy row from B to new row
        destinationView->copyRows(author, sourceViewB, rb, 1 /*srcRowsToCopy*/);
      }
      // else if (mergeApproach == "skip") then do nothing with conflicting B record
    }

    destinationView->print();

  } catch (...)  // if the copy fails then delete the destinationVersion view
  {
    __COUT_ERR__ << "Failed to merge " << sourceViewA.getTableName() << "_v" << sourceViewA.getVersion() << " and "
                 << sourceViewB.getTableName() << "_v" << sourceViewB.getVersion() << " into " << getConfigurationName()
                 << "_v" << destinationVersion << __E__;
    __COUT_WARN__ << "Deleting the failed destination version " << destinationVersion << __E__;
    eraseView(destinationVersion);
    throw;  // and rethrow
  }

  return destinationVersion;
}  // end mergeViews

//==============================================================================
// copyView
//	copies source view (including version) and places in self
//	as destination temporary version.
//	if destination version is invalid, then next available temporary version is chosen
//	if conflict, throw exception
//
//	Returns version of new temporary view that was created.
ConfigurationVersion ConfigurationBase::copyView(const ConfigurationView& sourceView,
                                                 ConfigurationVersion destinationVersion, const std::string& author) {
  // check that column sizes match
  if (sourceView.getNumberOfColumns() != mockupConfigurationView_.getNumberOfColumns()) {
    __SS__ << "Error! Number of Columns of source view must match destination mock-up view."
           << "Dimension of source is [" << sourceView.getNumberOfColumns() << "] and of destination mockup is ["
           << mockupConfigurationView_.getNumberOfColumns() << "]." << __E__;
    __SS_THROW__;
  }

  // check for destination version confict
  if (!destinationVersion.isInvalid() && configurationViews_.find(destinationVersion) != configurationViews_.end()) {
    __SS__ << "Error! Asked to copy a view with a conflicting version: " << destinationVersion << __E__;
    __SS_THROW__;
  }

  // if destinationVersion is INVALID, creates next available temporary version
  destinationVersion = createTemporaryView(ConfigurationVersion(), destinationVersion);

  __COUT__ << "Copying from " << sourceView.getTableName() << "_v" << sourceView.getVersion() << " to "
           << getConfigurationName() << "_v" << destinationVersion << __E__;

  try {
    configurationViews_[destinationVersion].copy(sourceView, destinationVersion, author);
  } catch (...)  // if the copy fails then delete the destinationVersion view
  {
    __COUT_ERR__ << "Failed to copy from " << sourceView.getTableName() << "_v" << sourceView.getVersion() << " to "
                 << getConfigurationName() << "_v" << destinationVersion << __E__;
    __COUT_WARN__ << "Deleting the failed destination version " << destinationVersion << __E__;
    eraseView(destinationVersion);
    throw;  // and rethrow
  }

  return destinationVersion;
}  // end copyView()

//==============================================================================
// createTemporaryView
//	-1, from MockUp, else from valid view version
//	destTemporaryViewVersion is starting point for search for available temporary versions.
//	if destTemporaryViewVersion is invalid, starts search at ConfigurationVersion::getNextTemporaryVersion().
// 	returns new temporary version number (which is always negative)
ConfigurationVersion ConfigurationBase::createTemporaryView(ConfigurationVersion sourceViewVersion,
                                                            ConfigurationVersion destTemporaryViewVersion) {
  __COUT__ << "Configuration: " << getConfigurationName() << __E__;

  __COUT__ << "Num of Views: " << configurationViews_.size()
           << " (Temporary Views: " << (configurationViews_.size() - getNumberOfStoredViews()) << ")" << __E__;

  ConfigurationVersion tmpVersion = destTemporaryViewVersion;
  if (tmpVersion.isInvalid()) tmpVersion = ConfigurationVersion::getNextTemporaryVersion();
  while (isStored(tmpVersion) &&  // find a new valid temporary version
         !(tmpVersion = ConfigurationVersion::getNextTemporaryVersion(tmpVersion)).isInvalid())
    ;
  if (isStored(tmpVersion) || tmpVersion.isInvalid()) {
    __SS__ << "Invalid destination temporary version: " << destTemporaryViewVersion
           << ". Expected next temporary version < " << tmpVersion << __E__;
    __COUT_ERR__ << ss.str();
    __SS_THROW__;
  }

  if (sourceViewVersion == ConfigurationVersion::INVALID ||  // use mockup if sourceVersion is -1 or not found
      configurationViews_.find(sourceViewVersion) == configurationViews_.end()) {
    if (sourceViewVersion != -1) {
      __SS__ << "ERROR: sourceViewVersion " << sourceViewVersion << " not found. "
             << "Invalid source version. Version requested is not stored (yet?) or does not exist." << __E__;
      __COUT_ERR__ << ss.str();
      __SS_THROW__;
    }
    __COUT__ << "Using Mock-up view" << __E__;
    configurationViews_[tmpVersion].copy(mockupConfigurationView_, tmpVersion, mockupConfigurationView_.getAuthor());
  } else {
    try  // do not allow init to throw an exception here..
    {    // it's ok to copy invalid data, the user may be trying to change it
      configurationViews_[tmpVersion].copy(configurationViews_[sourceViewVersion], tmpVersion,
                                           configurationViews_[sourceViewVersion].getAuthor());
    } catch (...) {
      __COUT_WARN__ << "createTemporaryView() Source view failed init(). "
                    << "This is being ignored (hopefully the new copy is being fixed)." << __E__;
    }
  }

  return tmpVersion;
}  // end createTemporaryView()

//==============================================================================
// getNextAvailableTemporaryView
//	ConfigurationVersion::INVALID is always MockUp
// returns next available temporary version number (which is always negative)
ConfigurationVersion ConfigurationBase::getNextTemporaryVersion() const {
  ConfigurationVersion tmpVersion;

  // std::map guarantees versions are in increasing order!
  if (configurationViews_.size() != 0 && configurationViews_.begin()->first.isTemporaryVersion())
    tmpVersion = ConfigurationVersion::getNextTemporaryVersion(configurationViews_.begin()->first);
  else
    tmpVersion = ConfigurationVersion::getNextTemporaryVersion();

  // verify tmpVersion is ok
  if (isStored(tmpVersion) || tmpVersion.isInvalid() || !tmpVersion.isTemporaryVersion()) {
    __SS__ << "Invalid destination temporary version: " << tmpVersion << __E__;
    __COUT_ERR__ << ss.str();
    __SS_THROW__;
  }
  return tmpVersion;
}

//==============================================================================
// getNextVersion
// 	returns next available new version
//	the implication is any version number equal or greater is available.
ConfigurationVersion ConfigurationBase::getNextVersion() const {
  ConfigurationVersion tmpVersion;

  // std::map guarantees versions are in increasing order!
  if (configurationViews_.size() != 0 && !configurationViews_.rbegin()->first.isTemporaryVersion())
    tmpVersion = ConfigurationVersion::getNextVersion(configurationViews_.rbegin()->first);
  else
    tmpVersion = ConfigurationVersion::getNextVersion();

  // verify tmpVersion is ok
  if (isStored(tmpVersion) || tmpVersion.isInvalid() || tmpVersion.isTemporaryVersion()) {
    __SS__ << "Invalid destination next version: " << tmpVersion << __E__;
    __COUT_ERR__ << ss.str();
    __SS_THROW__;
  }
  return tmpVersion;
}

//==============================================================================
// getTemporaryView
//	must be a valid temporary version, and the view must be stored in configuration.
// 	temporary version indicates it has not been saved to database and assigned a version number
ConfigurationView* ConfigurationBase::getTemporaryView(ConfigurationVersion temporaryVersion) {
  if (!temporaryVersion.isTemporaryVersion() || !isStored(temporaryVersion)) {
    __SS__ << getConfigurationName() << ":: Error! Temporary version not found!" << __E__;
    __COUT_ERR__ << ss.str();
    __SS_THROW__;
  }
  return &configurationViews_[temporaryVersion];
}

//==============================================================================
// convertToCaps
//	static utility for converting configuration and column names to the caps version
//	throw std::runtime_error if not completely alpha-numeric input
std::string ConfigurationBase::convertToCaps(std::string& str, bool isConfigName) {
  // append Configuration to be nice to user
  unsigned int configPos = (unsigned int)std::string::npos;
  if (isConfigName && (configPos = str.find("Configuration")) != str.size() - strlen("Configuration"))
    str += "Configuration";

  // create all caps name and validate
  //	only allow alpha names with Configuration at end
  std::string capsStr = "";
  for (unsigned int c = 0; c < str.size(); ++c)
    if (str[c] >= 'A' && str[c] <= 'Z') {
      // add _ before configuration and if lower case to uppercase
      if (c == configPos || (c && str[c - 1] >= 'a' && str[c - 1] <= 'z') ||  // if this is a new start of upper case
          (c && str[c - 1] >= 'A' && str[c - 1] <= 'Z' &&  // if this is a new start from running caps
           c + 1 < str.size() && str[c + 1] >= 'a' && str[c + 1] <= 'z'))
        capsStr += "_";
      capsStr += str[c];
    } else if (str[c] >= 'a' && str[c] <= 'z')
      capsStr += char(str[c] - 32);  // capitalize
    else if (str[c] >= '0' && str[c] <= '9')
      capsStr += str[c];  // allow numbers
    else                  // error! non-alpha
      __THROW__(std::string("ConfigurationBase::convertToCaps::") +
                "Invalid character found in name (allowed: A-Z, a-z, 0-9):" + str);

  return capsStr;
}
