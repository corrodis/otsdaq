#ifndef ots_DOMTreeErrorReporter_h
#define ots_DOMTreeErrorReporter_h

#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <string>

namespace ots
{

class DOMTreeErrorReporter : public xercesc::ErrorHandler
{
public:
    DOMTreeErrorReporter();
    ~DOMTreeErrorReporter();

    void warning    (const xercesc::SAXParseException& exception);
    void error      (const xercesc::SAXParseException& exception);
    void fatalError (const xercesc::SAXParseException& exception);
    void resetErrors(void);
private:
    std::string reportParseException(const xercesc::SAXParseException& exception);

};

}
#endif
