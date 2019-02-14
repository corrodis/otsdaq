/*
 * RegisterConfigurationInfoReader.h
 *
 *  Created on: Jul 28, 2015
 *      Author: parilla
 */

#ifndef ots_RegisterConfigurationInfoReader_h_
#define ots_RegisterConfigurationInfoReader_h_

#include <string>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/util/XMLChar.hpp>

namespace ots
{
class RegisterBase;

class RegisterConfigurationInfoReader
{
  public:
	RegisterConfigurationInfoReader ();
	virtual ~RegisterConfigurationInfoReader ();
	void read (RegisterBase& configuration);
	void read (RegisterBase* configuration);

  private:
	void initPlatform (void);
	void terminatePlatform (void);
	bool checkViewType (std::string type);

	xercesc::DOMNode*    getNode (XMLCh* tagName, xercesc::DOMNode* parent, unsigned int itemNumber);
	xercesc::DOMNode*    getNode (XMLCh* tagName, xercesc::DOMElement* parent, unsigned int itemNumber);
	xercesc::DOMElement* getElement (XMLCh* tagName, xercesc::DOMNode* parent, unsigned int itemNumber);
	xercesc::DOMElement* getElement (XMLCh* tagName, xercesc::DOMElement* parent, unsigned int itemNumber);
	XMLCh*               rootTag_;
	XMLCh*               headerTag_;
	XMLCh*               typeTag_;
	XMLCh*               extensionTableNameTag_;
	XMLCh*               nameTag_;
	XMLCh*               dataSetTag_;
	XMLCh*               versionTag_;
	XMLCh*               commentDescriptionTag_;
	XMLCh*               createdByUserTag_;
	XMLCh*               dataTag_;
	XMLCh*               typeNameTag_;
	XMLCh*               registerNameTag_;
	XMLCh*               baseAddressTag_;
	XMLCh*               sizeTag_;
	XMLCh*               accessTag_;
};

}  // namespace ots

#endif /* RegisterConfigurationINFOREADER_H_ */
