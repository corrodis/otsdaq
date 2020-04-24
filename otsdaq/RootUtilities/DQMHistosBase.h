#ifndef _ots_DQMHistosBase_h_
#define _ots_DQMHistosBase_h_

#include <map>
#include <string>

class TFile;
class TDirectory;
class TObject;

namespace ots
{
class DQMHistosBase
{
  public:
	DQMHistosBase(void);
	virtual ~DQMHistosBase(void);

	virtual void book(void) { ; }
	virtual void fill(std::string& /*buffer*/, std::map<std::string, std::string> /*header*/) { ; }
	virtual void load(std::string /*fileName*/) { ; }

	TObject* get(std::string name);
	TFile*   getFile(void) { return theFile_; }

  protected:
	virtual void save(void);
	virtual void openFile(std::string fileName);
	virtual void closeFile(void);

	TFile*      theFile_;
	TDirectory* myDirectory_;
};
}  // namespace ots

#endif
