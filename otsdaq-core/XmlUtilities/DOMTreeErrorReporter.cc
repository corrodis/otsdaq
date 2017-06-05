#include "otsdaq-core/XmlUtilities/DOMTreeErrorReporter.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

#include <iostream>
#include <sstream>
#include <xercesc/util/XMLString.hpp>

using namespace ots;

#undef 	__MOUT_HDR__
#define __MOUT_HDR__ 	"DOMTreeErrorReporter"

//==============================================================================
DOMTreeErrorReporter::DOMTreeErrorReporter()
{}

//==============================================================================
DOMTreeErrorReporter::~DOMTreeErrorReporter()
{}

//==============================================================================
void DOMTreeErrorReporter::warning(const xercesc::SAXParseException& ex)
{
	__MOUT__ << "Warning!" << std::endl;
	throw std::runtime_error(reportParseException(ex));
}

//==============================================================================
void DOMTreeErrorReporter::error(const xercesc::SAXParseException& ex)
{
	__MOUT__ << "Error!" << std::endl;
    throw std::runtime_error(reportParseException(ex));
}

//==============================================================================
void DOMTreeErrorReporter::fatalError(const xercesc::SAXParseException& ex)
{
	__MOUT__ << "Fatal Error!" << std::endl;
    throw std::runtime_error(reportParseException(ex));
}

//==============================================================================
void DOMTreeErrorReporter::resetErrors()
{}

//==============================================================================
std::string DOMTreeErrorReporter::reportParseException(const xercesc::SAXParseException& exception)
{
    __SS__ << "\n"
    	<< "\tIn file \""
		<< xercesc::XMLString::transcode(exception.getSystemId())
		<< "\", line "
		<< exception.getLineNumber()
		<< ", column "
		<< exception.getColumnNumber()
		<< std::endl
		<< "\tMessage: "
		<< xercesc::XMLString::transcode(exception.getMessage())
		//<< " (check against xsd file)" //RAR commented, has no meaning to me or users..
		<< "\n\n" ;
    __MOUT__ << "\n" << ss.str() << std::endl;
    return ss.str();
}
