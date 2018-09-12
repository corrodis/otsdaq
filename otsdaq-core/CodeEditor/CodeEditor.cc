#include "otsdaq-core/CodeEditor/CodeEditor.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/CgiDataUtilities/CgiDataUtilities.h"


using namespace ots;


#define SOURCE_BASE_PATH 			    std::string(getenv("MRB_SOURCE")) + "/"



#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "CodeEditor"


//========================================================================================================================
//CodeEditor
CodeEditor::CodeEditor()
{

} //end CodeEditor()



//========================================================================================================================
//xmlRequest
//	all requests are handled here
void CodeEditor::xmlRequest(
		const std::string&				option,
		cgicc::Cgicc& 					cgiIn,
		HttpXmlDocument* 				xmlOut)
try
{

	//request options:
	//
	//	getDirectoryContent
	//	getFileContent
	//	saveFileContent
	//	cleanBuild
	//	incrementalBuild
	//
	//

	if(option == "getDirectoryContent")
	{

	}
	else if(option == "getFileContent")
	{

	}
	else if(option == "saveFileContent")
	{

	}
	else if(option == "cleanBuild")
	{

	}
	else if(option == "incrementalBuild")
	{

	}
	else
	{
		__SS__ << "Unrecognized request option '" << option << ".'" << __E__;
		__SS_THROW__;
	}
}
catch(const std::runtime_error& e)
{
	__SS__ << "Error encountered while handling the Code Editor request option '" <<
			option << "': " << e.what() << __E__;
	xmlOut->addTextElementToData("Error",ss.str());
}
catch(...)
{
	__SS__ << "Unknown error encountered while handling the Code Editor request option '" <<
			option << "!'" << __E__;
	xmlOut->addTextElementToData("Error",ss.str());
} //end xmlRequest()


//========================================================================================================================
//getDirectoryContent
void CodeEditor::getDirectoryContent(
		cgicc::Cgicc& 					cgiIn,
		HttpXmlDocument* 				xmlOut)
{
	std::string path = CgiDataUtilities::getData(cgiIn, "path");
	__COUTV__(path);

	xmlOut->addTextElementToData("option",path);

} //end getDirectoryContent

//========================================================================================================================
//getFileContent
void CodeEditor::getFileContent(
		cgicc::Cgicc& 					cgiIn,
		HttpXmlDocument* 				xmlOut)
{
	std::string path = CgiDataUtilities::getData(cgiIn, "path");
	__COUTV__(path);

	xmlOut->addTextElementToData("option",path);

} //end getFileContent

//========================================================================================================================
//saveFileContent
void CodeEditor::saveFileContent(
		cgicc::Cgicc& 					cgiIn,
		HttpXmlDocument* 				xmlOut)
{
	std::string path = CgiDataUtilities::getData(cgiIn, "path");
	__COUTV__(path);

	xmlOut->addTextElementToData("option",path);

} //end saveFileContent

//========================================================================================================================
//cleanBuild
void CodeEditor::cleanBuild(
		cgicc::Cgicc& 					cgiIn,
		HttpXmlDocument* 				xmlOut)
{
	std::string path = CgiDataUtilities::getData(cgiIn, "path");
	__COUTV__(path);

	xmlOut->addTextElementToData("option",path);

} //end cleanBuild

//========================================================================================================================
//incrementalBuild
void CodeEditor::incrementalBuild(
		cgicc::Cgicc& 					cgiIn,
		HttpXmlDocument* 				xmlOut)
{
	std::string path = CgiDataUtilities::getData(cgiIn, "path");
	__COUTV__(path);

	xmlOut->addTextElementToData("option",path);

} //end incrementalBuild









