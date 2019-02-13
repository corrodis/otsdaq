#include "otsdaq-core/ConfigurationDataFormats/ConfigurationVersion.h"

using namespace ots;

const unsigned int ConfigurationVersion::INVALID = -1;
const unsigned int ConfigurationVersion::DEFAULT = 0;
const unsigned int ConfigurationVersion::SCRATCH = ~(1 << 31);  // highest positive integer

//==============================================================================
ConfigurationVersion::ConfigurationVersion(unsigned int version) : version_(version) {}
ConfigurationVersion::ConfigurationVersion(char* const& versionStr) {
  if (!versionStr)
    version_ = ConfigurationVersion::INVALID;
  else
    sscanf(versionStr, "%u", &version_);
}
ConfigurationVersion::ConfigurationVersion(const std::string& versionStr)
    : ConfigurationVersion((char*)versionStr.c_str()) {}

//==============================================================================
ConfigurationVersion::~ConfigurationVersion(void) {}

//==============================================================================
// version
//	get version as integer
unsigned int ConfigurationVersion::version(void) const { return version_; }

//==============================================================================
// toString
std::string ConfigurationVersion::toString(void) const {
  // represent invalid/temporary versions as negative number strings
  return (isTemporaryVersion() || isInvalid()) ? std::to_string((int)version_) : std::to_string(version_);
}

//==============================================================================
// assignment operator with type int
ConfigurationVersion& ConfigurationVersion::operator=(const unsigned int version) {
  version_ = version;
  return *this;
}

//==============================================================================
// operator==
bool ConfigurationVersion::operator==(unsigned int version) const { return (version_ == version); }
bool ConfigurationVersion::operator==(const ConfigurationVersion& version) const {
  return (version_ == version.version_);
}

//==============================================================================
// operator!=
bool ConfigurationVersion::operator!=(unsigned int version) const { return (version_ != version); }
bool ConfigurationVersion::operator!=(const ConfigurationVersion& version) const {
  return (version_ != version.version_);
}

//==============================================================================
// operator<
bool ConfigurationVersion::operator<(const ConfigurationVersion& version) const {
  return (version_ < version.version_);
}

//==============================================================================
// operator>
bool ConfigurationVersion::operator>(const ConfigurationVersion& version) const {
  return (version_ > version.version_);
}

//==============================================================================
// isInvalid
bool ConfigurationVersion::isInvalid() const {
  return (version_ == ConfigurationVersion::INVALID ||
          (version_ > ConfigurationVersion::SCRATCH &&
           version_ < INVALID - NUM_OF_TEMP_VERSIONS));  // unused reserved block
}

//==============================================================================
// isTemporaryVersion
//	claim a block of unsigned ints to designate as temporary versions
bool ConfigurationVersion::isTemporaryVersion() const {
  return (version_ >= INVALID - NUM_OF_TEMP_VERSIONS && version_ != INVALID);
}

//==============================================================================
// isScratchVersion
//	claim the most positive signed integer as the scratch version
bool ConfigurationVersion::isScratchVersion() const { return (version_ == ConfigurationVersion::SCRATCH); }

//==============================================================================
// isMockupVersion
//	the INVALID index represents the mockup version
bool ConfigurationVersion::isMockupVersion() const { return (version_ == ConfigurationVersion::INVALID); }

//==============================================================================
// getNextVersion
//	returns next version given the most recent version
//		if given nothing returns DEFAULT as first version
//		if given 0, returns 1, etc.
//	if no available versions left return INVALID
ConfigurationVersion ConfigurationVersion::getNextVersion(const ConfigurationVersion& version) {
  ConfigurationVersion retVersion(version.version_ + 1);  // DEFAULT := INVALID + 1
  return (!retVersion.isInvalid() && !retVersion.isTemporaryVersion()) ? retVersion : ConfigurationVersion();
}

//==============================================================================
// getNextTemporaryVersion
//	returns next Temporary version given the most recent temporary version
//		if given nothing returns -2 as first temporary version
//		if given -2, returns -3, etc.
//	if no available temporary versions left return INVALID
ConfigurationVersion ConfigurationVersion::getNextTemporaryVersion(const ConfigurationVersion& version) {
  ConfigurationVersion retVersion(version.isTemporaryVersion() ? (version.version_ - 1) : (INVALID - 1));
  return retVersion.isTemporaryVersion() ? retVersion : ConfigurationVersion();
}
