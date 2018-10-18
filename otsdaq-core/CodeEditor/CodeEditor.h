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
	void build							(cgicc::Cgicc& cgiIn, HttpXmlDocument* xmlOut);

	std::string safePathString			(const std::string& path);
	std::string safeExtensionString		(const std::string& extension);

public:

	static std::string SPECIAL_TYPE_FEInterface, SPECIAL_TYPE_DataProcessor, SPECIAL_TYPE_ControlsInterface, SPECIAL_TYPE_Tools;

	static std::map<std::string /*special type*/,std::set<std::string> /*special file paths*/>
	getSpecialsMap						(void);

	static void readFile				(const std::string& path, std::string& contents);
	static void writeFile				(const std::string& path, const std::string& contents, const unsigned long long& insertPos = -1, const std::string& insertString = "");

};

}

#endif
