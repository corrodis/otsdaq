/*
 * RegisterSequencerInfoReader.h
 *
 *  Created on: Jul 29, 2015
 *      Author: parilla
 */

#ifndef REGISTERSEQUENCERINFOREADER_H_
#define REGISTERSEQUENCERINFOREADER_H_
#include <string>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/util/XMLChar.hpp>
#include <xercesc/util/XMLString.hpp>

namespace ots
{
class RegisterBase;

class RegisterSequencerInfoReader
{
  public:
	RegisterSequencerInfoReader();
	virtual ~RegisterSequencerInfoReader();
	void read(RegisterBase& configuration);
	void read(RegisterBase* configuration);

  private:
	void initPlatform(void);
	void terminatePlatform(void);
	bool checkViewType(std::string type);

	xercesc::DOMNode*    getNode(XMLCh*            tagName,
	                             xercesc::DOMNode* parent,
	                             unsigned int      itemNumber);
	xercesc::DOMNode*    getNode(XMLCh*               tagName,
	                             xercesc::DOMElement* parent,
	                             unsigned int         itemNumber);
	xercesc::DOMElement* getElement(XMLCh*            tagName,
	                                xercesc::DOMNode* parent,
	                                unsigned int      itemNumber);
	xercesc::DOMElement* getElement(XMLCh*               tagName,
	                                xercesc::DOMElement* parent,
	                                unsigned int         itemNumber);
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
	XMLCh*               componentNameTag_;
	XMLCh*               registerNameTag_;
	XMLCh*               registerValueTag_;
	XMLCh*               sequenceNumberTag_;
	XMLCh*               stateTag_;
};

}  // namespace ots

#endif /* REGISTERSEQUENCERINFOREADER_H_ */
