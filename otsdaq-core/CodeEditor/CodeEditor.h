#ifndef _ots_CodeEditor_h_
#define _ots_CodeEditor_h_

#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/Macros/StringMacros.h"
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"
#include "xgi/Method.h" 								//for cgicc::Cgicc

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>




namespace ots
{

class HttpXmlDocument;
    
//CodeEditor
//	This class provides the functionality for editing, saving, and building code
class CodeEditor
{
public:
	CodeEditor();
	
	//request are handled here
	void xmlRequest(
			const std::string&		option,
			cgicc::Cgicc& 			cgiIn,
			HttpXmlDocument* 		xmlOut);

private:

	void getDirectoryContent			(cgicc::Cgicc& cgiIn, HttpXmlDocument* xmlOut);
	void getFileContent					(cgicc::Cgicc& cgiIn, HttpXmlDocument* xmlOut);
	void saveFileContent				(cgicc::Cgicc& cgiIn, HttpXmlDocument* xmlOut);
	void cleanBuild						(cgicc::Cgicc& cgiIn, HttpXmlDocument* xmlOut);
	void incrementalBuild				(cgicc::Cgicc& cgiIn, HttpXmlDocument* xmlOut);

};

}

#endif
