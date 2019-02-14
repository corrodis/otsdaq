/*
 * RegisterView.h
 *
 *  Created on: Jul 29, 2015
 *      Author: parilla
 */

#ifndef _ots_RegisterView_h_
#define _ots_RegisterView_h_
#include "otsdaq-core/ConfigurationDataFormats/ViewRegisterInfo.h"
#include "otsdaq-core/ConfigurationDataFormats/ViewRegisterSequencerInfo.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include <stdlib.h>
#include <cassert>
#include <iostream>
#include <sstream>  // std::stringstream, std::stringbuf
#include <string>
#include <typeinfo>
#include <vector>
namespace ots
{
class RegisterView
{
  public:
	typedef std::vector<std::vector<std::string> > DataView;
	typedef DataView::iterator                     iterator;
	typedef DataView::const_iterator               const_iterator;

	RegisterView (std::string name = "");
	virtual ~RegisterView ();

	unsigned int findRow (unsigned int col, const std::string value) const;
	unsigned int findRow (unsigned int col, const unsigned int value) const;

	//Getters
	std::string                                   getName () const;
	int                                           getVersion () const;
	std::string                                   getComment () const;
	std::string                                   getAuthor () const;
	time_t                                        getCreationTime () const;
	unsigned int                                  getNumberOfRows () const;
	unsigned int                                  getNumberOfColumns () const;
	const std::vector<ViewRegisterInfo>&          getRegistersInfo () const;
	std::vector<ViewRegisterInfo>*                getRegistersInfoPointer ();
	const std::vector<ViewRegisterSequencerInfo>& getRegistersSequencerInfo () const;
	std::vector<ViewRegisterSequencerInfo>*       getRegistersSequencerInfoPointer ();

	/*template<class T>
	  void getValue(T& value, unsigned int row, unsigned int col) const
	  {
		assert(col < columnsInfo_.size() && row < getNumberOfRows());
		if(columnsInfo_[col].getDataType() == ViewColumnInfo::DATATYPE_NUMBER)
			if(typeid(double) == typeid(value))
				value = strtod(theDataView_[row][col].c_str(),0);
			else if(typeid(float) == typeid(value))
				value = strtof(theDataView_[row][col].c_str(),0);
			else
				value = strtol(theDataView_[row][col].c_str(),0,10);
		else
		{
			std::cout << __COUT_HDR_FL__ << "\tUnrecognized View data type: " << columnsInfo_[col].getDataType() << std::endl;
			assert(0);
		}
	  }
	  void getValue(std::string& value, unsigned int row, unsigned int col) const
	  {
		assert(col < columnsInfo_.size() && row < getNumberOfRows());
		  if(columnsInfo_[col].getDataType() == ViewColumnInfo::DATATYPE_STRING)
			value = theDataView_[row][col];
		else if(columnsInfo_[col].getDataType() == ViewColumnInfo::DATATYPE_TIME)
			value = theDataView_[row][col];
		else
		{
			std::cout << __COUT_HDR_FL__ << "\tUnrecognized View data type: " << columnsInfo_[col].getDataType() << std::endl;
			assert(0);
		}
	  }*/

	//Setters
	void setName (std::string name);
	void setVersion (int version);
	void setVersion (char* version);
	void setComment (std::string name);
	void setAuthor (std::string name);
	void setCreationTime (time_t t);

	/*    template<class T>
	void setValue(T value, unsigned int row, unsigned int col)
	{
    	assert(col < columnsInfo_.size() && row < getNumberOfRows());
		if(columnsInfo_[col].getDataType() == ViewColumnInfo::DATATYPE_NUMBER)
		{
			std::stringstream ss;
			ss << value;
			theDataView_[row][col] = ss.str();
			//TODO: Should this number be forced to a long int???.. currently getValue only gets long int.
		}
		else
		{
			std::cout << __COUT_HDR_FL__ << "\tUnrecognized View data type: " << columnsInfo_[col].getDataType() << std::endl;
			assert(0);
		}
	}*/
	/*void setValue(std::string value, unsigned int row, unsigned int col)
	{
    	assert(col < columnsInfo_.size() && row < getNumberOfRows());
		if(columnsInfo_[col].getDataType() == ViewColumnInfo::DATATYPE_STRING)
			theDataView_[row][col] = value;
		else if(columnsInfo_[col].getDataType() == ViewColumnInfo::DATATYPE_TIME)
			theDataView_[row][col] = value;
		else
		{
			std::cout << __COUT_HDR_FL__ << "\tUnrecognized View data type: " << columnsInfo_[col].getDataType() << std::endl;
			assert(0);
		}
	}*/

	int  addRow ();          //returns index of added row, always is last row unless
	bool deleteRow (int r);  //returns true on success

  private:
	std::string                            name_;     //View name (extensionTableName in xml)
	int                                    version_;  //Configuration version
	std::string                            comment_;  //Configuration version comment
	std::string                            author_;
	time_t                                 creationTime_;
	std::vector<ViewRegisterInfo>          registersInfo_;
	std::vector<ViewRegisterSequencerInfo> registersSequencerInfo_;
	DataView                               theDataView_;
};

}  // namespace ots

#endif /* REGISTERVIEW_H_ */
