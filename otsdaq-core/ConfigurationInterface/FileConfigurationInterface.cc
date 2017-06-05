#include "otsdaq-core/ConfigurationInterface/FileConfigurationInterface.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationHandler.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include <iostream>
#include <set>
#include <dirent.h>
#include <errno.h>
#include "JSONConfigurationHandler.h.unused"

using namespace ots;


//==============================================================================
void FileConfigurationInterface::fill(ConfigurationBase* configuration, ConfigurationVersion version) const
{
    ConfigurationHandler::readXML (configuration, version);
}

//==============================================================================
//findLatestVersion
//	return INVALID if no existing versions
ConfigurationVersion FileConfigurationInterface::findLatestVersion(const ConfigurationBase* configuration) const
{
	auto versions = getVersions(configuration);
	if(!versions.size())
		return ConfigurationVersion(); //return INVALID
	return *(versions.rbegin());
}

//==============================================================================
// save active configuration view to file
void  FileConfigurationInterface::saveActiveVersion(const ConfigurationBase* configuration, bool overwrite) const
{
	ConfigurationHandler::writeXML(configuration);
}


//==============================================================================
std::set<ConfigurationVersion> FileConfigurationInterface::getVersions(const ConfigurationBase* configuration) const
{
    std::string configDir = ConfigurationHandler::getXMLDir(configuration);
    std::cout << __COUT_HDR_FL__ << "ConfigurationDir: " << configDir << std::endl;
    DIR *dp;

    struct dirent *dirp;

    if((dp = opendir(configDir.c_str())) == 0)
    {
        std::cout << __COUT_HDR_FL__<< "ERROR:(" << errno << ").  Can't open directory: " << configDir << std::endl;
        throw std::runtime_error("Error in File Interface getVersion!");
    }

    const unsigned char isDir = 0x4;
    //const std::string        pDir  = ".";
    //const std::string        ppDir = "..";
    //int                 dirVersion;
    std::string::const_iterator it;
    std::string                 dirName;
    std::set<ConfigurationVersion>               dirNumbers;

    while ((dirp = readdir(dp)) != 0)
    {
        if(dirp->d_type == isDir && dirp->d_name[0] != '.' )
        {
            dirName = dirp->d_name;
            //Check if there are non numeric directories
            it = dirName.begin();

            while (it != dirName.end() && std::isdigit(*it))
                ++it;

            if(dirName.empty() || it != dirName.end())
            {
                std::cout << __COUT_HDR_FL__ << "WARNING: there is a non numeric directory in " << configDir << " called " << dirName
                << " please remove it from there since only numeric directories are considered." << std::endl;
                continue;
            }

            dirNumbers.insert(ConfigurationVersion(strtol(dirp->d_name, 0 , 10)));
        }
    }
    closedir(dp);
    return dirNumbers;
}

