#ifndef _ots_DQMHistosOuterTracker_h_
#define _ots_DQMHistosOuterTracker_h_

#include <map>
#include <queue>
#include <string>
#include "otsdaq-core/DataDecoders/DataDecoder.h"
//ROOT documentation
//http://root.cern.ch/root/html/index.html

class TFile;
class TCanvas;
class TH1;
class TH1I;
class TH1F;
class TH2F;
class TProfile;
class TDirectory;
class TObject;

namespace ots
{
class ConfigurationManager;

class DQMHistosOuterTracker
{
  public:
	DQMHistosOuterTracker(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID);
	virtual ~DQMHistosOuterTracker(void);
	void     setConfigurationManager(ConfigurationManager* configurationManager) { theConfigurationManager_ = configurationManager; }
	void     book(void);
	void     fill(std::string& buffer, std::map<std::string, std::string> header);
	void     save(void);
	void     load(std::string fileName);
	TObject* get(std::string name);

	TFile* getFile() { return theFile_; }  //added by RAR

	//Getters
	//TCanvas*  getCanvas (void){return canvas_;}
	//TH1F*     getHisto1D(void){return histo1D_;}
	//TH2F*     getHisto2D(void){return histo2D_;}
	//TProfile* getProfile(void){return profile_;}

  protected:
	void   openFile(std::string fileName);
	void   closeFile(void);
	TFile* theFile_;

	DataDecoder          theDataDecoder_;
	std::queue<uint32_t> convertedBuffer_;

	//TCanvas*      canvas_; // main canvas
	//TH1F*         histo1D_;// 1-D histogram
	//TH2F*         histo2D_;// 2-D histogram
	//TProfile*     profile_;// profile histogram
	//       IPAddress          port                channel
	std::map<std::string, std::map<std::string, std::map<unsigned int, TH1*>>> planeOccupancies_;
	//std::vector<TH1I*> planeOccupancies_;
	TH1I*                 numberOfTriggers_;
	const std::string     supervisorContextUID_;
	const std::string     supervisorApplicationUID_;
	const std::string     bufferUID_;
	const std::string     processorUID_;
	TDirectory*           currentDirectory_;
	ConfigurationManager* theConfigurationManager_;
};
}  // namespace ots

#endif
