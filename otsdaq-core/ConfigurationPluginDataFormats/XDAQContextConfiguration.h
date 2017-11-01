#ifndef _ots_XDAQContextConfiguration_h_
#define _ots_XDAQContextConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include <string>

namespace ots
{

class XDAQContextConfiguration : public ConfigurationBase
{
public:
	struct XDAQApplication
	{
		std::string  applicationGroupID_;
		std::string  applicationUID_;
		bool		 status_;
		std::string  class_;
		unsigned int id_;
		unsigned int instance_;
		std::string  network_;
		std::string  group_;
		std::string  module_;
		std::string  sourceConfig_;
	};

	struct XDAQContext
	{
		std::string  contextUID_;
		std::string  sourceConfig_;
		bool		 status_;
		unsigned int id_;
		std::string  address_;
		unsigned int port_;
		std::vector<XDAQApplication> applications_;
	};

	XDAQContextConfiguration(void);
	virtual ~XDAQContextConfiguration(void);

	//Methods
	void 								init					(ConfigurationManager *configManager);
	void 								extractContexts			(ConfigurationManager *configManager);
	void 								outputXDAQXML			(std::ostream &out);
	//void 								outputXDAQScript		(std::ostream &out);
	//void 								outputARTDAQScript		(std::ostream &out);

	std::string 						getContextUID			(const std::string &url) const;
	std::string 						getApplicationUID		(const std::string &url, unsigned int id) const;


	const std::vector<XDAQContext> & getContexts			() { return contexts_; }


	ConfigurationTree 					getSupervisorConfigNode	(ConfigurationManager *configManager, const std::string &contextUID, const std::string &appUID) const;

	//artdaq specific get methods
	std::vector<const XDAQContext *>	getBoardReaderContexts	() const;
	std::vector<const XDAQContext *>	getEventBuilderContexts	() const;
	std::vector<const XDAQContext *>	getAggregatorContexts	() const;
	unsigned int						getARTDAQAppRank		(const std::string &contextUID = "X") const;
	static bool							isARTDAQContext			(const std::string &contextUID);

private:

	std::vector<XDAQContext> contexts_;
	std::vector<unsigned int> artdaqContexts_;

	std::vector<unsigned int> artdaqBoardReaders_;
	std::vector<unsigned int> artdaqEventBuilders_;
	std::vector<unsigned int> artdaqAggregators_;

public:

	//XDAQ Context Column names
	struct ColContext
	{
		std::string const colContextUID_                     = "ContextUID";
		std::string const colLinkToApplicationConfiguration_ = "LinkToApplicationConfiguration";
		std::string const colLinkToApplicationGroupID_       = "LinkToApplicationGroupID";
		std::string const colStatus_                         = ViewColumnInfo::COL_NAME_STATUS;
		std::string const colId_                             = "Id";
		std::string const colAddress_                        = "Address";
		std::string const colPort_                           = "Port";
	} colContext_;

	//XDAQ App Column names
	struct ColApplication
	{
		std::string const colApplicationGroupID_            = "ApplicationGroupID";
		std::string const colApplicationUID_                = "ApplicationUID";
		std::string const colLinkToSupervisorConfiguration_ = "LinkToSupervisorConfiguration";
		std::string const colLinkToSupervisorUID_           = "LinkToSupervisorUID";
		std::string const colStatus_                        = ViewColumnInfo::COL_NAME_STATUS;
		std::string const colClass_                         = "Class";
		std::string const colId_                            = "Id";
		std::string const colInstance_                      = "Instance";
		std::string const colNetwork_                       = "Network";
		std::string const colGroup_                         = "Group";
		std::string const colModule_                        = "Module";
	} colApplication_;
};
}
#endif
