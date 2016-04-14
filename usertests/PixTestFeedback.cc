#include <stdlib.h>     
#include <algorithm>    
#include <iostream>
#include <fstream>

#include <TH1.h>
#include <TRandom.h>
#include <TStopwatch.h>
#include <TStyle.h>

#include "PixTestFeedback.hh"
#include "PixUtil.hh"
#include "log.h"
#include "helper.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestFeedback)

/*

-- Feedback
vcal                200
step                5

*/

// ----------------------------------------------------------------------
PixTestFeedback::PixTestFeedback(PixSetup *a, std::string name) : PixTest(a, name), fParVcal(35), fParStep(5) {
  PixTest::init();
  init(); 
  //  LOG(logINFO) << "PixTestFeedback ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestFeedback::PixTestFeedback() : PixTest() {
  //  LOG(logINFO) << "PixTestFeedback ctor()";
}

// ----------------------------------------------------------------------
bool PixTestFeedback::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      if (!parName.compare("step")) {
	fParStep = atoi(sval.c_str()); 
      }
      if (!parName.compare("vcal")) {
	fParVcal = atoi(sval.c_str()); 
      }
      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestFeedback::init() {
  setToolTips(); 
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestFeedback::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("effvsvdig")) {
    efficiecyVsVdig();
    return;
  }
  if (!command.compare("effvsfb")) {
    efficiencyVsFeedback();
    return;
  }
  if (!command.compare("setvdigvwllsh")) {
    setVdigVwllsh();
    return;
  }

  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestFeedback::setToolTips() {
  fTestTip    = string("")
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;
}

// ----------------------------------------------------------------------
void PixTestFeedback::bookHist(string name) {
  fDirectory->cd(); 

  LOG(logDEBUG) << "nothing done with " << name;
}


//----------------------------------------------------------
PixTestFeedback::~PixTestFeedback() {
  LOG(logDEBUG) << "PixTestFeedback dtor";
}

void PixTestFeedback::efficiecyVsVdig() {

  cacheDacs();
  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  maskPixels();

  // -- pattern generator setup without resets
  resetROC();
  fPg_setup.clear();
  vector<pair<string, uint8_t> > pgtmp = fPixSetup->getConfigParameters()->getTbPgSettings();
  for (unsigned i = 0; i < pgtmp.size(); ++i) {
    if (string::npos != pgtmp[i].first.find("resetroc")) continue;
    if (string::npos != pgtmp[i].first.find("resettbm")) continue;
    fPg_setup.push_back(pgtmp[i]);
  }
  if (0) for (unsigned int i = 0; i < fPg_setup.size(); ++i) cout << fPg_setup[i].first << ": " << (int)fPg_setup[i].second << endl;

  fApi->setPatternGenerator(fPg_setup);

  int iStep = fParStep;
  int nRocs = fApi->_dut->getNEnabledRocs();

  TH1D* hEff[32];
  TH1D* hX[32];

  for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
    hEff[iRocIdx] = bookTH1D(Form("EfficiencyVsVdig_C%d", getIdFromIdx(iRocIdx)),  Form("EfficiencyVsVdig_C%d", getIdFromIdx(iRocIdx)),  256, 0, 256.0);
    hX[iRocIdx] = bookTH1D(Form("XHitsVsVdig_C%d", getIdFromIdx(iRocIdx)),  Form("XHitsVsVdig_C%d", getIdFromIdx(iRocIdx)),  256, 0, 256.0);
  }

  for (int i=0;i<=15;i+=1) {

    fApi->setDAC("vdig", i);

    vector< std::pair<double, int> > effXhits = getEfficiency();

    for (int j=0; j< effXhits.size(); j++) {
      hEff[j]->SetBinContent(i, effXhits[j].first);
      hX[j]->SetBinContent(i, effXhits[j].second);
    }

  }

  for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
    fHistList.push_back(hEff[iRocIdx]);
    fHistList.push_back(hX[iRocIdx]);
  }


  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw((getHistOption(*il)).c_str()); 
    fDisplayedHist = (il);
  }
  restoreDacs();
  PixTest::update(); 
  dutCalibrateOff();

}

void PixTestFeedback::setVdigVwllsh() {

  cacheDacs();
  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  maskPixels();

  // -- pattern generator setup without resets
  resetROC();
  fPg_setup.clear();
  vector<pair<string, uint8_t> > pgtmp = fPixSetup->getConfigParameters()->getTbPgSettings();
  for (unsigned i = 0; i < pgtmp.size(); ++i) {
    if (string::npos != pgtmp[i].first.find("resetroc")) continue;
    if (string::npos != pgtmp[i].first.find("resettbm")) continue;
    fPg_setup.push_back(pgtmp[i]);
  }
  if (0) for (unsigned int i = 0; i < fPg_setup.size(); ++i) cout << fPg_setup[i].first << ": " << (int)fPg_setup[i].second << endl;

  fApi->setPatternGenerator(fPg_setup);

  int iStep = fParStep;
  int nRocs = fApi->_dut->getNEnabledRocs();

  vector<double> max_efficiency(nRocs, 0.0);
  vector<int> max_efficiency_feedback(nRocs, 150);
  vector<int> max_efficiency_vdig(nRocs, 10);

  vector<int> test_feedbacks;
  test_feedbacks.push_back(5);
  test_feedbacks.push_back(10);
  test_feedbacks.push_back(20);
  test_feedbacks.push_back(50);
  test_feedbacks.push_back(100);
  test_feedbacks.push_back(150);

  for (int i=3;i<=15;i+=1) {

    for (int j=0;j<test_feedbacks.size();j++) {

      fApi->setDAC("vdig", i);

      fApi->setDAC("vwllpr", test_feedbacks[j]);
      fApi->setDAC("vwllsh", test_feedbacks[j]);

      vector< std::pair<double, int> > effXhits = getEfficiency(10);

      for (int k=0; k< effXhits.size(); k++) {
        if (effXhits[k].first >= max_efficiency[k]) {
          max_efficiency[k] = effXhits[k].first;
          max_efficiency_feedback[k] = test_feedbacks[j];
          max_efficiency_vdig[k] = i;
        }

      }

    }

  }

  restoreDacs();

  for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
    LOG(logINFO) << "Settings found for ROC" << getIdFromIdx(iRocIdx) << ": vdig = " << max_efficiency_vdig[iRocIdx] << ", vwllpr = vwllsh = " << max_efficiency_feedback[iRocIdx];
  
    fApi->setDAC("vdig", max_efficiency_vdig[iRocIdx], getIdFromIdx(iRocIdx));
    fApi->setDAC("vwllpr", max_efficiency_feedback[iRocIdx], getIdFromIdx(iRocIdx));
    fApi->setDAC("vwllsh", max_efficiency_feedback[iRocIdx], getIdFromIdx(iRocIdx));
  }

  PixTest::update(); 
  dutCalibrateOff();

}

// ----------------------------------------------------------------------
void PixTestFeedback::doTest() {

  TStopwatch t;

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestFeedback::doTest()"));


  efficiencyVsFeedback();

  int seconds = t.RealTime(); 
  LOG(logINFO) << "PixTestFeedback::doTest() done, duration: " << seconds << " seconds";
}

vector< std::pair<double, int> > PixTestFeedback::getEfficiency(int nTrig) {

  vector< std::pair<double, int> > result;

  vector<TH2D*> test2 = efficiencyMaps("highRate", nTrig, FLAG_CHECK_ORDER | FLAG_FORCE_UNMASKED);
  vector<TH2D*> test3 = getXrayMaps();
  vector<int> deadPixel(test2.size(), 0);
  vector<int> probPixel(test2.size(), 0);
  vector<int> xHits(test3.size(),0);
  vector<int> fidHits(test2.size(),0);
  vector<int> allHits(test2.size(),0);
  vector<int> fidPixels(test2.size(),0);
  TH1D *h1(0), *h2(0); 
  vector<int> nCalHits(test2.size(),0);

  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
    fHistOptions.insert(make_pair(test3[i], "colz"));
    h1 = bookTH1D(Form("HR_Overall_Efficiency_C%d", getIdFromIdx(i)),  Form("HR_Overall_Efficiency_C%d", getIdFromIdx(i)),  201, 0., 1.005);
    fHistList.push_back(h1); 
    h2 = bookTH1D(Form("HR_Fiducial_Efficiency_C%d", getIdFromIdx(i)),  Form("HR_Fiducial_Efficiency_C%d", getIdFromIdx(i)),  201, 0., 1.005);
    fHistList.push_back(h2); 
    
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
        allHits[i] += static_cast<int>(test2[i]->GetBinContent(ix+1, iy+1));
  h1->Fill(test2[i]->GetBinContent(ix+1, iy+1)/nTrig);
  if ((ix > 0) && (ix < 51) && (iy < 79) && (test2[i]->GetBinContent(ix+1, iy+1) > 0)) {
    fidHits[i] += static_cast<int>(test2[i]->GetBinContent(ix+1, iy+1));
    ++fidPixels[i];
    h2->Fill(test2[i]->GetBinContent(ix+1, iy+1)/nTrig);
  }
  // -- count dead pixels
  if (test2[i]->GetBinContent(ix+1, iy+1) < nTrig) {
    ++probPixel[i];
    if (test2[i]->GetBinContent(ix+1, iy+1) < 1) {
      ++deadPixel[i];
    }
  }
  nCalHits[i] += test2[i]->GetBinContent(ix+1, iy+1);
  // -- Count X-ray hits detected
  if (test3[i]->GetBinContent(ix+1,iy+1)>0){
    xHits[i] += static_cast<int> (test3[i]->GetBinContent(ix+1,iy+1));
  }
      }
    }

    std::pair<double, int> rocResult;
    rocResult.first = (double)nCalHits[i]/(4160.0 * nTrig);
    rocResult.second = xHits[i];
    result.push_back(rocResult);
  }
  return result;
}

void PixTestFeedback::efficiencyVsFeedback() {

  cacheDacs();
  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  maskPixels();

  // -- pattern generator setup without resets
  resetROC();
  fPg_setup.clear();
  vector<pair<string, uint8_t> > pgtmp = fPixSetup->getConfigParameters()->getTbPgSettings();
  for (unsigned i = 0; i < pgtmp.size(); ++i) {
    if (string::npos != pgtmp[i].first.find("resetroc")) continue;
    if (string::npos != pgtmp[i].first.find("resettbm")) continue;
    fPg_setup.push_back(pgtmp[i]);
  }
  if (0) for (unsigned int i = 0; i < fPg_setup.size(); ++i) cout << fPg_setup[i].first << ": " << (int)fPg_setup[i].second << endl;

  fApi->setPatternGenerator(fPg_setup);

  int iStep = fParStep;
  int nRocs = fApi->_dut->getNEnabledRocs();

  TH1D* hEff[32];
  TH1D* hX[32];

  for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
    hEff[iRocIdx] = bookTH1D(Form("Efficiency_C%d", getIdFromIdx(iRocIdx)),  Form("Efficiency_C%d", getIdFromIdx(iRocIdx)),  256, 0, 256.0);
    hX[iRocIdx] = bookTH1D(Form("Noise_Hits_C%d", getIdFromIdx(iRocIdx)),  Form("Noise_Hits_C%d", getIdFromIdx(iRocIdx)),  256, 0, 256.0);
  }

  for (int i=0;i<=255;i+=iStep) {

    fApi->setDAC("vwllpr", i);
    fApi->setDAC("vwllsh", i);

    vector< std::pair<double, int> > effXhits = getEfficiency();

    for (int j=0; j< effXhits.size(); j++) {
      hEff[j]->SetBinContent(i, effXhits[j].first);
      hX[j]->SetBinContent(i, effXhits[j].second);
    }

  }

  for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
    fHistList.push_back(hEff[iRocIdx]);
    fHistList.push_back(hX[iRocIdx]);
  }


  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw((getHistOption(*il)).c_str()); 
    fDisplayedHist = (il);
  }
  restoreDacs();
  PixTest::update(); 
  dutCalibrateOff();

}