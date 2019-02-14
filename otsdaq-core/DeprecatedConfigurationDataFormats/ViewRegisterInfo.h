/*
 * ViewRegisterInfo.h
 *
 *  Created on: Jul 29, 2015
 *      Author: parilla
 *
 *
 *
 */

#ifndef VIEWREGISTERINFO_H_
#define VIEWREGISTERINFO_H_

#include <string>
#include <vector>

namespace ots {

class ViewRegisterInfo {
       public:
	ViewRegisterInfo(std::string typeName, std::string registerName, std::string baseAddress, int size, std::string access);
	virtual ~ViewRegisterInfo();

	const std::string& getTypeName(void) const;
	const std::string& getRegisterName(void) const;
	const std::string& getBaseAddress(void) const;
	const int	  getSize(void) const;
	const std::string& getAccess(void) const;
	const int	  getNumberOfColumns(void) const;

       protected:
	std::vector<std::string> dataTable_;
	int			 typeName_;
	int			 registerName_;
	int			 baseAddress_;
	int			 size_;
	int			 access_;
};

}  // namespace ots

#endif /* VIEWREGISTERINFO_H_ */
