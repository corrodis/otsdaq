/*
 * RegisterSequencerInfoReader.cpp
 *
 *  Created on: Jul 29, 2015
 *      Author: parilla
 */

#include "otsdaq-core/ConfigurationDataFormats/RegisterSequencerInfoReader.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/XmlUtilities/ConvertFromXML.h"

#include <iostream>
#include <stdexcept>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNodeList.hpp>

using namespace ots;

RegisterSequencerInfoReader::RegisterSequencerInfoReader()
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
		xercesc::XMLString::release(&componentNameTag_);
		xercesc::XMLString::release(&registerNameTag_);
		xercesc::XMLString::release(&registerValueTag_);
		xercesc::XMLString::release(&sequenceNumberTag_);
		xercesc::XMLString::release(&stateTag_);
	}
	catch (...)
	{
		TLOG(TLVL_ERROR, __FILE__) << "Unknown exception encountered in TagNames destructor"
					   << "     ";
	}
}

RegisterSequencerInfoReader::~RegisterSequencerInfoReader()
{
	// TODO Auto-generated destructor stub
}

//==============================================================================
void RegisterSequencerInfoReader::initPlatform(void)
{
	try
	{
		xercesc::XMLPlatformUtils::Initialize();  // Initialize Xerces infrastructure
	}
	catch (xercesc::XMLException& e)
	{
		TLOG(TLVL_ERROR, __FILE__) << "XML toolkit initialization error: " << XML_TO_CHAR(e.getMessage()) << "     ";
		// throw exception here to return ERROR_XERCES_INIT
	}
}

//==============================================================================
void RegisterSequencerInfoReader::terminatePlatform(void)
{
	try
	{
		xercesc::XMLPlatformUtils::Terminate();  // Terminate after release of memory
	}
	catch (xercesc::XMLException& e)
	{
		TLOG(TLVL_ERROR, __FILE__) << "XML toolkit teardown error: " << XML_TO_CHAR(e.getMessage()) << "     ";
	}
}

//==============================================================================
xercesc::DOMNode* RegisterSequencerInfoReader::getNode(XMLCh* tagName, xercesc::DOMNode* parent, unsigned int itemNumber)
{
	return getNode(tagName, dynamic_cast<xercesc::DOMElement*>(parent), itemNumber);
}

//==============================================================================
xercesc::DOMNode* RegisterSequencerInfoReader::getNode(XMLCh* tagName, xercesc::DOMElement* parent, unsigned int itemNumber)
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
xercesc::DOMElement* RegisterSequencerInfoReader::getElement(XMLCh* tagName, xercesc::DOMNode* parent, unsigned int itemNumber)
{
	return dynamic_cast<xercesc::DOMElement*>(getNode(tagName, parent, itemNumber));
}

//==============================================================================
xercesc::DOMElement* RegisterSequencerInfoReader::getElement(XMLCh* tagName, xercesc::DOMElement* parent, unsigned int itemNumber)
{
	return dynamic_cast<xercesc::DOMElement*>(getNode(tagName, parent, itemNumber));
}

//==============================================================================
void RegisterSequencerInfoReader::read(RegisterBase& configuration)
{
	/*
    std::string configurationDataDir = std::string(getenv("CONFIGURATION_DATA_PATH")) + "/" + configuration.getTypeName() + "RegisterSequencer/";
    		//"/ConfigurationInfo/";
    std::string configFile = configurationDataDir + "/0/" + configuration.getTypeName() + "RegisterSequencer" + ".xml";
    //std::cout << __COUT_HDR_FL__ << configFile << std::endl;

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

    XercesDOMParser* parser = new XercesDOMParser;
    // Configure DOM parser.
    parser->setValidationScheme(XercesDOMParser::Val_Auto);//Val_Never
    parser->setDoNamespaces             ( true );
    parser->setDoSchema                 ( true );
    parser->useCachedGrammarInParse     ( false );

    DOMTreeErrorReporter* errorHandler = new DOMTreeErrorReporter() ;
    parser->setErrorHandler(errorHandler);
    try
    {
        parser->parse( configFile.c_str() );

        // no need to free this pointer - owned by the parent parser object
        DOMDocument* xmlDocument = parser->getDocument();

        // Get the top-level element: Name is "root". No attributes for "root"
        DOMElement* elementRoot = xmlDocument->getDocumentElement();
        if( !elementRoot )
            throw(std::runtime_error( "empty XML document" ));

        //<CONFIGURATION>
        DOMElement* configurationElement = getElement(configurationTag_, elementRoot, 0);
        if( configuration.getConfigurationName() != XML_TO_CHAR(configurationElement->getAttribute(configurationNameAttributeTag_)) )
        {

            std::cout << __COUT_HDR_FL__ << "In " << configFile << " the configuration name " << XML_TO_CHAR(configurationElement->getAttribute(configurationNameAttributeTag_))
            << " doesn't match the the class configuration name " << configuration.getConfigurationName() << "     ";
            assert(0);
        }
        //<DATA>
        DOMNodeList* viewNodeList = configurationElement->getElementsByTagName(dataTag_);
        bool storageTypeFound = false;
        for( XMLSize_t view = 0; view < viewNodeList->getLength(); view++ )
        {
            if( !viewNodeList->item(view)->getNodeType() || viewNodeList->item(view)->getNodeType() != DOMNode::ELEMENT_NODE )//true is not 0 && is element
                continue;

            configuration.getMockupViewP()->setName(XML_TO_CHAR(viewElement->getAttribute(viewNameAttributeTag_)));
            DOMElement * componentNameElement 	= dynamic_cast< xercesc::DOMElement* >(viewNodeList->getElementByTagName(typeNameTag_		));
            DOMElement * registerNameElement 	= dynamic_cast< xercesc::DOMElement* >(viewNodeList->getElementByTagName(registerNameTag_	));
            DOMElement * registerValueElement 	= dynamic_cast< xercesc::DOMElement* >(viewNodeList->getElementByTagName(registerValueTag_	));
            DOMElement * sequenceNumberElement	= dynamic_cast< xercesc::DOMElement* >(viewNodeList->getElementByTagName(sequenceNumberTag_	));
            DOMElement * stateElement		 	= dynamic_cast< xercesc::DOMElement* >(viewNodeList->getElementByTagName(stateTag_			));
            configuration.getMockupRegisterViewP()->getRegistersSequencerInfoPointer()->push_back(
                ViewRegisterSequencerInfo(XML_TO_CHAR(componentNameElement	),
                				 	 	 XML_TO_CHAR(registerNameElement	),
                				 	 	 XML_TO_CHAR(registerValueElement	),
                				 	 	 XML_TO_CHAR(sequenceNumberElement	),
                				 	 	 XML_TO_CHAR(stateElement			)));
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
        ostringstream errBuf;
        errBuf << "Error parsing file: " << XML_TO_CHAR(e.getMessage()) << flush;
    }
    delete parser;
    delete errorHandler;
  */
}

//==============================================================================
void RegisterSequencerInfoReader::read(RegisterBase* configuration)
{
	read(*configuration);
}
