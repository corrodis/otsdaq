#include "otsdaq-core/CodeEditor/CodeEditor.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/CgiDataUtilities/CgiDataUtilities.h"


#include <dirent.h> //DIR and dirent
#include <thread>   //for std::thread
#include <sys/stat.h> 	//for mkdir

using namespace ots;


#define SOURCE_BASE_PATH 			  		std::string(getenv("MRB_SOURCE")) + "/"
#define CODE_EDITOR_DATA_PATH 			    std::string(getenv("SERVICE_DATA_PATH")) + "/" + "CodeEditorData"



#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "CodeEditor"


//========================================================================================================================
//CodeEditor
CodeEditor::CodeEditor()
{
	std::string path = CODE_EDITOR_DATA_PATH;
	DIR *dir = opendir(path.c_str());
	if(dir)
		closedir(dir);
	else if(-1 == mkdir(path.c_str(),0755))
	{
		//lets create the service folder (for first time)
		__SS__ << "Service directory creation failed: " <<
				path << std::endl;
		__SS_THROW__;
	}

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
//safePathString
std::string CodeEditor::safePathString(const std::string& path)
{
	__COUTV__(path);
	//remove all non ascii and non /, -, _,, space
	std::string fullpath = "";
	for(unsigned int i=0;i<path.length();++i)
		if((path[i] >= 'a' && path[i] <= 'z') ||
				(path[i] >= 'A' && path[i] <= 'Z') ||
				path[i] >= '_' ||
				path[i] >= '-' ||
				path[i] >= ' ' ||
				path[i] >= '/')
			fullpath += path[i];
	__COUTV__(fullpath);
	if(!fullpath.length())
	{
		__SS__ << "Invalid path '" << fullpath << "' found!" << __E__;
		__SS_THROW__;
	}
	return fullpath;
} //end safePathString()

//========================================================================================================================
//safeExtensionString
std::string CodeEditor::safeExtensionString(const std::string& extension)
{
	__COUTV__(extension);

	std::string retExt = "";
	//remove all non ascii
	for(unsigned int i=0;i<extension.length();++i)
		if((extension[i] >= 'a' && extension[i] <= 'z') ||
				(extension[i] >= 'A' && extension[i] <= 'Z'))
			retExt += extension[i];
	__COUTV__(retExt);
	if(
			retExt != "h" && retExt != "cc" && retExt != "txt" && //should match get directory content restrictions
			retExt != "sh" && retExt != "css" && retExt != "html" &&
			retExt != "js" && retExt != "py" && retExt != "fcl"
					&& retExt != "xml")
	{
		__SS__ << "Invalid extension '" << retExt << "' found!" << __E__;
		__SS_THROW__;
	}
	return retExt;
} //end safeExtensionString()

//========================================================================================================================
//getDirectoryContent
void CodeEditor::getDirectoryContent(
		cgicc::Cgicc& 					cgiIn,
		HttpXmlDocument* 				xmlOut)
{
	std::string path = CgiDataUtilities::getData(cgiIn, "path");
	path = safePathString(CgiDataUtilities::decodeURIComponent(path));
	__COUTV__(path);

	xmlOut->addTextElementToData("path",path);


	const unsigned int numOfTypes = 3;
	std::string specialTypeNames[] = {
			"Front-End Plugins",
			"Data Processor Plugins",
			"Controls Interface Plugins"};
	std::string specialTypes[] = {
			"FEInterface",
			"DataProcessor",
			"ControlsInterface"};

	std::string pathMatchPrepend = "/"; //some requests come in with leading "/" and "//"
	if(path.length() > 1 && path[1] == '/')
		pathMatchPrepend += '/';

	for(unsigned int i=0;i<numOfTypes;++i)
		if(path == pathMatchPrepend + specialTypeNames[i])
		{
			__COUT__ << "Getting all " << specialTypeNames[i] <<
					"..." << __E__;

			std::map<std::string /*special type*/,
				std::set<std::string> /*special file paths*/> retMap = getSpecialsMap();
			if(retMap.find(specialTypes[i]) != retMap.end())
			{
				for(const auto& specialTypeFile : retMap[specialTypes[i]])
				{
					xmlOut->addTextElementToData("specialFile",specialTypeFile);
				}
			}
			else
			{
				__SS__ << "No files for type '" << specialTypeNames[i] <<
						"' were found." << __E__;
				__SS_THROW__;
			}
			return;
		}



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

	if(path == "/")
	{
		//add special directory for types

		for(unsigned int i=0;i<numOfTypes;++i)
			xmlOut->addTextElementToData("special",specialTypeNames[i]);
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
				//__COUT__ << "Directory: " << type << " " << name << __E__;

				xmlOut->addTextElementToData("directory",name);
			}
			else //type 8 or 0 is file
			{
				//__COUT__ << "File: " << type << " " << name << "\n" << std::endl;
				if(
						name.find(".h") == name.length()-2 		||
						name.find(".cc") == name.length()-3 	||
						name.find(".js") == name.length()-3 	||
						name.find(".sh") == name.length()-3 	||
						name.find(".py") == name.length()-3 	||
						name.find(".fcl") == name.length()-4 	||
						name.find(".txt") == name.length()-4 	||
						name.find(".css") == name.length()-4 	||
						name.find(".xml") == name.length()-4 	||
						name.find(".html") == name.length()-5
				)
				{
					//__COUT__ << "EditFile: " << type << " " << name << __E__;
					xmlOut->addTextElementToData("file",name);
				}
			}
		}
	} //end directory traversal

	closedir(pDIR);


} //end getDirectoryContent

//========================================================================================================================
//getFileContent
void CodeEditor::getFileContent(
		cgicc::Cgicc& 					cgiIn,
		HttpXmlDocument* 				xmlOut)
{
	std::string path = CgiDataUtilities::getData(cgiIn, "path");
	path = safePathString(CgiDataUtilities::decodeURIComponent(path));
	xmlOut->addTextElementToData("path",path);

	std::string extension = CgiDataUtilities::getData(cgiIn, "ext");
	extension = safeExtensionString(extension);
	xmlOut->addTextElementToData("ext",extension);


	std::string fullpath = SOURCE_BASE_PATH + path + "." + extension;
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
	path = safePathString(CgiDataUtilities::decodeURIComponent(path));
	xmlOut->addTextElementToData("path",path);

	std::string extension = CgiDataUtilities::getData(cgiIn, "ext");
	extension = safeExtensionString(extension);
	xmlOut->addTextElementToData("ext",extension);


	std::string contents = CgiDataUtilities::postData(cgiIn, "content");
	__COUTV__(contents);
	contents = StringMacros::decodeURIComponent(contents);

	std::string fullpath = SOURCE_BASE_PATH + path + "." + extension;
	__COUTV__(fullpath);

	FILE *fp;

	//get old file size
	fp = fopen(fullpath.c_str(), "rb");
	if (!fp)
	{
		__SS__ << "Could not open file for saving at " << fullpath << __E__;
		__SS_THROW__;
	}
	std::fseek(fp, 0, SEEK_END);
	long long int oldSize = std::ftell(fp);
	fclose(fp);

	fp = fopen(fullpath.c_str(), "wb");
	if (!fp)
	{
		__SS__ << "Could not open file for saving at " << fullpath << __E__;
		__SS_THROW__;
	}


	std::fwrite(&contents[0], 1, contents.size(), fp);
	std::fclose(fp);

	//log changes
	{
		path = CODE_EDITOR_DATA_PATH+"/codeEditorChangeLog.txt";
		fp = fopen(path.c_str(),"a");
		if (!fp)
		{
			__SS__ << "Could not open change log for change tracking at " <<
					path << __E__;
			__SS_THROW__;
		}
		fprintf(fp,"time=%lld old-size=%lld new-size=%lld path=%s\n",
				(long long)time(0),
				oldSize,
				(long long)contents.size(),
				fullpath.c_str());

		fclose(fp);
		__COUT__ << "Changes logged to: " << path << __E__;
	}


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
					//__COUTV__(result.substr(0,i));
					__MOUT__ << result.substr(0,i);
					result = result.substr(i+1); //discard before new line
				}
			}

			//__COUTV__(result);
			__MOUT__ << result.substr(0,i);
		}
	},clean).detach();




} //end build()


//========================================================================================================================
std::map<std::string /*special type*/,std::set<std::string> /*special file paths*/>
CodeEditor::getSpecialsMap(void)
{
	std::map<std::string /*special type*/,
		std::set<std::string> /*special file paths*/> retMap;
	std::string path = std::string(getenv("MRB_SOURCE"));

	__COUTV__(path);

	const unsigned int numOfSpecials = 4;
	std::string specialFolders[] = {"FEInterfaces","DataProcessorPlugins","ControlsInterfacePlugins",
		"FEInterfacePlugins"};
	std::string specialMapTypes[] = {"FEInterface","DataProcessor","ControlsInterface",
		"FEInterface"};

	//Note: can not do lambda recursive function if using auto to declare the function,
	//	and must capture reference to the function. Also, must capture specialFolders
	//	reference for use internally (const values already are captured).
	std::function<void(const std::string&, const std::string&, const unsigned int, const int)>
	localRecurse = [&specialFolders, &specialMapTypes,
					&retMap,
					&localRecurse]
					(
							const std::string& path,
							const std::string& offsetPath,
							const unsigned int depth,
							const int specialIndex) {

		//__COUTV__(path);
		//__COUTV__(depth);

		DIR *pDIR;
		struct dirent *entry;
		bool isDir;
		if( !(pDIR=opendir(path.c_str())) )
		{
			__SS__ << "Plugin base path '" << path << "' could not be opened!" << __E__;
			__SS_THROW__;
		}

		//else directory good, get all folders and look for special folders
		std::string name;
		int type;
		int childSpecialIndex;
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
					//__COUT__ << "Directory: " << type << " " << name << __E__;

					childSpecialIndex = -1;
					for(unsigned int i=0;i<numOfSpecials;++i)
						if(name == specialFolders[i])
						{
							//__COUT__ << "Found special folder '" << specialFolders[i] <<
							//		"' at path " <<	path << __E__;

							childSpecialIndex = i;
							break;
						}

					//recurse deeper!
					if(depth < 4) //limit search depth
						localRecurse(path + "/" + name,
								offsetPath + "/" + name, depth + 1, childSpecialIndex);
				}
				else if(specialIndex >= 0)
				{
					//get special files!!

					if(
							name.find(".h") == name.length()-2 		||
							name.find(".cc") == name.length()-3 	||
							name.find(".txt") == name.length()-4
					)
					{
						//__COUT__ << "Found special '" << specialFolders[specialIndex] <<
						//		"' file '" << name << "' at path " <<
						//		path << " " << specialIndex << __E__;

						retMap[specialMapTypes[specialIndex]].emplace(
								offsetPath + "/" + name);
					}
				}
			}
		} //end directory traversal

		closedir(pDIR);

	}; //end localRecurse() definition

	//start recursive traversal to find special folders
	localRecurse(path,"" /*offsetPath*/, 0 /*depth*/, -1 /*specialIndex*/);

	__COUTV__(StringMacros::mapToString(retMap));
	return retMap;
} //end getSpecialsMap()









