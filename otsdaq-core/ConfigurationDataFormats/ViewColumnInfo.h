#ifndef _ots_ViewColumnInfo_h_
#define _ots_ViewColumnInfo_h_

#include <string>
#include <vector>
#include <map>
#include <memory> /* shared_ptr */

namespace ots
{

class ViewColumnInfo
{

public:

	ViewColumnInfo(const std::string &type, const std::string &name, const std::string &storageName, const std::string &dataType, const std::string &dataChoicesCSV, std::string *capturedExceptionString);
	ViewColumnInfo(const ViewColumnInfo& c); //copy constructor because of bitmap pointer
	ViewColumnInfo& operator=(const ViewColumnInfo& c); //assignment operator because of bitmap pointer



    virtual ~ViewColumnInfo(void);

  	const std::string& 					getType       	(void) const;
  	const std::string& 					getName       	(void) const;
  	const std::string& 					getStorageName	(void) const;
  	const std::string& 					getDataType   	(void) const;
  	const std::string& 					getDefaultValue	(void) const;
  	const std::vector<std::string>& 	getDataChoices 	(void) const;

  	struct BitMapInfo //uses dataChoices CSV fields if type is TYPE_BITMAP_DATA
  	{
  		BitMapInfo()
  		:minColor_("")
  		,midColor_("")
  		,maxColor_("")
  		{}
  		unsigned int 	numOfRows_, numOfColumns_, cellBitSize_;
  		uint64_t 		minValue_, maxValue_, stepValue_;
  		std::string 	aspectRatio_;
  		std::string 	minColor_, midColor_, maxColor_;
  		std::string 	absMinColor_, absMaxColor_;
  		bool			rowsAscending_, colsAscending_, snakeRows_, snakeCols_;
  	};
  	const BitMapInfo& 					getBitMapInfo 	(void) const; //uses dataChoices CSV fields if type is TYPE_BITMAP_DATA

  	static std::vector<std::string> 	getAllTypesForGUI 	(void);
  	static std::map<std::pair<std::string,std::string>,std::string> 	getAllDefaultsForGUI(void);
  	static std::vector<std::string>		getAllDataTypesForGUI(void);

  	const bool							isChildLink			(void) const;
  	const bool							isChildLinkUID		(void) const;
  	const bool							isChildLinkGroupID	(void) const;
  	const bool							isGroupID			(void) const;

  	std::string							getChildLinkIndex	(void) const;

  	static const std::string TYPE_UID;
  	static const std::string TYPE_DATA, TYPE_UNIQUE_DATA, TYPE_MULTILINE_DATA, TYPE_FIXED_CHOICE_DATA, TYPE_BITMAP_DATA;
	static const std::string TYPE_ON_OFF, TYPE_TRUE_FALSE, TYPE_YES_NO;
  	static const std::string TYPE_COMMENT, TYPE_AUTHOR, TYPE_TIMESTAMP;
  	static const std::string TYPE_START_CHILD_LINK, TYPE_START_CHILD_LINK_UID, TYPE_START_CHILD_LINK_GROUP_ID, TYPE_START_GROUP_ID;
  	static const std::string DATATYPE_NUMBER, DATATYPE_STRING, DATATYPE_TIME;

  	static const std::string TYPE_VALUE_YES		;
  	static const std::string TYPE_VALUE_NO		;
  	static const std::string TYPE_VALUE_TRUE	;
  	static const std::string TYPE_VALUE_FALSE	;
  	static const std::string TYPE_VALUE_ON		;
  	static const std::string TYPE_VALUE_OFF		;

  	static const std::string DATATYPE_STRING_DEFAULT	;
  	static const std::string DATATYPE_COMMENT_DEFAULT	;
  	static const std::string DATATYPE_BOOL_DEFAULT		;
  	static const std::string DATATYPE_NUMBER_DEFAULT	;
  	static const std::string DATATYPE_TIME_DEFAULT		;
  	static const std::string DATATYPE_LINK_DEFAULT		;

  	static const std::string COL_NAME_STATUS, COL_NAME_PRIORITY;

private:
  	ViewColumnInfo(); //private constructor, only used in assignment operator
  	void								extractBitMapInfo();

protected:
  	std::string type_;
  	std::string name_;
  	std::string storageName_;
  	std::string dataType_;
  	std::vector<std::string> dataChoices_;
  	BitMapInfo* bitMapInfoP_;
};


}

#endif
