#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <iostream>  // cout
#include <fstream>
#include <iomanip>   // setw

#include <TH1.h>
#include <TRandom.h>
#include <TStopwatch.h>
#include <TStyle.h>

#include "PixTestOnShellQuickTest.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestOnShellQuickTest)

// testParameters.dat
/*
-- OnShellQuickTest
ntrig               10
bbvthrcomp          120
hvtest              button
bbtest              button
*/

// constructors
//------------------------------------------------------------------------------
PixTestOnShellQuickTest::PixTestOnShellQuickTest( PixSetup *a, std::string name) :
PixTest(a, name), fParNtrig(-1), fBBVthrcomp(-1), fParVcalS(250)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestOnShellQuickTest::PixTestOnShellQuickTest() : PixTest() {
  //  LOG(logINFO) << "PixTestOnShellQuickTest ctor()";
}


// destructor
//----------------------------------------------------------
PixTestOnShellQuickTest::~PixTestOnShellQuickTest() {
  LOG(logDEBUG) << "PixTestOnShellQuickTest dtor";
}

// set parameters
// ----------------------------------------------------------------------
bool PixTestOnShellQuickTest::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
      if (!parName.compare("bbvthrcomp")) {
        fBBVthrcomp = atoi(sval.c_str());
      }
      if (!parName.compare("ntrig")) {
        fParNtrig = atoi(sval.c_str());
      }

      break;
    }
  }

  return found;
}


// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("bbtest")) {
    bbQuickTest();
    return;
  }
  if (!command.compare("hvtest")) {
    hvQuickTest();
    return;
  }

  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::init() {
  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str());
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str());
  }
  fDirectory->cd();
}



// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::setToolTips() {
  fTestTip    = string("");
  fSummaryTip = string("summary plot to be implemented");
}

// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::bookHist(string name) {
  fDirectory->cd();
  LOG(logDEBUG) << "nothing done with " << name;
}


// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::doTest() {

  TStopwatch t;

  fDirectory->cd();
  PixTest::update();
  bigBanner(Form("PixTestOnShellQuickTest::doTest()"));

  hvQuickTest();
  bbQuickTest();

  int seconds = t.RealTime();
  LOG(logINFO) << "PixTestOnShellQuickTest::doTest() done, duration: " << seconds << " seconds";
}


std::vector<int> PixTestOnShellQuickTest::readParametersFile(std::string fileName) {
  std::vector<int> values;
  std::string parametersFileName = fPixSetup->getConfigParameters()->getDirectory() + "/" + fileName;
  std::ifstream parametersFile(parametersFileName, std::ifstream::in);
  std::string line;
  while(std::getline(parametersFile, line)) {
    int par = stoi(line);
    values.push_back(par);
  }
  return values;
}


std::vector<int> PixTestOnShellQuickTest::readBBvthrcomp() {
  return readParametersFile("bumpBondingVthrcomp.dat");
}


std::vector<int> PixTestOnShellQuickTest::readDbVana() {
  return readParametersFile("dbPretestVana.dat");
}


std::vector<int> PixTestOnShellQuickTest::readDbCaldel() {
  return readParametersFile("dbPretestCaldel.dat");
}


// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::bbQuickTest() {

  cacheDacs();

  banner(Form("PixTestOnShellQuickTest::bbQuickTest()"));
  if (fBBVthrcomp < 0) {
    LOG(logWARNING) << "no Vthrcomp cut defined, using default value!";
    fBBVthrcomp = 120;
  }
  int nTrig = fParNtrig;
  if (nTrig < 2) {
    nTrig = 2;
  }
  LOG(logINFO) << "using Ntrig = " << nTrig << ", Vthrcomp = " << fBBVthrcomp;


  // test if pixels alive
  fApi->setDAC("ctrlreg", 0);
  fApi->setDAC("vcal", 200);
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  vector<TH2D*> aliveMaps = efficiencyMaps("PixelAlive", 10, FLAG_FORCE_MASKED);

  // bump bonding map
  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fParVcalS);


  std::vector<int> dbVana = readDbVana();
  if (dbVana.size() >= aliveMaps.size()) {
    LOG(logINFO) << "using Vana from file: ";
    std::stringstream ss;
    ss << "Vana:";
    for (unsigned int idxRoc = 0; idxRoc < dbVana.size(); idxRoc++) {
      ss << dbVana[idxRoc] << "  ";
    }
    LOG(logINFO) << ss.str();
    for (unsigned int idxRoc = 0; idxRoc < dbVana.size(); idxRoc++) {
      fApi->setDAC("vana", dbVana[getIdFromIdx(idxRoc)], getIdFromIdx(idxRoc));
    }
  }

  std::vector<int> dbCaldel = readDbCaldel();
  if (dbCaldel.size() >= aliveMaps.size()) {
    LOG(logINFO) << "using Caldel from file: ";
    std::stringstream ss;
    ss << "Caldel:";
    for (unsigned int idxRoc = 0; idxRoc < dbCaldel.size(); idxRoc++) {
      ss << dbCaldel[idxRoc] << "  ";
    }
    LOG(logINFO) << ss.str();
    for (unsigned int idxRoc = 0; idxRoc < dbCaldel.size(); idxRoc++) {
      fApi->setDAC("caldel", dbCaldel[getIdFromIdx(idxRoc)], getIdFromIdx(idxRoc));
    }
  }

  std::vector<int> bbVthrcompsRoc = readBBvthrcomp();
  if (bbVthrcompsRoc.size() >= aliveMaps.size()) {
    LOG(logINFO) << "using Vthrcomps from file: ";
    std::stringstream ss;
    ss << "Vthrcomp:";
    for (unsigned int idxRoc=0;idxRoc<bbVthrcompsRoc.size();idxRoc++) {
      ss << bbVthrcompsRoc[idxRoc] << "  ";
    }
    LOG(logINFO) << ss.str();

    for (unsigned int idxRoc=0;idxRoc<aliveMaps.size();idxRoc++) {
      fApi->setDAC("vthrcomp", bbVthrcompsRoc[getIdFromIdx(idxRoc)], getIdFromIdx(idxRoc));
    }

  } else {
    fApi->setDAC("vthrcomp", fBBVthrcomp);
  }

  vector<TH2D*> bbAliveMaps = efficiencyMaps("BumpBondAlive", nTrig, FLAG_FORCE_MASKED | FLAG_CALS);

  // compare both maps to check for dead bumps
  vector<TH2D*> bbMaps;
  vector<int> nDeadBumps;
  for (unsigned int idxRoc=0;idxRoc<aliveMaps.size();idxRoc++) {
    TH2D* bbMap = bookTH2D(Form("bb_defects_C%d", getIdFromIdx(idxRoc)), Form("bb_defects_C%d", getIdFromIdx(idxRoc)), 52, 0, 52.0, 80, 0, 80.0);

    int nDeadBumpsRoc=0;
    int bbMinHits = nTrig/10;
    if (bbMinHits < 1) {
      bbMinHits = 1;
    }
    for (int ix = 0; ix < aliveMaps[idxRoc]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < aliveMaps[idxRoc]->GetNbinsY(); ++iy) {
        int aliveCount = aliveMaps[idxRoc]->GetBinContent(1+ix, 1+iy);
        int bbCount = bbAliveMaps[idxRoc]->GetBinContent(1+ix, 1+iy);

        if (aliveCount > 0 && bbCount < bbMinHits) {
          bbMap->SetBinContent(1+ix, 1+iy, 1);
          nDeadBumpsRoc++;
        }
      }
    }

    bbMaps.push_back(bbMap);
    nDeadBumps.push_back(nDeadBumpsRoc);
  }

  // BB defects per ROC histogram
  TH1D* bbDefectsPerRoc = bookTH1D("bb_defects_per_roc", "bb_defects_per_roc", 16, 0, 16.0);
  for (unsigned int idxRoc=0;idxRoc<nDeadBumps.size();idxRoc++) {
    bbDefectsPerRoc->SetBinContent(1+getIdFromIdx(idxRoc), nDeadBumps[idxRoc]);
  }
  fHistList.push_back(bbDefectsPerRoc);

  // save histograms
  for (unsigned int idxRoc=0;idxRoc<nDeadBumps.size();idxRoc++) {
    fHistList.push_back(bbAliveMaps[idxRoc]);
    fHistOptions.insert( make_pair(bbAliveMaps[idxRoc], "colz")  );
  }

  // save histograms
  for (unsigned int idxRoc=0;idxRoc<nDeadBumps.size();idxRoc++) {
    fHistList.push_back(bbMaps[idxRoc]);
    fHistOptions.insert( make_pair(bbMaps[idxRoc], "colz")  );
  }
  PixTest::update();
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), bbMaps[0] );
  (*fDisplayedHist)->Draw("colz");

  // database comparison
  std::vector<int> dbBumpDefects = readParametersFile("databaseBumpDefects.dat");

  if (dbBumpDefects.size() > 0) {

    std::stringstream ss;
    LOG(logINFO) << "results from database:";
    ss << "Dead bumps per ROC: ";
    for (unsigned int idxRoc=0;idxRoc<dbBumpDefects.size();idxRoc++) {
      ss << dbBumpDefects[idxRoc] << "  ";
    }
    LOG(logINFO) << ss.str();
  }

  // print output
  std::stringstream ss;
  LOG(logINFO) << "quicktest:";
  ss << "Dead bumps per ROC: ";
  for (unsigned int idxRoc=0;idxRoc<nDeadBumps.size();idxRoc++) {
    ss << nDeadBumps[idxRoc] << "  ";
  }
  LOG(logINFO) << ss.str();


  restoreDacs();
  PixTest::update();
  dutCalibrateOff();
}


// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::hvQuickTest() {

  banner(Form("PixTestOnShellQuickTest::hvQuickTest()"));
  // save DACs
  cacheDacs();

  // save HV on/off state
  bool hvOn = false;
  if (fPixSetup->getConfigParameters()->getHvOn()) {
    hvOn = true;
  } else {
    hvOn = false;
  }

  fApi->setDAC("ctrlreg", 0);
  fApi->setDAC("vcal", 200);

  int nTrig=10;

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  // pixel alive WITHOUT HV
  fApi->HVoff();
  vector<TH2D*> aliveMaps = efficiencyMaps("PixelAlive", nTrig, FLAG_FORCE_MASKED);

  // pixel alive WITH HV
  fApi->HVon();
  vector<TH2D*> aliveMapsHV = efficiencyMaps("PixelAlive", nTrig, FLAG_FORCE_MASKED);

  // count inefficient pixzels
  vector<int> deltaInefficienctPixels;
  for (unsigned int idxRoc=0;idxRoc<aliveMaps.size();idxRoc++) {
    int inefficientPixels = 0;
    int inefficientPixelsHV = 0;

    for (int ix = 0; ix < aliveMaps[idxRoc]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < aliveMaps[idxRoc]->GetNbinsY(); ++iy) {
        int aliveCount = aliveMaps[idxRoc]->GetBinContent(1+ix, 1+iy);
        int aliveCountHV = aliveMapsHV[idxRoc]->GetBinContent(1+ix, 1+iy);
        if (aliveCount < nTrig) {
          inefficientPixels++;
        }
        if (aliveCountHV < nTrig) {
          inefficientPixelsHV++;
        }
      }
    }

    int deltaInefficienctPixelsRoc = inefficientPixels - inefficientPixelsHV;
    deltaInefficienctPixels.push_back(deltaInefficienctPixelsRoc);
  }

  // delta alive per ROC histogram
  TH1D* deltaAlivePerRoc = bookTH1D("delta_alive_hv", "delta_alive_hv", 16, 0, 16.0);
  for (unsigned int idxRoc=0;idxRoc<deltaInefficienctPixels.size();idxRoc++) {
    deltaAlivePerRoc->SetBinContent(1+getIdFromIdx(idxRoc), deltaInefficienctPixels[idxRoc]);
  }
  fHistList.push_back(deltaAlivePerRoc);
  PixTest::update();
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), deltaAlivePerRoc);
  (*fDisplayedHist)->Draw("");

  // print output
  std::stringstream ss;
  ss << "Delta Pixel Alive with HV per ROC: ";
  for (unsigned int idxRoc=0;idxRoc<deltaInefficienctPixels.size();idxRoc++) {
    ss << deltaInefficienctPixels[idxRoc] << "  ";
  }
  LOG(logINFO) << ss.str();

  // restore HV on/off state
  if (!hvOn) {
    fApi->HVoff();
  }

  // restore DACs
  restoreDacs();

  PixTest::update();
  dutCalibrateOff();
}

