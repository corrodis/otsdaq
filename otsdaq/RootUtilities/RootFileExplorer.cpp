#include "otsdaq/RootUtilities/RootFileExplorer.h"
//#include "otsdaq_cmsoutertracker/otsdaq-cmsoutertracker/Ph2_ACF/Utils/MessageTools.h"

#define STDLINE(X,Y) __COUT_WARN__ << X << __E__;

//================================================================================================
RootFileExplorer::RootFileExplorer(
		std::string filePath,
		std::string rootPath,
                                   HttpXmlDocument & xmlOut
                                  ) : rootTagName_("ROOT")
{
 filePath_  = filePath                    ;
 rootPath_  = rootPath                    ;
 fullPath_  = rootPath_ + "/" + filePath_ ;
 fullHPath_ = ""                          ;
 xmlOut_    = xmlOut                      ;
 level_     = 0                           ;
 debug_     = true                       ;
  
 rootFile_ = new TFile(fullPath_.c_str()) ;

 if( debug_ ) 
 {
  STDLINE(rootPath_,ACYellow) ; 
  STDLINE(filePath_,ACCyan  ) ; 
  STDLINE(fullPath_,ACWhite ) ;
  rootFile_->ls() ;
 }
}

//================================================================================================
xercesc::DOMDocument * RootFileExplorer::initialize(void)
{
 try
 {
  xercesc::XMLPlatformUtils::Initialize();  // Initialize Xerces infrastructure
 }
 catch(xercesc::XMLException& e)
 {
	 std::string msg = xercesc::XMLString::transcode(e.getMessage()) ;
  STDLINE(string("XML toolkit initialization error: ")+msg,ACRed) ;
 }

 theImplementation_ = xercesc::DOMImplementationRegistry::getDOMImplementation(xercesc::XMLString::transcode("Core"));

 if(theImplementation_)
 {
  try
  {
   theDocument_ = theImplementation_->createDocument(
                                                     xercesc::XMLString::transcode("http://www.w3.org/2001/XMLSchema-instance"),  
                                                     xercesc::XMLString::transcode(rootTagName_.c_str()),  
                                                     0
                                                    );                           
  }
  catch(const xercesc::OutOfMemoryException&)
  {
   XERCES_STD_QUALIFIER cerr << "OutOfMemoryException"
                             << XERCES_STD_QUALIFIER endl;
  }
  catch(const xercesc::DOMException& e)
  {
   XERCES_STD_QUALIFIER cerr << "DOMException code is:  " 
                             << e.code 
                             << " " 
                             << xercesc::XMLString::transcode(e.getMessage())
                             << XERCES_STD_QUALIFIER endl;
  }
  catch(const xercesc::XMLException& e)
  {
   STDLINE(std::string("Error Message: ")+xercesc::XMLString::transcode(e.getMessage()),ACRed) ;
  }
  catch(...)
  {
   XERCES_STD_QUALIFIER cerr << "An error occurred creating the theDocument_"
                             << XERCES_STD_QUALIFIER endl;
  }
 }
 else
 {
  XERCES_STD_QUALIFIER cerr << "Requested theImplementation_ is not supported"
                            << XERCES_STD_QUALIFIER endl;
 }

 rootElement_ = theDocument_->getDocumentElement();

 this->makeDirectoryBinaryTree(rootFile_,0,NULL) ; 
 return theDocument_ ;   
}
//==========================================================================================
xercesc::DOMElement * RootFileExplorer::populateBinaryTreeNode(xercesc::DOMElement * anchorNode, 
                                                               std::string           name      ,
                                                               int                   level     ,
                                                               std::string           isLeaf    )
{ 
 xercesc::DOMElement * nodes ;
 if( theNodes_.find(level) == theNodes_.end() ) // a new node
 {
  nodes                            = theDocument_->createElement( xercesc::XMLString::transcode("nodes"          ));
  anchorNode->appendChild(nodes);
  theNodes_[level] = nodes  ;
 }
 else // Is already there
 {
  nodes      = theNodes_[level] ;
 }

 xercesc::DOMElement * node         = theDocument_->createElement( xercesc::XMLString::transcode("node"            ));
 nodes->appendChild(node);

 xercesc::DOMElement * nChilds      = theDocument_->createElement( xercesc::XMLString::transcode("nChilds"         ));
 node->appendChild(nChilds);

 xercesc::DOMText    * nChildsVal   = theDocument_->createTextNode(xercesc::XMLString::transcode("x"               ));
 nChilds->appendChild(nChildsVal);

 xercesc::DOMElement * dirName      = theDocument_->createElement( xercesc::XMLString::transcode("dirName"         ));
 node->appendChild(dirName);

 xercesc::DOMText    * dirNameVal   = theDocument_->createTextNode(xercesc::XMLString::transcode(name.c_str()      ));
 dirName->appendChild(dirNameVal);

 xercesc::DOMElement * fullPath     = theDocument_->createElement( xercesc::XMLString::transcode("fullPath"        ));
 node->appendChild(fullPath);

 xercesc::DOMText    * fullPathVal  = theDocument_->createTextNode(xercesc::XMLString::transcode(fullPath_.c_str() ));
 fullPath->appendChild(fullPathVal);

 xercesc::DOMElement * fullHPath    = theDocument_->createElement( xercesc::XMLString::transcode("fullHPath"       ));
 node->appendChild(fullHPath);

 xercesc::DOMText    * fullHPathVal = theDocument_->createTextNode(xercesc::XMLString::transcode(fullHPath_.c_str()));
 fullHPath->appendChild(fullHPathVal);

 xercesc::DOMElement * leaf         = theDocument_->createElement( xercesc::XMLString::transcode("leaf"            ));
 node->appendChild(leaf);

 xercesc::DOMText    * leafVal      = theDocument_->createTextNode(xercesc::XMLString::transcode(isLeaf.c_str()    ));
 leaf->appendChild(leafVal);
 
 return node;
}
//================================================================================================
void RootFileExplorer::makeDirectoryBinaryTree(TDirectory          * currentDirectory, 
                                               int                   level           ,
                                               xercesc::DOMElement * anchorNode       )
{
 if( !anchorNode) anchorNode = rootElement_ ;

 TKey * keyH = NULL ;
 TIter hList(currentDirectory->GetListOfKeys());
 while((keyH = (TKey*)hList()))
 {
  std::string hName = keyH->GetName() ;  
  string what = keyH->GetClassName() ;
  if( what == "TTree"       ) continue ;
  if( what == "TNtuple"     ) continue ;
  if( what == "TGeoManager" ) continue ;
  if( what == "TGeoVolume" ) continue ;
  if( debug_ ) STDLINE(string("currentDirectory: ")+string(currentDirectory->GetName()),ACCyan) ;
  if( debug_ ) STDLINE(string("Object type     : ")+what,ACRed) ;
  if( keyH->IsFolder() ) 
  {
   currentDirectory->cd(hName.c_str());
   TDirectory * subDir = gDirectory ;
   if( theHierarchy_.find(level) == theHierarchy_.end() ) 
   {
    theHierarchy_[level] = subDir->GetName() ;
   }
   if( debug_ ) STDLINE(fullHPath_,ACBlue) ;
   if( debug_ ) STDLINE(subDir->GetName(),ACCyan) ;
   xercesc::DOMElement * node = this->populateBinaryTreeNode(anchorNode, hName, level, "false" ) ;
   this->makeDirectoryBinaryTree(subDir,level+1,node) ;
  }
  else
  {
   if( debug_ ) STDLINE(hName,"") ;
   fullHPath_ = "" ;
   for(int i=0; i<level; i++)
   {
    fullHPath_ += theHierarchy_[i] + "/" ;
   }
   xercesc::DOMElement * node = this->populateBinaryTreeNode(anchorNode, hName, level, "true"  ) ;
  }
 }
}
