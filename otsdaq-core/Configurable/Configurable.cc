#include "otsdaq-core/Configurable/Configurable.h"

#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"


using namespace ots;

//==============================================================================
Configurable::Configurable(const ConfigurationTree& theXDAQContextConfigTree, const std::string& theConfigurationPath)
: theXDAQContextConfigTree_		(theXDAQContextConfigTree)
, theConfigurationPath_    		(theConfigurationPath)
, theConfigurationRecordName_	(theXDAQContextConfigTree_.getNode(theConfigurationPath_).getValueAsString())
{
	__CFG_COUT__ << " Configurable class constructed. " << __E__;
}

//==============================================================================
Configurable::~Configurable(void)
{}

//==============================================================================
ConfigurationTree Configurable::getSelfNode() const
{
	return theXDAQContextConfigTree_.getNode(theConfigurationPath_);
}

//==============================================================================
const ConfigurationManager* Configurable::getConfigurationManager() const
{
	return theXDAQContextConfigTree_.getConfigurationManager();
}

//==============================================================================
const std::string& Configurable::getContextUID() const
{
	return theXDAQContextConfigTree_.getForwardNode(
			theConfigurationPath_,1 /*steps to xdaq node*/).getValueAsString();
}

//==============================================================================
const std::string& Configurable::getApplicationUID() const
{		
	return theXDAQContextConfigTree_.getForwardNode(
			theConfigurationPath_,3 /*steps to app node*/).getValueAsString();
}

//==============================================================================
unsigned int Configurable::getApplicationLID() const
{
	const XDAQContextConfiguration* contextConfig =
			getConfigurationManager()->__GET_CONFIG__(XDAQContextConfiguration);

	return contextConfig->getApplicationNode(
			getConfigurationManager(),
			getContextUID(),
			getApplicationUID()).getNode(
					contextConfig->colApplication_.colId_).getValue<unsigned int>();
}

//==============================================================================
std::string Configurable::getContextAddress() const
{
	const XDAQContextConfiguration* contextConfig =
			getConfigurationManager()->__GET_CONFIG__(XDAQContextConfiguration);

	return contextConfig->getContextNode(
			getConfigurationManager(),
			getContextUID()).getNode(
					contextConfig->colContext_.colAddress_).getValue<std::string>();
}

//==============================================================================
unsigned int Configurable::getContextPort() const
{
	const XDAQContextConfiguration* contextConfig =
			getConfigurationManager()->__GET_CONFIG__(XDAQContextConfiguration);

	return contextConfig->getContextNode(
			getConfigurationManager(),
			getContextUID()).getNode(
					contextConfig->colContext_.colPort_).getValue<unsigned int>();
}
