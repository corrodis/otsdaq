#include <fhiclcpp/ParameterSet.h>
#include <fhiclcpp/detail/print_mode.h>
#include <fhiclcpp/intermediate_table.h>
#include <fhiclcpp/make_ParameterSet.h>
#include <fhiclcpp/parse.h>

#include "otsdaq-core/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"

#include <fstream>   // for std::ofstream
#include <iostream>  // std::cout
#include <typeinfo>

#include "otsdaq-core/TableCore/TableInfoReader.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "ARTDAQTableBase-" + getTableName()

#define ARTDAQ_FCL_PATH std::string(__ENV__("USER_DATA")) + "/" + "ARTDAQConfigurations/"

//==============================================================================
// TableBase
//	If a valid string pointer is passed in accumulatedExceptions
//	then allowIllegalColumns is set for InfoReader
//	If accumulatedExceptions pointer = 0, then illegal columns throw std::runtime_error
// exception
ARTDAQTableBase::ARTDAQTableBase(std::string  tableName,
                                 std::string* accumulatedExceptions /* =0 */)
    : TableBase(tableName, accumulatedExceptions)
{
	// make directory just in case
	mkdir((ARTDAQ_FCL_PATH).c_str(), 0755);
}  // end constuctor()

//==============================================================================
// ARTDAQTableBase
//	Default constructor should never be used because table type is lost
ARTDAQTableBase::ARTDAQTableBase(void) : TableBase("ARTDAQTableBase")
{
	__SS__ << "Should not call void constructor, table type is lost!" << __E__;
	__SS_THROW__;
}  // end illegal default constructor()

//==============================================================================
ARTDAQTableBase::~ARTDAQTableBase(void) {}  // end destructor()

//========================================================================================================================
std::string ARTDAQTableBase::getFHICLFilename(const std::string& type,
                                              const std::string& name)
{
	__COUT__ << "Type: " << type << " Name: " << name << __E__;
	std::string filename = ARTDAQ_FCL_PATH + type + "-";
	std::string uid      = name;
	for(unsigned int i = 0; i < uid.size(); ++i)
		if((uid[i] >= 'a' && uid[i] <= 'z') || (uid[i] >= 'A' && uid[i] <= 'Z') ||
		   (uid[i] >= '0' && uid[i] <= '9'))  // only allow alpha numeric in file name
			filename += uid[i];

	filename += ".fcl";

	__COUT__ << "fcl: " << filename << __E__;

	return filename;
}  // end getFHICLFilename()

//========================================================================================================================
std::string ARTDAQTableBase::getFlatFHICLFilename(const std::string& type,
                                                  const std::string& name)
{
	__COUT__ << "Type: " << type << " Name: " << name << __E__;
	std::string filename = ARTDAQ_FCL_PATH + type + "-";
	std::string uid      = name;
	for(unsigned int i = 0; i < uid.size(); ++i)
		if((uid[i] >= 'a' && uid[i] <= 'z') || (uid[i] >= 'A' && uid[i] <= 'Z') ||
		   (uid[i] >= '0' && uid[i] <= '9'))  // only allow alpha numeric in file name
			filename += uid[i];

	filename += "_flattened.fcl";

	__COUT__ << "fcl: " << filename << __E__;

	return filename;
}  // end getFlatFHICLFilename()

//========================================================================================================================
void ARTDAQTableBase::flattenFHICL(const std::string& type, const std::string& name)
{
	__COUT__ << "flattenFHICL()" << __E__;
	std::string inFile  = getFHICLFilename(type, name);
	std::string outFile = getFlatFHICLFilename(type, name);

	__COUTV__(__ENV__("FHICL_FILE_PATH"));

	cet::filepath_lookup_nonabsolute policy("FHICL_FILE_PATH");

	fhicl::ParameterSet pset;

	try
	{
		fhicl::intermediate_table tbl;
		fhicl::parse_document(inFile, policy, tbl);
		fhicl::ParameterSet pset;
		fhicl::make_ParameterSet(tbl, pset);

		std::ofstream ofs{outFile};
		ofs << pset.to_indented_string(0, fhicl::detail::print_mode::annotated);
	}
	catch(cet::exception const& e)
	{
		__SS__ << "Failed to parse fhicl: " << e.what() << __E__;
		__SS_THROW__;
	}
}  // end flattenFHICL()

//========================================================================================================================
// insertParameters
//	Inserts parameters in FHiCL outputs stream.
//
// 	onlyInsertAtTableParameters allows @table:: parameters only,
//	so that calling code can do two passes (i.e. top of fcl block, @table:: only,
//	and bottom of fcl block, ignore/skip @table:: as default behavior).
void ARTDAQTableBase::insertParameters(std::ostream&      out,
                                       std::string&       tabStr,
                                       std::string&       commentStr,
                                       ConfigurationTree  parameterGroupLink,
                                       const std::string& parameterPreamble,
                                       bool onlyInsertAtTableParameters /*=false*/,
                                       bool includeAtTableParameters /*=false*/)
{
	// skip if link is disconnected
	if(!parameterGroupLink.isDisconnected())
	{
		///////////////////////
		auto otherParameters = parameterGroupLink.getChildren();

		std::string key;
		//__COUTV__(otherParameters.size());
		for(auto& parameter : otherParameters)
		{
			key = parameter.second.getNode(parameterPreamble + "Key").getValue();

			// handle special keyword @table:: (which imports full tables, usually as
			// defaults)
			if(key.find("@table::") != std::string::npos)
			{
				// include @table::
				if(onlyInsertAtTableParameters || includeAtTableParameters)
				{
					if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
					        .getValue<bool>())
						PUSHCOMMENT;

					OUT << key;

					// skip connecting : if special keywords found
					OUT << parameter.second.getNode(parameterPreamble + "Value")
					           .getValue()
					    << "\n";

					if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
					        .getValue<bool>())
						POPCOMMENT;
				}
				// else skip it

				continue;
			}
			// else NOT @table:: keyword parameter
			
			if(onlyInsertAtTableParameters) continue; //skip all other types

			if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				PUSHCOMMENT;

			OUT << key;

			// skip connecting : if special keywords found
			if(key.find("#include") == std::string::npos)
				OUT << ":";
			OUT << parameter.second.getNode(parameterPreamble + "Value").getValue()
			    << "\n";

			if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				POPCOMMENT;
		}
	}
	// else
	//	__COUT__ << "No parameters found" << __E__;

}  // end insertParameters



//========================================================================================================================
// insertModuleType
//	Inserts module type field, with consideration for @table:: 
std::string ARTDAQTableBase::insertModuleType(std::ostream&      out,
                                       std::string&       tabStr,
                                       std::string&       commentStr,
                                       ConfigurationTree  moduleTypeNode)
{
	std::string value = moduleTypeNode.getValue();
	
	OUT;
	if(value.find("@table::") == std::string::npos)
		out << "module_type: ";
	out << value << "\n";	
	
	return value;
} //end 

