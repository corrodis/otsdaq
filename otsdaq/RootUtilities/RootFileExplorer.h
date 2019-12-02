#ifndef __RootFileExplorer_H__
#define __RootFileExplorer_H__

#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <list>
#include <map> 
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "otsdaq/Macros/CoutMacros.h"
#include "otsdaq/MessageFacility/MessageFacility.h"
//#include "otsdaq/XmlUtilities/ConvertFromXML.h"
//#include "otsdaq/XmlUtilities/ConvertToXML.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/validators/common/Grammar.hpp>
//#include <xercesc/dom/DOMLSSerializer.hpp>
//#include <xercesc/dom/DOMLSOutput.hpp>

#include "otsdaq/XmlUtilities/HttpXmlDocument.h"

#if defined(XERCES_NEW_IOSTREAMS)
#include <iostream>
#else
#include <iostream.h>
#endif

#include <TDirectory.h>
#include <TFile.h>
#include <TKey.h>

using namespace std ;
using namespace ots ;

// clang-format off
class RootFileExplorer
{
 public:

                         RootFileExplorer       (string                fSystemPath     ,
                                                 string                fRootPath       ,
                                                 string                fFoldersPath    ,
                                                 string                fHistName       ,
                                                 string                fFileName       ,
                                                 HttpXmlDocument     & xmlOut           )  ;
                        ~RootFileExplorer       (void                                   ) {;}
  xercesc::DOMDocument * initialize             (void                                   )  ;
  void                   makeDirectoryBinaryTree(TDirectory          * currentDirectory,
                                                 int                   indent          ,
                                                 xercesc::DOMElement * anchorNode       )  ;
  xercesc::DOMElement  * populateBinaryTreeNode (xercesc::DOMElement * anchorNode      ,
                                                 std::string           name            ,
                                                 int                   level           ,
                                                 bool                  isLeaf           )  ;
 
 private:

  bool                                    debug_            ;
  string                                  fSystemPath_      ;
  string                                  fRootPath_        ;
  string                                  fFoldersPath_     ;
  string                                  fFileName_        ;
  string                                  fRFoldersPath_    ;
  string                                  fHistName_        ;
  std::string                             fThisFolderPath_  ;
  xercesc::DOMImplementation            * theImplementation_;
  xercesc::DOMDocument                  * theDocument_      ;
  xercesc::DOMElement                   * rootElement_      ;
  std::map<int, xercesc::DOMElement *>    theNodes_         ;
  std::map<int, std::string>              theHierarchy_     ;
  std::map<bool,std::string>              isALeaf_          ;
  const std::string                       rootTagName_      ;
  TFile                                 * rootFile_         ;
  int                                     level_            ;
  stringstream                            ss_               ;
  HttpXmlDocument                         xmlOut_           ;
} ;
// clang-format on
#endif
