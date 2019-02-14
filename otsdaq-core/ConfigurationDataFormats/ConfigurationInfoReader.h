#ifndef _ots_ConfigurationInfoReader_h_
#define _ots_ConfigurationInfoReader_h_

#include <string>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/util/XMLChar.hpp>

namespace ots
{
class ConfigurationBase;

class ConfigurationInfoReader
{
  public:
	ConfigurationInfoReader(bool allowIllegalColumns = false);
	~ConfigurationInfoReader(void);
	std::string read(ConfigurationBase& configuration);
	std::string read(ConfigurationBase* configuration);

	void        setAllowColumnErrors(bool setValue);
	const bool& getAllowColumnErrors(void);

  private:
	void initPlatform(void);
	void terminatePlatform(void);
	bool checkViewType(std::string type);

	xercesc::DOMNode*    getNode(XMLCh* tagName, xercesc::DOMNode* parent, unsigned int itemNumber);
	xercesc::DOMNode*    getNode(XMLCh* tagName, xercesc::DOMElement* parent, unsigned int itemNumber);
	xercesc::DOMElement* getElement(XMLCh* tagName, xercesc::DOMNode* parent, unsigned int itemNumber);
	xercesc::DOMElement* getElement(XMLCh* tagName, xercesc::DOMElement* parent, unsigned int itemNumber);
	XMLCh*               rootTag_;
	XMLCh*               configurationTag_;
	XMLCh*               configurationNameAttributeTag_;
	XMLCh*               viewTag_;
	XMLCh*               viewNameAttributeTag_;
	XMLCh*               viewTypeAttributeTag_;
	XMLCh*               viewDescriptionAttributeTag_;
	XMLCh*               columnTag_;
	XMLCh*               columnTypeAttributeTag_;
	XMLCh*               columnNameAttributeTag_;
	XMLCh*               columnStorageNameAttributeTag_;
	XMLCh*               columnDataTypeAttributeTag_;
	XMLCh*               columnDataChoicesAttributeTag_;

	bool allowIllegalColumns_;
};

}  // namespace ots

#endif
