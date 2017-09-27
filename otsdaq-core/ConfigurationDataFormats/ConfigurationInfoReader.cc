#include "otsdaq-core/ConfigurationDataFormats/ConfigurationInfoReader.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/XmlUtilities/DOMTreeErrorReporter.h"
#include "otsdaq-core/XmlUtilities/ConvertFromXML.h"
//#include "TimeFormatter.h"


#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
//#include <xercesc/dom/DOMWriter.hpp>

#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>

#include <stdexcept>
#include <iostream>
#include <sstream>

#include <sys/stat.h>
#include <errno.h>

using namespace ots;

#undef 	__COUT_HDR__
#define __COUT_HDR__ 	"ConfigInfoReader"


//==============================================================================
ConfigurationInfoReader::ConfigurationInfoReader(bool allowIllegalColumns)
: allowIllegalColumns_(allowIllegalColumns)
{
	initPlatform();
	rootTag_                       = xercesc::XMLString::transcode("ROOT");
	configurationTag_              = xercesc::XMLString::transcode("CONFIGURATION");
	configurationNameAttributeTag_ = xercesc::XMLString::transcode("Name");
	viewTag_                       = xercesc::XMLString::transcode("VIEW");
	viewNameAttributeTag_          = xercesc::XMLString::transcode("Name");
	viewTypeAttributeTag_          = xercesc::XMLString::transcode("Type");
	viewDescriptionAttributeTag_   = xercesc::XMLString::transcode("Description");
	columnTag_                     = xercesc::XMLString::transcode("COLUMN");
	columnTypeAttributeTag_        = xercesc::XMLString::transcode("Type");
	columnNameAttributeTag_        = xercesc::XMLString::transcode("Name");
	columnStorageNameAttributeTag_ = xercesc::XMLString::transcode("StorageName");
	columnDataTypeAttributeTag_    = xercesc::XMLString::transcode("DataType");
	columnDataChoicesAttributeTag_ = xercesc::XMLString::transcode("DataChoices");
}

//==============================================================================
ConfigurationInfoReader::~ConfigurationInfoReader(void)
{
	try
	{
		xercesc::XMLString::release( &rootTag_                       );
		xercesc::XMLString::release( &configurationTag_              );
		xercesc::XMLString::release( &configurationNameAttributeTag_ );
		xercesc::XMLString::release( &viewTag_                       );
		xercesc::XMLString::release( &viewNameAttributeTag_          );
		xercesc::XMLString::release( &viewTypeAttributeTag_          );
		xercesc::XMLString::release( &viewDescriptionAttributeTag_   );
		xercesc::XMLString::release( &columnTag_                     );
		xercesc::XMLString::release( &columnTypeAttributeTag_        );
		xercesc::XMLString::release( &columnNameAttributeTag_        );
		xercesc::XMLString::release( &columnStorageNameAttributeTag_ );
		xercesc::XMLString::release( &columnDataTypeAttributeTag_    );
		xercesc::XMLString::release( &columnDataChoicesAttributeTag_ );

	}
	catch( ... )
	{
		mf::LogError(__FILE__) << "Unknown exception encountered in TagNames destructor" << std::endl;
	}
	terminatePlatform();
}

//==============================================================================
void ConfigurationInfoReader::initPlatform(void)
{
	try
	{
		xercesc::XMLPlatformUtils::Initialize();  // Initialize Xerces infrastructure
	}
	catch( xercesc::XMLException& e )
	{
		mf::LogError(__FILE__) << "XML toolkit initialization error: " << XML_TO_CHAR(e.getMessage()) << std::endl;
		// throw exception here to return ERROR_XERCES_INIT
	}
}

//==============================================================================
void ConfigurationInfoReader::terminatePlatform(void)
{
	try
	{
		xercesc::XMLPlatformUtils::Terminate();  // Terminate after release of memory
	}
	catch( xercesc::XMLException& e )
	{
		mf::LogError(__FILE__) << "XML tolkit teardown error: " << XML_TO_CHAR(e.getMessage()) << std::endl;
	}
}


//==============================================================================
void ConfigurationInfoReader::setAllowColumnErrors(bool setValue)
{
	allowIllegalColumns_ = setValue;
}
//==============================================================================
const bool& ConfigurationInfoReader::getAllowColumnErrors(void)
{
	return allowIllegalColumns_;
}

//==============================================================================
bool ConfigurationInfoReader::checkViewType(std::string type)
{
	std::vector<std::string> types;
	int currentIndex = 0;
	while( type.find(',',currentIndex) != std::string::npos )
	{
		types.push_back(type.substr(currentIndex,type.find(',',currentIndex)-currentIndex));
		currentIndex = type.find(',',currentIndex)+1;
	}
	types.push_back(type.substr(currentIndex,type.size()));

	std::string systemType = getenv("CONFIGURATION_TYPE");
	for(unsigned int i=0; i<types.size(); i++)
	{
		if( types[i] == systemType )
			return true;
	}
	//In case I don't succeed let's check if maybe there is something wrong with the names
	const unsigned int allowedNamesSize = 3;
	const std::string allowedNames[allowedNamesSize] =
	{"File","Database","DatabaseTest"
	};
	if(systemType != allowedNames[0] && systemType != allowedNames[1] && systemType != allowedNames[2] )
	{
		__COUT__ << "The type defined in CONFIGURATION_TYPE ("
				<< systemType << ") doesn't match with any of the allowed types: File,Database or DatabaseTest"
				<< std::endl;

		throw(std::runtime_error( "Illegal configuration type" ));
	}
	for(unsigned int i=0; i<types.size(); i++)
	{
		if(types[i] != allowedNames[0] && types[i] != allowedNames[1] && types[i] != allowedNames[2] )
		{
			__COUT__ << "The type defined in the info file ("
					<< types[i] << ") doesn't match with any of the allowed types: "
					<< allowedNames[0] << ", " << allowedNames[1] << " or " << allowedNames[2]
																							<< std::endl;
			throw(std::runtime_error( "Illegal Type!" ));
		}
	}

	return false;
}

//==============================================================================
xercesc::DOMNode* ConfigurationInfoReader::getNode(XMLCh* tagName, xercesc::DOMNode* parent, unsigned int itemNumber)
{
	return getNode(tagName, dynamic_cast< xercesc::DOMElement* >(parent), itemNumber);
}

//==============================================================================
xercesc::DOMNode* ConfigurationInfoReader::getNode(XMLCh* tagName, xercesc::DOMElement* parent, unsigned int itemNumber)
{
	xercesc::DOMNodeList* nodeList = parent->getElementsByTagName(tagName);
	if( !nodeList )
	{
		throw(std::runtime_error( std::string("Can't find ") + XML_TO_CHAR(tagName) + " tag!"));
		__COUT__ << (std::string("Can't find ") + XML_TO_CHAR(tagName) + " tag!") << std::endl;
	}
	//    __COUT__<< "Name: "  << XML_TO_CHAR(nodeList->item(itemNumber)->getNodeName()) << std::endl;
	//    if( nodeList->item(itemNumber)->getFirstChild() != 0 )
	//        __COUT__<< "Value: " << XML_TO_CHAR(nodeList->item(itemNumber)->getFirstChild()->getNodeValue()) << std::endl;
	return nodeList->item(itemNumber);
}

//==============================================================================
xercesc::DOMElement* ConfigurationInfoReader::getElement(XMLCh* tagName, xercesc::DOMNode* parent, unsigned int itemNumber)
{
	return dynamic_cast< xercesc::DOMElement* >(getNode(tagName,parent,itemNumber));
}

//==============================================================================
xercesc::DOMElement* ConfigurationInfoReader::getElement(XMLCh* tagName, xercesc::DOMElement* parent, unsigned int itemNumber)
{
	return dynamic_cast< xercesc::DOMElement* >(getNode(tagName,parent,itemNumber));
}

//==============================================================================
std::string ConfigurationInfoReader::read(ConfigurationBase& configuration)
{
	std::string accumulatedExceptions = "";

	//KEEP For debugging... added by Gennadiy...
	//if configuration name starts with "TestConfiguration00" then turn off the reading of information
	//	auto tmp_test_config_prefix = std::string{"TestConfiguration00"};
	//        auto tmp_config_name = configuration.getConfigurationName();
	//	if (std::equal(tmp_test_config_prefix.begin(), tmp_test_config_prefix.end(), tmp_config_name.begin())) return accumulatedExceptions;
	//KEEP End debugging... for Gennadiy...


	//These environment variables are required
	if(getenv("CONFIGURATION_TYPE"     ) == NULL)	__COUT__ << "Missing env variable: CONFIGURATION_TYPE. It must be set!" << std::endl;
	if(getenv("CONFIGURATION_DATA_PATH") == NULL) 	__COUT__ << "Missing env variable: CONFIGURATION_DATA_PATH. It must be set!" << std::endl;
	if(getenv("CONFIGURATION_INFO_PATH") == NULL) 	__COUT__ << "Missing env variable: CONFIGURATION_INFO_PATH. It must be set!" << std::endl;


	//example c++ setting of necessary environment variables
	//setenv("CONFIGURATION_TYPE","File",1);
	//setenv("CONFIGURATION_DATA_PATH",(std::string(getenv("USER_DATA")) + "/ConfigurationDataExamples").c_str(),1);
	//setenv("CONFIGURATION_INFO_PATH",(std::string(getenv("USER_DATA")) + "/ConfigurationInfo").c_str(),1);


	std::string configurationDataDir = std::string(getenv("CONFIGURATION_INFO_PATH")) + "/";
	std::string configFile = configurationDataDir + configuration.getConfigurationName() + "Info.xml";
	//__COUT__ << configFile << std::endl;
	struct stat fileStatus;

	int iretStat = stat(configFile.c_str(), &fileStatus);
	if( iretStat == ENOENT )
	{
		__SS__ << ("Path file_name does not exist, or path is an empty std::string.") << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}
	else if( iretStat == ENOTDIR )
	{
		__SS__ << ("A component of the path is not a directory.") << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}
	else if( iretStat == ELOOP )
	{
		__SS__ << ("Too many symbolic links encountered while traversing the path.") << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}
	else if( iretStat == EACCES )
	{
		__SS__ << ("Permission denied.") << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}
	else if( iretStat == ENAMETOOLONG )
	{
		__SS__ << ("File can not be read. Name too long.") << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}

	xercesc::XercesDOMParser* parser = new xercesc::XercesDOMParser;
	// Configure DOM parser.
	parser->setValidationScheme(xercesc::XercesDOMParser::Val_Auto);//Val_Never
	parser->setDoNamespaces             ( true );
	parser->setDoSchema                 ( true );
	parser->useCachedGrammarInParse     ( false );

	DOMTreeErrorReporter* errorHandler = new DOMTreeErrorReporter() ;
	parser->setErrorHandler(errorHandler);
	try
	{
		parser->parse( configFile.c_str() );

		// no need to free this pointer - owned by the parent parser object
		xercesc::DOMDocument* xmlDocument = parser->getDocument();

		// Get the top-level element: Name is "root". No attributes for "root"
		xercesc::DOMElement* elementRoot = xmlDocument->getDocumentElement();
		if( !elementRoot )
		{
			delete parser;
			delete errorHandler;
			throw(std::runtime_error( "empty XML document" ));
		}

		//<CONFIGURATION>
		xercesc::DOMElement* configurationElement = getElement(configurationTag_, elementRoot, 0);
		if( configuration.getConfigurationName() != XML_TO_CHAR(configurationElement->getAttribute(configurationNameAttributeTag_)) )
		{

			__SS__ << "In " << configFile << " the configuration name " << XML_TO_CHAR(configurationElement->getAttribute(configurationNameAttributeTag_))
            				<< " doesn't match the the class configuration name " << configuration.getConfigurationName() << std::endl;

			delete parser;
			delete errorHandler;
			__COUT_ERR__ << "\n" << ss.str();
			throw(std::runtime_error( ss.str()));
		}
		//<VIEW>
		xercesc::DOMNodeList* viewNodeList = configurationElement->getElementsByTagName(viewTag_);
		bool storageTypeFound = false;

		if(viewNodeList->getLength() != 1)
		{

			__SS__ << "In " << configFile << " the configuration name " << XML_TO_CHAR(configurationElement->getAttribute(configurationNameAttributeTag_))
            				<< " there must only be one view. There were " <<
							viewNodeList->getLength() << " found." << std::endl;

			delete parser;
			delete errorHandler;
			__COUT_ERR__ << "\n" << ss.str();
			throw(std::runtime_error( ss.str()));
		}

		for( XMLSize_t view = 0; view < viewNodeList->getLength(); view++ )
		{
			if( !viewNodeList->item(view)->getNodeType() || viewNodeList->item(view)->getNodeType() != xercesc::DOMNode::ELEMENT_NODE )//true is not 0 && is element
				continue;
			xercesc::DOMElement* viewElement = dynamic_cast< xercesc::DOMElement* >( viewNodeList->item(view) );
			std::string viewType = XML_TO_CHAR(viewElement->getAttribute(viewTypeAttributeTag_));
			if(!checkViewType(viewType))
				continue;
			storageTypeFound = true;
			configuration.getMockupViewP()->setTableName(XML_TO_CHAR(viewElement->getAttribute(viewNameAttributeTag_)));
			xercesc::DOMNodeList* columnNodeList = viewElement->getElementsByTagName(columnTag_);
			for( XMLSize_t column = 0; column < columnNodeList->getLength(); column++ )
			{
				//<COLUMN>
				xercesc::DOMElement* columnElement = dynamic_cast< xercesc::DOMElement* >( columnNodeList->item(column) );
				//__COUT__ << XML_TO_CHAR(columnElement->getAttribute(columnNameAttributeTag_)) << std::endl;

				//automatically delete the persistent version of the column info
				std::string capturedException;
				configuration.getMockupViewP()->getColumnsInfoP()->push_back(
						ViewColumnInfo(
								XML_TO_CHAR(columnElement->getAttribute(columnTypeAttributeTag_)),
								XML_TO_CHAR(columnElement->getAttribute(columnNameAttributeTag_)),
								XML_TO_CHAR(columnElement->getAttribute(columnStorageNameAttributeTag_)),
								XML_TO_CHAR(columnElement->getAttribute(columnDataTypeAttributeTag_)),
								XML_TO_CHAR(columnElement->getAttribute(columnDataChoicesAttributeTag_)),
								allowIllegalColumns_?&capturedException:0)); //capture exception string if allowing illegal columns

				//if error detected (this implies allowing illegal columns)
				//	accumulate and return accumulated errors at end
				if(capturedException != "")
					accumulatedExceptions += std::string("\n\nColumn Error:") + capturedException;

				//</COLUMN>
			}

			//handle view description (which is actually the configuration
			//	description since only one view allowed)
			std::string configurationDescription = XML_TO_CHAR(viewElement->getAttribute(
					viewDescriptionAttributeTag_));

			configuration.setConfigurationDescription(
					ConfigurationView::decodeURIComponent(configurationDescription));
			//__COUT__ << "configurationDescription = " << configurationDescription << std::endl;

			//</VIEW>
		}
		if( !storageTypeFound )
		{
			__COUT__ << "The type defined in CONFIGURATION_TYPE ("
					<< getenv("CONFIGURATION_TYPE") << ") doesn't match with any of the types defined in " << configFile << std::endl;

			delete parser;
			delete errorHandler;
			throw(std::runtime_error( "Configuration Type mismatch!" ));
		}

		//</CONFIGURATION>
	}
	catch( xercesc::XMLException& e )
	{
		std::ostringstream errBuf;
		errBuf << "Error parsing file: " << XML_TO_CHAR(e.getMessage()) << std::flush;
	}
	delete parser;
	delete errorHandler;

	//__COUT__ << std::endl;

	//if exceptions have been accumulated
	//	then in allowIllegalColumns mode
	//return accumulated exception strings to next level
	return accumulatedExceptions;
}

//==============================================================================
//returns accumulated exception string (while allowIllegalColumns == true)
// otherwise "" if no exceptions
std::string ConfigurationInfoReader::read(ConfigurationBase* configuration)
{
	return read(*configuration);
}
