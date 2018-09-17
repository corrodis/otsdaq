#include "otsdaq-core/CodeEditor/CodeEditor.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/CgiDataUtilities/CgiDataUtilities.h"


#include <dirent.h> //DIR and dirent
#include <thread>   //for std::thread

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
	__COUTV__(option);

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
		getDirectoryContent(cgiIn,xmlOut);
	}
	else if(option == "getFileContent")
	{
		getFileContent(cgiIn,xmlOut);
	}
	else if(option == "saveFileContent")
	{
		saveFileContent(cgiIn,xmlOut);
	}
	else if(option == "build")
	{
		build(cgiIn,xmlOut);
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

	xmlOut->addTextElementToData("path",path);

	std::string pluginFolders[] = {"FEInterfaces","DataProcessorPlugins","ControlsInterfacePlugins",
			"FEInterfacePlugins"};

	DIR *pDIR;
	struct dirent *entry;
	bool isDir;
	std::string name;
	int type;

	if( !(pDIR=opendir((SOURCE_BASE_PATH+path).c_str())) )
	{
		__SS__ << "Path '" << path << "' could not be opened!" << __E__;
		__SS_THROW__;
	}

	//else directory good, get all folders, .h, .cc, .txt files
	while((entry = readdir(pDIR)))
	{
		name = std::string(entry->d_name);
		type = int(entry->d_type);

		//__COUT__ << type << " " << name << "\n" << std::endl;

		if( name[0] != '.' && (type== 0 || //0 == UNKNOWN (which can happen - seen in SL7 VM)
				type == 4 || type == 8))
		{
			isDir = false;

			if(type == 0)
			{
				//unknown type .. determine if directory
				DIR *pTmpDIR = opendir((path + "/" + name).c_str());
				if(pTmpDIR)
				{
					isDir = true;
					closedir(pTmpDIR);
				}
				//else //assume file
			}

			if(type == 4)
				isDir = true; //flag directory types

			//handle directories


			if(isDir)
			{
				__COUT__ << "Directory: " << type << " " << name << __E__;

				xmlOut->addTextElementToData("directory",name);
			}
			else //type 8 or 0 is file
			{
				//__COUT__ << "File: " << type << " " << name << "\n" << std::endl;
				if(
						name.find(".h") == name.length()-2 ||
						name.find(".cc") == name.length()-3 ||
						name.find(".txt") == name.length()-4
				)
				{
					__COUT__ << "EditFile: " << type << " " << name << __E__;
					xmlOut->addTextElementToData("file",name);
				}
			}
		}
	}

	closedir(pDIR);


} //end getDirectoryContent

//========================================================================================================================
//getFileContent
void CodeEditor::getFileContent(
		cgicc::Cgicc& 					cgiIn,
		HttpXmlDocument* 				xmlOut)
{
	std::string path = CgiDataUtilities::getData(cgiIn, "path");
	xmlOut->addTextElementToData("path",path);

	std::string extension = CgiDataUtilities::getData(cgiIn, "ext");
	xmlOut->addTextElementToData("ext",extension);

	__COUTV__(path);
	__COUTV__(extension);

	//remove all non ascii and non /
	std::string fullpath = SOURCE_BASE_PATH;
	for(unsigned int i=0;i<path.length();++i)
		if((path[i] >= 'a' && path[i] <= 'z') ||
				(path[i] >= 'A' && path[i] <= 'Z') ||
				path[i] >= '_' ||
				path[i] >= '-' ||
				path[i] >= '/')
			fullpath += path[i];

	fullpath += ".";

	for(unsigned int i=0;i<extension.length();++i)
		if((extension[i] >= 'a' && extension[i] <= 'z') ||
				(extension[i] >= 'A' && extension[i] <= 'Z'))
			fullpath += extension[i];
	__COUTV__(fullpath);


	std::FILE *fp = std::fopen(fullpath.c_str(), "rb");
	if (!fp)
	{
		__SS__ << "Could not open file at " << path << __E__;
		__SS_THROW__;
	}

	std::string contents;
	std::fseek(fp, 0, SEEK_END);
	contents.resize(std::ftell(fp));
	std::rewind(fp);
	std::fread(&contents[0], 1, contents.size(), fp);
	std::fclose(fp);

	xmlOut->addTextElementToData("content",contents);

} //end getFileContent

//========================================================================================================================
//saveFileContent
void CodeEditor::saveFileContent(
		cgicc::Cgicc& 					cgiIn,
		HttpXmlDocument* 				xmlOut)
{
	std::string path = CgiDataUtilities::getData(cgiIn, "path");
	xmlOut->addTextElementToData("path",path);

	std::string extension = CgiDataUtilities::getData(cgiIn, "ext");
	xmlOut->addTextElementToData("ext",extension);

	__COUTV__(path);
	__COUTV__(extension);

	std::string contents = CgiDataUtilities::postData(cgiIn, "content");
	__COUTV__(contents);
	contents = StringMacros::decodeURIComponent(contents);



	//remove all non ascii and non /
	std::string fullpath = SOURCE_BASE_PATH;
	for(unsigned int i=0;i<path.length();++i)
		if((path[i] >= 'a' && path[i] <= 'z') ||
				(path[i] >= 'A' && path[i] <= 'Z') ||
				path[i] >= '_' ||
				path[i] >= '-' ||
				path[i] >= '/')
			fullpath += path[i];

	fullpath += ".";

	for(unsigned int i=0;i<extension.length();++i)
		if((extension[i] >= 'a' && extension[i] <= 'z') ||
				(extension[i] >= 'A' && extension[i] <= 'Z'))
			fullpath += extension[i];
	__COUTV__(fullpath);




	FILE *fp = fopen(fullpath.c_str(), "wb");
	if (!fp)
	{
		__SS__ << "Could not open file for saving at " << path << __E__;
		__SS_THROW__;
	}


	std::fwrite(&contents[0], 1, contents.size(), fp);
	std::fclose(fp);

} //end saveFileContent

//========================================================================================================================
//build
//	cleanBuild and incrementalBuild
void CodeEditor::build(
		cgicc::Cgicc& 					cgiIn,
		HttpXmlDocument* 				xmlOut)
{
	bool clean = CgiDataUtilities::getDataAsInt(cgiIn, "clean")?true:false;

	__COUTV__(clean);

	//launch as thread so it does not lock up rest of code
	std::thread([](bool clean) {

		std::string cmd;
		if(clean)
		{
			//clean
			{
				cmd = "mrb z 2>&1";

				std::array<char, 128> buffer;
				std::string result;
				std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
				if (!pipe) __THROW__("popen() failed!");

				size_t i;
				size_t j;

				while (!feof(pipe.get()))
				{
					if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
					{
						result += buffer.data();

						//each time there is a new line print out
						i = result.find('\n');
						__COUTV__(result.substr(0,i));
						__MOUT__ << result.substr(0,i);
						result = result.substr(i+1); //discard before new line
					}
				}

				__COUTV__(result);
				__MOUT__ << result.substr(0,i);
			}

			sleep(1);
			//mrbsetenv
			{
				cmd = "source mrbSetEnv 2>&1";

				std::array<char, 128> buffer;
				std::string result;
				std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
				if (!pipe) __THROW__("popen() failed!");

				size_t i;
				size_t j;

				while (!feof(pipe.get()))
				{
					if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
					{
						result += buffer.data();

						//each time there is a new line print out
						i = result.find('\n');
						__COUTV__(result.substr(0,i));
						__MOUT__ << result.substr(0,i);
						result = result.substr(i+1); //discard before new line
					}
				}

				__COUTV__(result);
				__MOUT__ << result.substr(0,i);
			}
			sleep(1);

		}

		//build
		{
			cmd = "mrb b 2>&1";

			std::array<char, 128> buffer;
			std::string result;
			std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
			if (!pipe) __THROW__("popen() failed!");

			size_t i;
			size_t j;

			while (!feof(pipe.get()))
			{
				if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
				{
					result += buffer.data();

					//each time there is a new line print out
					i = result.find('\n');
					__COUTV__(result.substr(0,i));
					__MOUT__ << result.substr(0,i);
					result = result.substr(i+1); //discard before new line
				}
			}

			__COUTV__(result);
			__MOUT__ << result.substr(0,i);
		}
	},clean).detach();




} //end build





