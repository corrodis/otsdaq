#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include <string.h>   //for strlen
#include <stdexcept>  //for runtime_error

using namespace ots;

const unsigned int ConfigurationGroupKey::INVALID = -1;
const unsigned int ConfigurationGroupKey::DEFAULT = 0;

//==============================================================================
ConfigurationGroupKey::ConfigurationGroupKey(unsigned int key) : key_(key) {}

//==============================================================================
// groupString parameter can be the full group name, or just the group key
ConfigurationGroupKey::ConfigurationGroupKey(char* const& groupString) {
  if (!groupString) {
    key_ = ConfigurationGroupKey::INVALID;
    return;
  }

  // find last character that is not part of key
  //	key consists of numeric, dash, underscore, and period
  int i = strlen(groupString) - 1;
  for (; i >= 0; --i)
    if ((groupString[i] < '0' || groupString[i] > '9') && groupString[i] != '-' && groupString[i] != '_' &&
        groupString[i] != '.')
      break;  // not part of key,... likely a 'v' if using "_v" syntax for version

  if (i == (int)strlen(groupString) - 1)  // no key characters found
  {
    key_ = ConfigurationGroupKey::DEFAULT;
    return;
  } else if (i < 0)  // only key characters found, so assume group key string was given
    i = 0;
  else
    ++i;

  // at this point, i is start of key sequence
  sscanf(&groupString[i], "%u", &key_);
}

//==============================================================================
ConfigurationGroupKey::ConfigurationGroupKey(const std::string& groupString)
    : ConfigurationGroupKey((char*)groupString.c_str()) {}

//==============================================================================
ConfigurationGroupKey::~ConfigurationGroupKey(void) {}

//==============================================================================
unsigned int ConfigurationGroupKey::key(void) const { return key_; }

//==============================================================================
// operator==
bool ConfigurationGroupKey::operator==(unsigned int key) const { return (key_ == key); }
bool ConfigurationGroupKey::operator==(const ConfigurationGroupKey& key) const { return (key_ == key.key_); }

//==============================================================================
// toString
std::string ConfigurationGroupKey::toString(void) const {
  // represent invalid/temporary versions as negative number strings
  return (isInvalid()) ? std::to_string((int)key_) : std::to_string(key_);
}

//==============================================================================
// assignment operator with type int
ConfigurationGroupKey& ConfigurationGroupKey::operator=(const unsigned int key) {
  key_ = key;
  return *this;
}

//==============================================================================
bool ConfigurationGroupKey::operator!=(unsigned int key) const { return (key_ != key); }
bool ConfigurationGroupKey::operator!=(const ConfigurationGroupKey& key) const { return (key_ != key.key_); }

//==============================================================================
// operator<
bool ConfigurationGroupKey::operator<(const ConfigurationGroupKey& key) const { return (key_ < key.key_); }

//==============================================================================
// operator>
bool ConfigurationGroupKey::operator>(const ConfigurationGroupKey& key) const { return (key_ > key.key_); }

//==============================================================================
// isInvalid
bool ConfigurationGroupKey::isInvalid() const { return (key_ == INVALID); }

//==============================================================================
// getNextKey
//	returns next key given the most recent key
//		if given nothing returns DEFAULT as first key
//		if given 0, returns 1, etc.
//	if no available keys left return INVALID
ConfigurationGroupKey ConfigurationGroupKey::getNextKey(const ConfigurationGroupKey& key) {
  ConfigurationGroupKey retKey(key.key_ + 1);  // DEFAULT := INVALID + 1
  return retKey;                               // if retKey is invalid, then INVALID is returned already
}

//==============================================================================
const unsigned int ConfigurationGroupKey::getDefaultKey(void) { return DEFAULT; }

//==============================================================================
const unsigned int ConfigurationGroupKey::getInvalidKey(void) { return INVALID; }

//==============================================================================
// getGroupNameWithKey
//	returns next key given the most recent key
//		if given nothing returns DEFAULT as first key
//		if given 0, returns 1, etc.
//	if no available keys left return INVALID
std::string ConfigurationGroupKey::getFullGroupString(const std::string& groupName, const ConfigurationGroupKey& key) {
  if (groupName.size() == 0) {
    __SS__ << ("ConfigurationGroupKey::getFullGroupString() Illegal Group Name! The Group Name was not provided.\n");
    __COUT_ERR__ << ss.str();
    __SS_THROW__;
  } else if (groupName.size() == 1) {
    __SS__ << ("ConfigurationGroupKey::getFullGroupString() Illegal Group Name! The Group Name is too short: \"" +
               groupName + "\"")
           << std::endl;
    __COUT_ERR__ << ss.str();
    __SS_THROW__;
  } else {
    for (unsigned int i = 0; i < groupName.size(); ++i) {
      if (!(  // alphaNumeric
              (groupName[i] >= 'A' && groupName[i] <= 'Z') || (groupName[i] >= 'a' && groupName[i] <= 'z') ||
              (groupName[i] >= '0' && groupName[i] <= '9'))) {
        __SS__
            << ("ConfigurationGroupKey::getFullGroupString() Illegal Group Name! Group Name must be alpha-numeric: \"" +
                groupName + "\"")
            << std::endl;
        __COUT_ERR__ << ss.str();
        __SS_THROW__;
      }
    }
  }

  return groupName + "_v" + std::to_string(key.key_);
}

//==============================================================================
void ConfigurationGroupKey::getGroupNameAndKey(const std::string& fullGroupString, std::string& groupName,
                                               ConfigurationGroupKey& key) {
  auto i = fullGroupString.rfind("_v");
  groupName = fullGroupString.substr(0, i);
  key = ConfigurationGroupKey(fullGroupString.substr(i + 2));
}
