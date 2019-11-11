#ifndef ots_XmlDocument_h
#define ots_XmlDocument_h

#include <string>
#include <vector>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

namespace ots
{
class XmlDocument
{
  public:
	XmlDocument(std::string rootName = "ROOT");
	XmlDocument(const XmlDocument& doc);
	XmlDocument& operator=(const XmlDocument& doc);
	~XmlDocument(void);

	xercesc::DOMElement* addTextElementToParent(std::string childName, std::string childText, xercesc::DOMElement* parent);
	xercesc::DOMElement* addTextElementToParent(std::string childName, std::string childText, std::string parentName, unsigned int parentIndex = 0);

	void saveXmlDocument(std::string filePath);
	void recursiveRemoveChild(xercesc::DOMElement* childEl,
	                          xercesc::DOMElement* parentEl);  // remove entire element
	                                                           // and sub tree, recursively

	bool loadXmlDocument(std::string filePath);

	void outputXmlDocument(std::ostringstream* out, bool dispStdOut = false);

  protected:
	void copyDocument(const xercesc::DOMDocument* toCopy, xercesc::DOMDocument* copy);
	void recursiveElementCopy(const xercesc::DOMElement* toCopy, xercesc::DOMElement* copy);
	void initDocument(void);
	void initPlatform(void);
	void terminatePlatform(void);
	void recursiveOutputXmlDocument(xercesc::DOMElement* currEl, std::ostringstream* out, bool dispStdOut = false, std::string tabStr = "");

	//	unsigned int 	addElementToParent				(std::string field, std::string
	// value,  xercesc::DOMElement *parentEl, bool verbose=false); 	void
	// recursiveAddElementToParent 	(xercesc::DOMElement *currEl, xercesc::DOMElement
	//*parentEl);
	//    std::string     recursiveFindElement            (xercesc::DOMElement *currEl,
	//    const std::string field, const unsigned int occurance, unsigned int &count);
	//    void            recursiveFindAllElements        (xercesc::DOMElement *currEl,
	//    const std::string field,std::vector<std::string> *retVec);
	std::string escapeString(std::string inString, bool allowWhiteSpace = false);

	xercesc::DOMImplementation* theImplementation_;
	xercesc::DOMDocument*       theDocument_;
	xercesc::DOMElement*        rootElement_;

	const std::string rootTagName_;
};

}  // namespace ots

#endif  // ots_XmlDocument_h
