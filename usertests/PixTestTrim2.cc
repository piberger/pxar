#include <stdlib.h>     
#include <algorithm>    
#include <iostream>
#include <fstream>

#include <TH1.h>
#include <TRandom.h>
#include <TStopwatch.h>
#include <TStyle.h>

#include "PixTestTrim2.hh"
#include "PixUtil.hh"
#include "log.h"
#include "helper.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestTrim2)

/*

-- Trim2
init                button
vcal                35
trim                button
vthrcomp            button
ntrig               50
nsteps              12
step                button
save                button
sethigh             button
setmiddle           button
setlow              button

*/

// ----------------------------------------------------------------------
PixTestTrim2::PixTestTrim2(PixSetup *a, std::string name) : PixTest(a, name), fParVcal(35), fParNtrig(1) {
  PixTest::init();
  init(); 
  //  LOG(logINFO) << "PixTestTrim2 ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestTrim2::PixTestTrim2() : PixTest() {
  //  LOG(logINFO) << "PixTestTrim2 ctor()";
}

// ----------------------------------------------------------------------
bool PixTestTrim2::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      if (!parName.compare("ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
      }
      if (!parName.compare("nsteps")) {
  fParNSteps = atoi(sval.c_str()); 
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
void PixTestTrim2::init() {
  setToolTips(); 
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestTrim2::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("trim")) {
    trimTest(); 
    return;
  }  
  if (!command.compare("step")) {
    retrimStep(); 
    return;
  }
  if (!command.compare("final")) {
    final(); 
    return;
  }
  if (!command.compare("save")) {
    saveTrimBits(); 
    return;
  }
  if (!command.compare("sethigh")) {
    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);
    maskPixels();
    setTrimBits(15);
    return;
  }
  if (!command.compare("setlow")) {
    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);
    maskPixels();
    setTrimBits(0);
    return;
  }
  if (!command.compare("setmiddle")) {
    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);
    maskPixels();
    setTrimBits(7);
    return;
  }
  if (!command.compare("init")) {
    fPixSetup->getConfigParameters()->setTrimVcalSuffix(Form("%d", fParVcal), true); 

    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);
    maskPixels();
    setTrimBits();
    PixTest::update(); 
    setTrimBits(7);
    LOG(logINFO) << "initialized!";
    return;
  }
  if (!command.compare("vthrcomp")) {
    findVthrcomp();
    return;
  }

  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestTrim2::setToolTips() {
  fTestTip    = string(Form("trimming results in a uniform in-time threshold\n")
		       + string("TO BE FINISHED!!"))
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;
}

// ----------------------------------------------------------------------
void PixTestTrim2::bookHist(string name) {
  fDirectory->cd(); 

  LOG(logDEBUG) << "nothing done with " << name;
}


//----------------------------------------------------------
PixTestTrim2::~PixTestTrim2() {
  LOG(logDEBUG) << "PixTestTrim2 dtor";
}


// ----------------------------------------------------------------------
void PixTestTrim2::doTest() {

  TStopwatch t;

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestTrim2::doTest()"));

  fProblem = false; 
  trimTest(); 
  if (fProblem) {
    LOG(logINFO) << "PixTestTrim2::doTest() aborted because of problem ";
    return;
  }
  /*
  TH1 *h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); */

  int seconds = t.RealTime(); 
  LOG(logINFO) << "PixTestTrim2::doTest() done, duration: " << seconds << " seconds";
}


void PixTestTrim2::getCaldelVthrcomp() {

  uint16_t FLAGS = FLAG_FORCE_MASKED;
  int ntrig = 10;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  int nRocs = rocIds.size();

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  results;
  fApi->_dut->testPixel(11, 22, true);
  fApi->_dut->maskPixel(11, 22, false);

  results = fApi->getEfficiencyVsDACDAC("caldel", 0, 255, "vthrcomp", 0, 255, FLAGS, ntrig);

  vector< pair<int,int> > CaldelRangesROC(256, make_pair(256, -1));
  vector< vector< pair<int,int> > >  CaldelRanges(nRocs, CaldelRangesROC);

  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int i = 0; i < results.size(); ++i) {
      pair<uint8_t, pair<uint8_t, vector<pixel> > > v = results[i];
      int idac1 = v.first; 
      pair<uint8_t, vector<pixel> > w = v.second;      
      int idac2 = w.first;
      vector<pixel> wpix = w.second;

      for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
        if (wpix[ipix].roc() == rocIds[iroc]) {
          if (wpix[ipix].value() == ntrig) {
            if (idac1 < CaldelRanges[iroc][idac2].first) {
              CaldelRanges[iroc][idac2].first = idac1;
            }
            if (idac1 > CaldelRanges[iroc][idac2].second) {
              CaldelRanges[iroc][idac2].second = idac1;
            }
          }
        }
      }

    }
  }

  fCaldelVthrcompLUT.clear();
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int vthrcomp = 0; vthrcomp < 256; ++vthrcomp){
      int rMin = CaldelRanges[iroc][vthrcomp].first;
      int rMax = CaldelRanges[iroc][vthrcomp].second;

      if (rMin < rMax) {
        LOG(logDEBUG) << "ROC"<< (int)iroc << " Vthrcomp = " << vthrcomp << ": caldel from "<< rMin << " to " << rMax;
        int middlePoint = (rMax+rMin)/2;
        fCaldelVthrcompLUT.push_back(middlePoint);
      } else {
        LOG(logDEBUG) << "ROC"<< (int)iroc << " Vthrcomp = " << vthrcomp << "---";
        fCaldelVthrcompLUT.push_back(-1);
      }
    }
  }


}

void PixTestTrim2::findVthrcomp() {

  getCaldelVthrcomp();

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();
  setTrimBits(15);

  int vcalBelow = fParVcal-10;
  if (vcalBelow < 1) vcalBelow = 1;

  fApi->setDAC("vcal", vcalBelow);
  fApi->setDAC("vtrim", 0);

  int vthrcomp = 250;
  float eff0=0;

  while (eff0 < 0.01 && vthrcomp > 0) {
    vthrcomp -= 10;
    LOG(logINFO) << "testing vtrhcomp = " << vthrcomp;
    setVthrcomp(vthrcomp);
    eff0 = getEfficiency();
  }
  LOG(logINFO) << "noise level found around vtrhcomp = " << vthrcomp;

  while (getEfficiency() > 0.0005 && vthrcomp > 0) {
    vthrcomp -= 10;
    LOG(logINFO) << "testing vtrhcomp = " << vthrcomp;
    setVthrcomp(vthrcomp);
  }
  vthrcomp += 15;
  setVthrcomp(vthrcomp);

  while (getEfficiency() > 0.0005 && vthrcomp > 0) {
    vthrcomp -= 1;
    LOG(logINFO) << "testing vtrhcomp = " << vthrcomp;
    setVthrcomp(vthrcomp);
  }

  fApi->setDAC("vtrim", 60);
  LOG(logINFO) << "final vtrhcomp = " << vthrcomp;
}


void PixTestTrim2::getVtrimVthrcomp() {

  uint16_t FLAGS = FLAG_FORCE_MASKED;
  int ntrig = 8;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  int nRocs = rocIds.size();

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  results;

  for (int ix=0;ix<52;ix+=15) {
    for (int iy=0;iy<80;iy+=23) {
      fApi->_dut->testPixel(ix, iy, true);
      fApi->_dut->maskPixel(ix, iy, false);
    }
  }

  results = fApi->getEfficiencyVsDACDAC("vtrim", 0, 255, "vthrcomp", 0, 255, FLAGS, ntrig);
  TH2D* effMap = bookTH2D(Form("effMapVtrimVthrcomp_C0"), Form("effMapVtrimVthrcomp_C0"), 256, 0, 256.0, 256, 0, 256.0); 

  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int i = 0; i < results.size(); ++i) {
      pair<uint8_t, pair<uint8_t, vector<pixel> > > v = results[i];
      int idac1 = v.first; 
      pair<uint8_t, vector<pixel> > w = v.second;      
      int idac2 = w.first;
      vector<pixel> wpix = w.second;

      for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
        if (wpix[ipix].roc() == rocIds[iroc]) {
          effMap->Fill(idac1, idac2, ntrig*0.5-fabs(wpix[ipix].value()-ntrig*0.5));
        }
      }

    }
  }
  effMap->Draw("colz");
  fHistList.push_back(effMap);
  fHistOptions.insert(make_pair(effMap, "colz")); 
  PixTest::update(); 
}

//
int PixTestTrim2::getNumberOfDeadPixels() {
  int ntrig = 10;

  vector<TH2D*> test2 = efficiencyMaps("PixelAlive", ntrig, FLAG_FORCE_MASKED); 
  vector<int> deadPixel(test2.size(), 0); 
  vector<int> probPixel(test2.size(), 0); 
  for (unsigned int i = 0; i < test2.size(); ++i) {
    // -- count dead pixels
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
  if (test2[i]->GetBinContent(ix+1, iy+1) < ntrig) {
    ++probPixel[i];

    if (test2[i]->GetBinContent(ix+1, iy+1) < 1) {
      ++deadPixel[i];
    }
  }
      }
    }
  }

  return probPixel[0];
}


double PixTestTrim2::getEfficiency() {
  int ntrig = 10;

  vector<TH2D*> test2 = efficiencyMaps("PixelAlive", ntrig, FLAG_FORCE_MASKED); 
  vector<int> deadPixel(test2.size(), 0); 
  vector<int> probPixel(test2.size(), 0); 
  int nHits = 0;
  for (unsigned int i = 0; i < test2.size(); ++i) {
    if (i==0) {
      // -- count dead pixels
      for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
        for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
          nHits += test2[i]->GetBinContent(ix+1, iy+1);
        }
      }
    }
  }

  return (double)nHits/(4160.0 * ntrig);
}


void PixTestTrim2::setVthrcomp(int vthrcomp) {
  fApi->setDAC("vthrcomp", vthrcomp);
  if (fCaldelVthrcompLUT[vthrcomp] > 0) {
    fApi->setDAC("caldel", fCaldelVthrcompLUT[vthrcomp]);
     LOG(logDEBUG) << "set Vthrcomp to " << vthrcomp << " and Caldel to " << fCaldelVthrcompLUT[vthrcomp];
  } else {
     LOG(logDEBUG) << "set Vthrcomp to " << vthrcomp;
  }
}

// ----------------------------------------------------------------------
void PixTestTrim2::retrimStep() {
  int ntrig = fParNtrig;

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  if (fParNSteps < 1) fParNSteps = 1;
  if (fParNSteps > 1000) fParNSteps = 1;

  int nSteps = fParNSteps;
  bool parChange = false;
  int nRequestLargerVtrim = 0;

  std::vector< int > best50PercentDifferences(4160, ntrig);
  std::vector< int > best50PercentDifferenceTrimbits(4160, 7);

  for (int j=1;j<=nSteps;j++) {

    PixTest::update(); 

    fApi->setDAC("vcal", fParVcal);
    vector<TH2D*> atThreshold = efficiencyMaps("PixelAlive", ntrig, FLAG_FORCE_MASKED); 

    fApi->setDAC("vcal", fParVcal + 5);
    vector<TH2D*> aboveThreshold = efficiencyMaps("PixelAlive", ntrig, FLAG_FORCE_MASKED); 

    fApi->setDAC("vcal", fParVcal - 5);
    vector<TH2D*> belowThreshold = efficiencyMaps("PixelAlive", ntrig, FLAG_FORCE_MASKED); 

    fApi->setDAC("vcal", 200);
    vector<TH2D*> largeSignal = efficiencyMaps("PixelAlive", ntrig, FLAG_FORCE_MASKED); 

    float fAtThreshold = 0;
    float fAboveThreshold = 0;
    float fBelowThreshold = 0;
    float fLargeSignal = 0;
    int nActions = 0;

    TH2D* trimbitMap = bookTH2D(Form("trimbitMap_C0"), Form("trimbitMap_C0"), 52, 0., static_cast<double>(52), 80, 0., static_cast<double>(80)); 

    for (unsigned int i = 0; i < atThreshold.size(); ++i) {
      if (i==0) {
        nRequestLargerVtrim = 0;
        parChange=false;
        for (int ix = 0; ix < atThreshold[i]->GetNbinsX(); ++ix) {
          for (int iy = 0; iy < atThreshold[i]->GetNbinsY(); ++iy) {

            fAtThreshold = atThreshold[i]->GetBinContent(ix+1, iy+1) / (float)ntrig;
            fAboveThreshold = aboveThreshold[i]->GetBinContent(ix+1, iy+1) / (float)ntrig;
            fBelowThreshold = belowThreshold[i]->GetBinContent(ix+1, iy+1) / (float)ntrig;
            fLargeSignal= largeSignal[i]->GetBinContent(ix+1, iy+1) / (float)ntrig;

            if (fAboveThreshold < 0.1 && fBelowThreshold < 0.1 && fAtThreshold < 0.1) {
                if (fLargeSignal > 0.9) {
                  if (fTrimBits[0][ix][iy] > 0) {
                    fTrimBits[0][ix][iy]--;
                    nActions++;
                  }
                } else {
                  if (fTrimBits[0][ix][iy] < 15) {
                    fTrimBits[0][ix][iy] += 1;
                    nActions++;
                  }
                }
            } else if (fLargeSignal < 0.5) {
                if (fTrimBits[0][ix][iy] < 15) {
                  fTrimBits[0][ix][iy] += 1;
                  nActions++;
                }
            } else if (fBelowThreshold - fAboveThreshold > 0.1) {
                if (fTrimBits[0][ix][iy] < 15) {
                  fTrimBits[0][ix][iy] += 1;
                  nActions++;
                }
            } else if (fBelowThreshold > 0.5) {
                if (fTrimBits[0][ix][iy] < 15) {
                  fTrimBits[0][ix][iy] += 1;
                  nActions++;
                }
            } else {
              int difference50 = abs(atThreshold[i]->GetBinContent(ix+1, iy+1) - ntrig/2);
              if (difference50 < best50PercentDifferences[ix*80+iy]) {
                best50PercentDifferences[ix*80+iy] = difference50 ;
                best50PercentDifferenceTrimbits[ix*80+iy] = fTrimBits[0][ix][iy];
              } else {
                fTrimBits[0][ix][iy] = best50PercentDifferenceTrimbits[ix*80+iy];
              }
              // normal cases
              if (fBelowThreshold < fAtThreshold && fAtThreshold < fAboveThreshold ) {
                    if (fAtThreshold > 0.7) {
                      // increase threshold
                      if (fTrimBits[0][ix][iy] < 15) {
                        fTrimBits[0][ix][iy]++;
                        nActions++;
                      }
                    } else if (fAtThreshold < 0.2) {
                      // decrease threshold
                      if (fTrimBits[0][ix][iy] > 0) {
                        fTrimBits[0][ix][iy]--;
                        nActions++;
                      } else {
                        nRequestLargerVtrim++;
                      }
                    }
                }

            }


            if (fTrimBits[0][ix][iy] > 15) fTrimBits[0][ix][iy] = 15;
            if (fTrimBits[0][ix][iy] < 1)  fTrimBits[0][ix][iy] = 0;
            trimbitMap->Fill(ix,iy,fTrimBits[0][ix][iy]);

          }
        }

      }
    }

    int vtrim = fApi->_dut->getDAC(0, "vtrim");
    if (nRequestLargerVtrim > 5 && vtrim < 250) {
      if (nRequestLargerVtrim > 100) {
        vtrim += 24;
      } else if (nRequestLargerVtrim > 50) {
        vtrim += 8;
      } else {
        vtrim += 4;
      }
      if (vtrim > 255) vtrim = 255;
      LOG(logINFO) << nRequestLargerVtrim << " pixels request a larger vtrim, going to " << vtrim;
      for (int i=0;i<4160;i++) {
        best50PercentDifferences[i] = ntrig;
      }
      fApi->setDAC("vtrim", vtrim);
      nSteps++;
    }

    setTrimBits();

    trimbitMap->Draw("colz");
    fHistList.push_back(trimbitMap);
    fHistOptions.insert(make_pair(trimbitMap, "colz")); 

    PixTest::update(); 

    LOG(logINFO) << "retrimStep()" << j << " of " << fParNSteps << " => nActions = " << nActions;
  }

  TH2D* trimbitMap = bookTH2D(Form("trimbitMap_Final_C0"), Form("trimbitMap_Final_C0"), 52, 0., static_cast<double>(52), 80, 0., static_cast<double>(80)); 
  TH1D* trimbitMapDstr = bookTH1D(Form("trimbitMap_Final_dstr_C0"), Form("trimbitMap_Final_dstr_C0"), 16, 0., 16); 
  for (int ix = 0; ix < 52; ++ix) {
    for (int iy = 0; iy < 80; ++iy) {
      fTrimBits[0][ix][iy] = best50PercentDifferenceTrimbits[ix*80+iy];
      trimbitMap->Fill(ix,iy,fTrimBits[0][ix][iy]);
      trimbitMapDstr->Fill(fTrimBits[0][ix][iy]);
    }
  }

  trimbitMap->Draw("colz");
  fHistList.push_back(trimbitMap);
  fHistList.push_back(trimbitMapDstr);
  fHistOptions.insert(make_pair(trimbitMap, "colz")); 
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), trimbitMapDstr);
  trimbitMapDstr->Draw();

  PixTest::update(); 

  fPixSetup->getConfigParameters()->setTrimVcalSuffix(Form("%d", fParVcal), true);

}

void PixTestTrim2::final() {

  int vcalMin = fParVcal-30;
  int vcalMax = fParVcal+30;

  if (vcalMin < 0) vcalMin = 0;
  if (vcalMax > 255) vcalMax = 255;

  vector<TH1*> thrF = scurveMaps("vcal", "TrimThrFinal", 20, vcalMin, vcalMax, -1, -1, 9); 
  PixTest::update(); 

  fPixSetup->getConfigParameters()->setTrimVcalSuffix(Form("%d", fParVcal), true); 
  saveDacs();
  saveTrimBits();

  string trimMeanString, trimRmsString; 
  string hname;
  for (unsigned int i = 0; i < thrF.size(); ++i) {
    hname = thrF[i]->GetName();
    // -- skip sig_ and thn_ histograms
    if (string::npos == hname.find("dist_thr_")) continue;
    trimMeanString += Form("%6.2f ", thrF[i]->GetMean()); 
    trimRmsString += Form("%6.2f ", thrF[i]->GetRMS()); 
  }

  // -- summary printout
  LOG(logINFO) << "vcal mean: " << trimMeanString; 
  LOG(logINFO) << "vcal RMS:  " << trimRmsString; 
  LOG(logINFO) << "PixTestTrim2::trimTest() done";

  PixTest::update(); 

}


// ----------------------------------------------------------------------
void PixTestTrim2::trimTest() {

  gStyle->SetPalette(1);
  bool verbose(false);
  cacheDacs(verbose);
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestTrim2::trimTest() ntrig = %d, vcal = %d", fParNtrig, fParVcal));

  //vector<TH1*> thrI = scurveMaps("vcal", "TrimThr0", 10, 0, 120, -1, -1, 9); 
  PixTest::update(); 

  findVthrcomp();

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();
  setTrimBits(7);

  retrimStep();
  final();
}


// ----------------------------------------------------------------------
void PixTestTrim2::setTrimBits(int itrim) {
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int ir = 0; ir < rocIds.size(); ++ir){
    vector<pixelConfig> pix = fApi->_dut->getEnabledPixels(rocIds[ir]);
    for (unsigned int ipix = 0; ipix < pix.size(); ++ipix) {
      if (itrim > -1) {
	fTrimBits[ir][pix[ipix].column()][pix[ipix].row()] = itrim;
      }
      fApi->_dut->updateTrimBits(pix[ipix].column(), pix[ipix].row(), fTrimBits[ir][pix[ipix].column()][pix[ipix].row()], rocIds[ir]);
    }
  }
}

