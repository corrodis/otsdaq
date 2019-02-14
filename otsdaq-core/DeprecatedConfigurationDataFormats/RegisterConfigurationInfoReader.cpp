/*
 * RegisterConfigurationInfoReader.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: parilla
 */

#include "otsdaq-core/ConfigurationDataFormats/RegisterConfigurationInfoReader.h"
#include "otsdaq-core/ConfigurationDataFormats/RegisterBase.h"
#include "otsdaq-core/XmlUtilities/ConvertFromXML.h"
#include "otsdaq-core/XmlUtilities/DOMTreeErrorReporter.h"

#include <sys/stat.h>
#include <iostream>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLString.hpp>

using namespace ots;

RegisterConfigurationInfoReader::RegisterConfigurationInfoReader()
{
	// TODO Auto-generated constructor stub
	initPlatform();
	rootTag_	       = xercesc::XMLString::transcode("ROOT");
	headerTag_	     = xercesc::XMLString::transcode("HEADER");
	typeTag_	       = xercesc::XMLString::transcode("TYPE");
	extensionTableNameTag_ = xercesc::XMLString::transcode("EXTENSION_TABLE_NAME");
	nameTag_	       = xercesc::XMLString::transcode("NAME");
	dataSetTag_	    = xercesc::XMLString::transcode("DATA_SET");
	versionTag_	    = xercesc::XMLString::transcode("VERSION");
	commentDescriptionTag_ = xercesc::XMLString::transcode("COMMENT_DESCRIPTION");
	createdByUserTag_      = xercesc::XMLString::transcode("CREATED_BY_USER");
	dataTag_	       = xercesc::XMLString::transcode("DATA");
	typeNameTag_	   = xercesc::XMLString::transcode("TYPE_NAME");
	registerNameTag_       = xercesc::XMLString::transcode("REGISTER_NAME");
	baseAddressTag_	= xercesc::XMLString::transcode("BASE_ADDRESS");
	sizeTag_	       = xercesc::XMLString::transcode("REGISTER_SIZE");
	accessTag_	     = xercesc::XMLString::transcode("ACCESS");
}

RegisterConfigurationInfoReader::~RegisterConfigurationInfoReader()
{
	try
	{
		xercesc::XMLString::release(&rootTag_);
		xercesc::XMLString::release(&headerTag_);
		xercesc::XMLString::release(&typeTag_);
		xercesc::XMLString::release(&extensionTableNameTag_);
		xercesc::XMLString::release(&nameTag_);
		xercesc::XMLString::release(&dataSetTag_);
		xercesc::XMLString::release(&versionTag_);
		xercesc::XMLString::release(&commentDescriptionTag_);
		xercesc::XMLString::release(&createdByUserTag_);
		xercesc::XMLString::release(&typeNameTag_);
		xercesc::XMLString::release(&registerNameTag_);
		xercesc::XMLString::release(&baseAddressTag_);
		xercesc::XMLString::release(&sizeTag_);
		xercesc::XMLString::release(&accessTag_);
	}
	catch (...)
	{
		__MOUT_ERROR__ << "Unknown exception encountered in TagNames destructor"
			       << "     ";
	}
	terminatePlatform();  // TODO Auto-generated destructor stub
}

//==============================================================================
void RegisterConfigurationInfoReader::initPlatform(void)
{
	try
	{
		xercesc::XMLPlatformUtils::Initialize();  // Initialize Xerces infrastructure
	}
	catch (xercesc::XMLException& e)
	{
		__MOUT_ERROR__ << "XML toolkit initialization error: " << XML_TO_CHAR(e.getMessage()) << "     ";
		// throw exception here to return ERROR_XERCES_INIT
	}
}

//==============================================================================
void RegisterConfigurationInfoReader::terminatePlatform(void)
{
	try
	{
		xercesc::XMLPlatformUtils::Terminate();  // Terminate after release of memory
	}
	catch (xercesc::XMLException& e)
	{
		__MOUT_ERROR__ << "XML toolkit teardown error: " << XML_TO_CHAR(e.getMessage()) << "     ";
	}
}

//==============================================================================
xercesc::DOMNode* RegisterConfigurationInfoReader::getNode(XMLCh* tagName, xercesc::DOMNode* parent, unsigned int itemNumber)
{
	return getNode(tagName, dynamic_cast<xercesc::DOMElement*>(parent), itemNumber);
}

//==============================================================================
xercesc::DOMNode* RegisterConfigurationInfoReader::getNode(XMLCh* tagName, xercesc::DOMElement* parent, unsigned int itemNumber)
{
	xercesc::DOMNodeList* nodeList = parent->getElementsByTagName(tagName);
	if (!nodeList)
	{
		throw(std::runtime_error(std::string("Can't find ") + XML_TO_CHAR(tagName) + " tag!"));
		std::cout << __COUT_HDR_FL__ << (std::string("Can't find ") + XML_TO_CHAR(tagName) + " tag!") << std::endl;
	}
	//    std::cout << __COUT_HDR_FL__<< "Name: "  << XML_TO_CHAR(nodeList->item(itemNumber)->getNodeName()) << std::endl;
	//    if( nodeList->item(itemNumber)->getFirstChild() != 0 )
	//        std::cout << __COUT_HDR_FL__<< "Value: " << XML_TO_CHAR(nodeList->item(itemNumber)->getFirstChild()->getNodeValue()) << std::endl;
	return nodeList->item(itemNumber);
}

//==============================================================================
xercesc::DOMElement* RegisterConfigurationInfoReader::getElement(XMLCh* tagName, xercesc::DOMNode* parent, unsigned int itemNumber)
{
	return dynamic_cast<xercesc::DOMElement*>(getNode(tagName, parent, itemNumber));
}

//==============================================================================
xercesc::DOMElement* RegisterConfigurationInfoReader::getElement(XMLCh* tagName, xercesc::DOMElement* parent, unsigned int itemNumber)
{
	return dynamic_cast<xercesc::DOMElement*>(getNode(tagName, parent, itemNumber));
}

//==============================================================================
void RegisterConfigurationInfoReader::read(RegisterBase& configuration)
{
	/*
    std::string configurationDataDir = std::string(getenv("CONFIGURATION_DATA_PATH")) + "/" + configuration.getTypeName() + "RegisterConfiguration/";
    		//"/ConfigurationInfo/";
    std::string configFile = configurationDataDir + "/0/" + configuration.getTypeName() + "RegisterConfiguration" + ".xml";
    struct stat fileStatus;

    int iretStat = stat(configFile.c_str(), &fileStatus);
    if( iretStat == ENOENT )
         );
    else if( iretStat == ENOTDIR )
        );
    else if( iretStat == ELOOP )
        );
    else if( iretStat == EACCES )
        );
    else if( iretStat == ENAMETOOLONG )
        );

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
            throw(std::runtime_error( "empty XML document" ));

        //<CONFIGURATION>
        xercesc::DOMElement* configurationElement = getElement(registerNameTag_, elementRoot, 0);
        if( configuration.getConfigurationName() != XML_TO_CHAR(configurationElement->getAttribute(configurationNameAttributeTag_)) )
        {

            std::cout << __COUT_HDR_FL__ << "In " << configFile << " the configuration name " << XML_TO_CHAR(configurationElement->getAttribute(configurationNameAttributeTag_))
            << " doesn't match the the class configuration name " << configuration.getConfigurationName() << "     ";
            assert(0);
	    }
        //<DATA>
        xercesc::DOMNodeList* viewNodeList = configurationElement->getElementsByTagName(dataTag_);
        bool storageTypeFound = false;
        for( XMLSize_t view = 0; view < viewNodeList->getLength(); view++ )
        {
	  if( !viewNodeList->item(view)->getNodeType() || viewNodeList->item(view)->getNodeType() != xercesc::DOMNode::ELEMENT_NODE )//true is not 0 && is element
                continue;

	  auto viewElement = dynamic_cast<xercesc::DOMElement*>(viewNodeList->item(view));
	  //configuration.getMockupViewP()->setName(XML_TO_CHAR(viewElement->getAttribute(viewNameAttributeTag_)));
	  xercesc::DOMElement * typeNameElement 	 	= dynamic_cast< xercesc::DOMElement* >(viewElement->getElementsByTagName(typeNameTag_		)->item(0));
            xercesc::DOMElement * registerNameElement 	= dynamic_cast< xercesc::DOMElement* >(viewElement->getElementsByTagName(registerNameTag_	)->item(0));
            xercesc::DOMElement * baseAddressElement 	= dynamic_cast< xercesc::DOMElement* >(viewElement->getElementsByTagName(baseAddressTag_	)->item(0));
            xercesc::DOMElement * sizeElement		 	= dynamic_cast< xercesc::DOMElement* >(viewElement->getElementsByTagName(sizeTag_			)->item(0));
            xercesc::DOMElement * accessElement		 	= dynamic_cast< xercesc::DOMElement* >(viewElement->getElementsByTagName(accessTag_			)->item(0));
            configuration.getMockupViewP()->getRegistersInfoPointer()->push_back(
		ViewRegisterInfo(XML_TO_CHAR(typeNameElement->getNodeValue()		),
				 XML_TO_CHAR(registerNameElement->getNodeValue()	),
				 XML_TO_CHAR(baseAddressElement->getNodeValue()		),
				 XML_TO_CHAR(sizeElement->getNodeValue()			),
				 XML_TO_CHAR(accessElement->getNodeValue()			)));
            //</DATA>

        }
        if( !storageTypeFound )
        {
         	std::cout << __COUT_HDR_FL__ << "The type defined in CONFIGURATION_TYPE ("
        	<< getenv("CONFIGURATION_TYPE") << ") doesn't match with any of the types defined in " << configFile << "     ";
            assert(0);
        }

        //</CONFIGURATION>
    }
    catch( xercesc::XMLException& e )
    {
      std::ostringstream errBuf;
      errBuf << "Error parsing file: " << XML_TO_CHAR(e.getMessage()) << std::flush;
    }
    delete parser;
    delete errorHandler;*/
}

//==============================================================================
void RegisterConfigurationInfoReader::read(RegisterBase* configuration)
{
	read(*configuration);
}
