#ifndef _ots_ConfigurationTree_h_
#define _ots_ConfigurationTree_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationView.h"
//#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
//#include <xercesc/dom/DOMElement.hpp>

#include <iostream>       // std::cout
#include <string>
#include <set>


namespace ots
{

class ConfigurationManager;
class ConfigurationBase;
class ConfigurationView;

class ConfigurationTree
{
	friend class ConfigurationGUISupervisor;

public:
	//Note: due to const members, implicit copy constructor exists, but NOT assignment operator=
	//	... so ConfigurationTree t = mytree.GetNode(nodeString); //ok
	//	... or ConfigurationTree t(mytree.GetNode(nodeString)); //ok
	//	... but mytree = mytree.GetNode(nodeString); //does NOT work
	ConfigurationTree							();
//	ConfigurationTree(const ConfigurationTree& a)
//	:
//		configMgr_				(a.configMgr_),
//	  configuration_			(a.configuration_),
//	  groupId_					(a.groupId_),
//	  linkColName_				(a.linkColName_),
//	  disconnectedTargetName_ 	(a.disconnectedTargetName_),
//	  childLinkIndex_			(a.childLinkIndex_),
//	  row_						(a.row_),
//	  col_						(a.col_),
//	  configView_				(a.configView_)
//	{
//		std::cout << __PRETTY_FUNCTION__ << std::endl;
//		//return *this;
//	}

	ConfigurationTree							(const ConfigurationManager* const& configMgr, const ConfigurationBase* const &config);
	~ConfigurationTree							(void);

	ConfigurationTree& operator=(const ConfigurationTree& a)
	{
		std::cout << __PRETTY_FUNCTION__ << "OPERATOR= COPY CONSTRUCTOR ConfigManager: " << configMgr_ << " configuration: " << configuration_  << std::endl;
		//Note: Members of the ConfigurationTree are declared constant.
		//	(Refer to comments at top of class declaration for solutions)
		//	So this operator cannot work.. SO I am going to crash just in case it is called by mistake
		std::cout << __PRETTY_FUNCTION__ << "OPERATOR= COPY CONSTRUCTOR CANNOT BE USED - SO YOUR CODE IS WRONG! Crashing now." << std::endl;
		std::cout << __PRETTY_FUNCTION__ << "OPERATOR= COPY CONSTRUCTOR CANNOT BE USED - SO YOUR CODE IS WRONG! Crashing now." << std::endl;
		std::cout << __PRETTY_FUNCTION__ << "OPERATOR= COPY CONSTRUCTOR CANNOT BE USED - SO YOUR CODE IS WRONG! Crashing now." << std::endl;
		std::cout << __PRETTY_FUNCTION__ << "OPERATOR= COPY CONSTRUCTOR CANNOT BE USED - SO YOUR CODE IS WRONG! Crashing now." << std::endl;
		std::cout << __PRETTY_FUNCTION__ << "OPERATOR= COPY CONSTRUCTOR CANNOT BE USED - SO YOUR CODE IS WRONG! Crashing now." << std::endl;
		std::cout << __PRETTY_FUNCTION__ << "OPERATOR= COPY CONSTRUCTOR CANNOT BE USED - SO YOUR CODE IS WRONG! Crashing now." << std::endl;
		std::cout << __PRETTY_FUNCTION__ << "OPERATOR= COPY CONSTRUCTOR CANNOT BE USED - SO YOUR CODE IS WRONG! Crashing now." << std::endl;
		exit(0);

		//copy to const members is not allowed.. but would look like this:

		configMgr_	    = a.configMgr_;
		configuration_  = a.configuration_;
		//groupId_		= a.groupId_;
		//linkColName_	= a.linkColName_;
		//childLinkIndex_ = a.childLinkIndex_;
		//row_			= a.row_;
		//col_			= a.col_;
		configView_	    = a.configView_;
		std::cout << __PRETTY_FUNCTION__ << "OPERATOR COPY CONSTRUCTOR" << std::endl;
		return *this;
	};


	static const std::string DISCONNECTED_VALUE;
	static const std::string VALUE_TYPE_DISCONNECTED;
	static const std::string VALUE_TYPE_NODE;

	static const std::string NODE_TYPE_GROUP_TABLE;
	static const std::string NODE_TYPE_TABLE;
	static const std::string NODE_TYPE_GROUP_LINK;
	static const std::string NODE_TYPE_UID_LINK;
	static const std::string NODE_TYPE_VALUE;
	static const std::string NODE_TYPE_UID;


	//Methods


	//==============================================================================
	//getValue (not std::string value)
	//	throw exception unless it value node
	template<class T>
	void getValue(T& value) const
	{
		if(row_ != ConfigurationView::INVALID && col_ != ConfigurationView::INVALID)	//this node is a value node
			configView_->getValue(value,row_,col_);
		else if(row_ == ConfigurationView::INVALID && col_ == ConfigurationView::INVALID)	//this node is config node maybe with groupId
			throw std::runtime_error("Requesting getValue on config node level. Must be a value node.");
		else if(row_ == ConfigurationView::INVALID)
		{
			std::cout << __COUT_HDR_FL__ << std::endl;
			throw std::runtime_error("Malformed ConfigurationTree");
		}
		else if(col_ == ConfigurationView::INVALID)						//this node is uid node
			throw std::runtime_error("Requesting getValue on uid node level. Must be a value node.");
		else
		{
			std::cout << __COUT_HDR_FL__ << std::endl;
			throw std::runtime_error("Impossible");
		}
	}

	//==============================================================================
	//getValue (not std::string value)
	//	throw exception unless it value node
	template<class T>
	T getValue(void) const
	{
		T value;
		if(row_ != ConfigurationView::INVALID && col_ != ConfigurationView::INVALID)	//this node is a value node
		{
			configView_->getValue(value,row_,col_);
			return value;
		}
		else if(row_ == ConfigurationView::INVALID && col_ == ConfigurationView::INVALID)	//this node is config node maybe with groupId
			throw std::runtime_error("Requesting getValue on config node level. Must be a value node.");
		else if(row_ == ConfigurationView::INVALID)
		{
			std::cout << __COUT_HDR_FL__ << std::endl;
			throw std::runtime_error("Malformed ConfigurationTree");
		}
		else if(col_ == ConfigurationView::INVALID)						//this node is uid node
			throw std::runtime_error("Requesting getValue on uid node level. Must be a value node.");
		else
		{
			std::cout << __COUT_HDR_FL__ << std::endl;
			throw std::runtime_error("Impossible");
		}
	}

	//navigating between nodes
	ConfigurationTree						getNode				        (const std::string& nodeName, bool doNotThrowOnBrokenUIDLinks=false) const;
	ConfigurationTree				        getBackNode 		        (      std::string  nodeName, unsigned int backSteps=1) const;


	//extracting information from node
	const std::string&						getConfigurationName		(void) const;
	const ConfigurationVersion&				getConfigurationVersion		(void) const;
	const time_t&							getConfigurationCreationTime(void) const;
	std::vector<std::string>				getChildrenNames	        (void) const;
	std::vector<std::pair<std::string,ConfigurationTree> >	getChildren	(std::map<std::string /*relative-path*/, std::string /*value*/> filterMap = std::map<std::string /*relative-path*/, std::string /*value*/>()) const;
	std::map<std::string,ConfigurationTree> getChildrenMap	            (void) const;
	void									getValue			        (std::string& value) const;
	std::string								getValue			        (void) const;
	std::string								getEscapedValue		        (void) const;
	const std::string&						getValueAsString	        (bool returnLinkTableValue=false) const;
	const std::string&						getUIDAsString		        (void) const;
	const std::string&						getValueDataType	        (void) const;
	const std::string&						getValueType	        	(void) const;
	const std::string&						getValueName		        (void) const;
	std::string								getNodeType			        (void) const;
	const std::string&						getDisconnectedTableName	(void) const;
	const std::string&						getDisconnectedLinkID		(void) const;
	const std::string&						getChildLinkIndex			(void) const;
	std::vector<std::string>				getFixedChoices				(void) const;

protected:
	const unsigned int&						getRow			        	(void) const;
	const unsigned int&						getColumn		        	(void) const;
	const ViewColumnInfo&					getColumnInfo	        	(void) const;

	//extracting information from a list of records
	struct RecordField
	{
		RecordField(const std::string &table, const std::string &uid,
				const std::string &columnName, const std::string &relativePath,
				const ViewColumnInfo *columnInfo)
		:tableName_(table)
		,columnName_(columnName)
		,relativePath_(relativePath)
		,columnInfo_(columnInfo)
		{}

		std::string tableName_, columnName_, relativePath_;
		//relativePath_ is relative to record uid node, not including columnName_

		const ViewColumnInfo *columnInfo_;
	};
	std::vector<ConfigurationTree::RecordField>		getCommonFields(const std::vector<std::string /*relative-path*/> &recordList, const std::vector<std::string /*relative-path*/> &fieldFilterList, unsigned int depth = -1) const;
	std::set<std::string /*unique-value*/>			getUniqueValuesForField(const std::vector<std::string /*relative-path*/> &recordList, const std::string &fieldName) const;


public:


	//boolean info
	bool									isDefaultValue				(void) const;
	bool									isConfigurationNode	        (void) const;
	bool									isValueNode			        (void) const;
	bool									isDisconnected		        (void) const;
	bool									isLinkNode			        (void) const;
	bool									isGroupLinkNode		        (void) const;
	bool									isUIDLinkNode		        (void) const;
	bool									isUIDNode			        (void) const;


	void         			 				print				        (const unsigned int &depth = -1, std::ostream &out = std::cout) const;

	//make stream output easy
	friend std::ostream& operator<<	(std::ostream& out, const ConfigurationTree& t)
	{
		out << t.getValueAsString();
		return out;
	}
private:
	//privately ONLY allow full access to member variables through constructor
	ConfigurationTree(const ConfigurationManager* const& configMgr, const ConfigurationBase* const& config, const std::string& groupId, const std::string &linkColName, const std::string &linkColValue, const std::string& disconnectedTargetName, const std::string& disconnectedLinkID, const std::string &childLinkIndex, const unsigned int row  = ConfigurationView::INVALID, const unsigned int col = ConfigurationView::INVALID);

	static ConfigurationTree	recurse		  (const ConfigurationTree& t, const std::string& childPath);
	static void 				recursivePrint(const ConfigurationTree& t, unsigned int depth, std::ostream &out, std::string space);

	void						recursiveGetCommonFields(std::vector<ConfigurationTree::RecordField> &fieldCandidateList, std::vector<int> &fieldCount, const std::vector<std::string /*relative-path*/> &fieldFilterList, unsigned int depth, const std::string &relativePathBase, bool inFirstRecord) const;

	//std::string					getRecordFieldValueAsString(std::string fieldName) const;

	//Any given ConfigurationTree is either a config, uid, or value node:
	//	- config node is a pointer to a config table
	//	- uid node is a pointer to a row in a config table
	//	- value node is a pointer to a cell in a config table
	//
	//Assumption: uid column is present
	const ConfigurationManager* configMgr_;
	const ConfigurationBase* 	configuration_; //config node
	const std::string			groupId_;		//group config node
	const std::string			linkColName_;	//link node field name
	const std::string			linkColValue_;	//link node field value
	const std::string			disconnectedTargetName_;	//only used if disconnected to determine target table name
	const std::string			disconnectedLinkID_;	//only used if disconnected to determine target link ID
	const std::string			childLinkIndex_;//child link index
	const unsigned int			row_;			//uid node
	const unsigned int			col_;			//value node
	const ConfigurationView*	configView_;

};
}

#endif
