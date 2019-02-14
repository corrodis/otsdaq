#define BOOST_TEST_MODULE (databaseconfiguration test)

#include "boost/test/auto_unit_test.hpp"

#include <dirent.h>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
//#include <otsdaq_demo/otsdaq-demo/FEInterfaces/FEWROtsUDPFSSRInterface.h>
//#include <otsdaq_demo/otsdaq-demo/UserConfigurationDataFormats/FEWROtsUDPFSSRInterfaceConfiguration.h>
#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/Configurations.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/ConfigurationAliases.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/FEConfiguration.h"
#include "artdaq-database/JsonDocument/JSONDocument.h"
#include "artdaq-database/StorageProviders/FileSystemDB/provider_filedb_index.h"
#include "otsdaq-core/PluginMakers/MakeInterface.h"
#include "otsdaq-core/PluginMakers/MakeInterfaceConfiguration.h"

using namespace ots;

BOOST_AUTO_TEST_SUITE(databaseconfiguration_test)

BOOST_AUTO_TEST_CASE(readxml_writedb_configurations)
{
	//artdaq::database::filesystem::index::debug::enable();
	//artdaq::database::jsonutils::debug::enableJSONDocument();

	std::vector<std::string> configTables;

	//normally CONFIGURATION_TYPE is set by StartOTS.sh
	setenv("CONFIGURATION_DATA_PATH", (std::string(getenv("USER_DATA")) + "/ConfigurationDataExamples").c_str(), 1);
	std::string configDir = std::string(getenv("CONFIGURATION_DATA_PATH")) + '/';

	//CONFIGURATION_TYPE needed by otsdaq/otsdaq-core/ConfigurationDataFormats/ConfigurationInfoReader.cc [187]
	//Can be File, Database, DatabaseTest
	setenv("CONFIGURATION_TYPE", "File", 1);

	//add configurations to vector list from directory
	{
		std::cout << __COUT_HDR_FL__ << "ConfigurationDir: " << configDir << std::endl;
		DIR* dp;

		struct dirent* dirp;

		if ((dp = opendir(configDir.c_str())) == 0)
		{
			std::cout << __COUT_HDR_FL__ << "ERROR:(" << errno << ").  Can't open directory: " << configDir << std::endl;
			exit(0);
		}

		const unsigned char isDir = 0x4;
		while ((dirp = readdir(dp)) != 0)
			if (dirp->d_type == isDir && dirp->d_name[0] != '.')
			{
				std::cout << __COUT_HDR_FL__ << dirp->d_name << std::endl;
				configTables.push_back(dirp->d_name);
			}

		closedir(dp);
	}

	ConfigurationInterface* theInterface_ = ConfigurationInterface::getInstance(true);

	for (unsigned int i = 0; i < configTables.size(); ++i)
	{
		theInterface_		= ConfigurationInterface::getInstance(true);
		ConfigurationBase* base = 0;
		std::cout << __COUT_HDR_FL__ << std::endl;
		std::cout << __COUT_HDR_FL__ << std::endl;
		std::cout << __COUT_HDR_FL__ << (i + 1) << " of " << configTables.size() << ": " << configTables[i] << std::endl;

		theInterface_->get(base, configTables[i], 0, 0, false,
				   ConfigurationVersion(ConfigurationVersion::DEFAULT));  //load version 0 for all

		std::cout << __COUT_HDR_FL__ << "loaded " << configTables[i] << std::endl;

		//if(configTables[i]  != "ARTDAQAggregatorConfiguration") continue;

		//save the active version
		std::cout << __COUT_HDR_FL__ << "Current version: " << base->getViewVersion() << std::endl;

		//
		//		**** switch to db style interface?!!?!? ****   //
		//
		theInterface_ = ConfigurationInterface::getInstance(false);
		//
		//

		//theInterface_->saveActiveVersion(base); //saves current version

		ConfigurationVersion tmpView = base->createTemporaryView(ConfigurationVersion(ConfigurationVersion::DEFAULT));
		theInterface_->saveNewVersion(base, tmpView);

		delete base;  //cleanup config instance

		//break;
	}

	std::cout << __COUT_HDR_FL__ << "end of debugging Configuration!" << std::endl;
	return;
}

BOOST_AUTO_TEST_CASE(readdb_writexml_configurations)
{
	//return;
	std::vector<std::string> configTables;

	//normally CONFIGURATION_TYPE is set by StartOTS.sh
	setenv("CONFIGURATION_DATA_PATH", (std::string(getenv("USER_DATA")) + "/ConfigurationDataExamples").c_str(), 1);
	std::string configDir = std::string(getenv("CONFIGURATION_DATA_PATH")) + '/';

	//CONFIGURATION_TYPE needed by otsdaq/otsdaq-core/ConfigurationDataFormats/ConfigurationInfoReader.cc [187]
	//Can be File, Database, DatabaseTest
	setenv("CONFIGURATION_TYPE", "File", 1);

	//add configurations to vector list from directory
	{
		std::cout << __COUT_HDR_FL__ << "ConfigurationDir: " << configDir << std::endl;
		DIR* dp;

		struct dirent* dirp;

		if ((dp = opendir(configDir.c_str())) == 0)
		{
			std::cout << __COUT_HDR_FL__ << "ERROR:(" << errno << ").  Can't open directory: " << configDir << std::endl;
			exit(0);
		}

		const unsigned char isDir = 0x4;
		while ((dirp = readdir(dp)) != 0)
			if (dirp->d_type == isDir && dirp->d_name[0] != '.')
			{
				std::cout << __COUT_HDR_FL__ << dirp->d_name << std::endl;
				configTables.push_back(dirp->d_name);
			}

		closedir(dp);
	}

	ConfigurationInterface* theInterface_ = ConfigurationInterface::getInstance(false);

	for (unsigned int i = 0; i < configTables.size(); ++i)
	{
		theInterface_		= ConfigurationInterface::getInstance(false);
		ConfigurationBase* base = 0;
		std::cout << __COUT_HDR_FL__ << std::endl;
		std::cout << __COUT_HDR_FL__ << std::endl;
		std::cout << __COUT_HDR_FL__ << (i + 1) << " of " << configTables.size() << ": " << configTables[i] << std::endl;

		theInterface_->get(base, configTables[i], 0, 0, false, ConfigurationVersion(ConfigurationVersion::DEFAULT));  //load version 0 for all

		std::cout << __COUT_HDR_FL__ << "loaded " << configTables[i] << std::endl;

		//save the active version
		std::cout << __COUT_HDR_FL__ << "Current version: " << base->getViewVersion() << std::endl;

		//
		//		**** switch to db style interface?!!?!? ****   //
		//
		//theInterface_ = ConfigurationInterface::getInstance(true);
		//
		//

		ConfigurationVersion tmpView = base->createTemporaryView(ConfigurationVersion(ConfigurationVersion::DEFAULT));
		theInterface_->saveNewVersion(base, tmpView);

		delete base;  //cleanup config instance
			      //break;
	}

	std::cout << __COUT_HDR_FL__ << "end of debugging Configuration!" << std::endl;
	return;
}

BOOST_AUTO_TEST_SUITE_END()
