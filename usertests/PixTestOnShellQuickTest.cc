#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <iostream>  // cout
#include <fstream>
#include <iomanip>   // setw

#include <TH1.h>
#include <TRandom.h>
#include <TStopwatch.h>
#include <TStyle.h>
#include <TMarker.h>
#include <TColor.h>

#include "PixTestOnShellQuickTest.hh"
#include "PixTestFactory.hh"
#include "log.h"
#include "timer.h"
#include "helper.h"
#include "PixUtil.hh"


using namespace std;
using namespace pxar;

ClassImp(PixTestOnShellQuickTest)

// testParameters.dat
/*
-- OnShellQuickTest
ntrig               10
bbvthrcomp          120
signaltest          button
hvtest              button
bbtest              button
*/

// constructors
//------------------------------------------------------------------------------
PixTestOnShellQuickTest::PixTestOnShellQuickTest( PixSetup *a, std::string name) :
PixTest(a, name), fParNtrig(10), fBBVthrcomp(105), fParVcalS(250), fParVcal(200),
fParDeltaVthrComp(50),
fParFracCalDel(0.5),
fTargetIa(24)
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

      if (!parName.compare("vcals") ) {
        fParVcalS = atoi(sval.c_str() );
      }

      if (!parName.compare("vcal") ) {
        fParVcal = atoi(sval.c_str() );
      }

      if (!parName.compare("deltavthrcomp") ) {
        fParDeltaVthrComp = atoi(sval.c_str() );
      }

      if (!parName.compare("fraccaldel") ) {
        fParFracCalDel = atof(sval.c_str() );
      }

      if (!parName.compare("targetia")) {
        fTargetIa = atoi(sval.c_str());  // [mA/ROC]
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

  if (!command.compare("signaltest")) {
    signalTest();
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

void PixTestOnShellQuickTest::powercycleModule() {

  fApi->Poff();

  // powercycle module, since after the "adctest" test, module can be left in bad state
  TStopwatch sw;
  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
  } while (sw.RealTime() < 0.5);
  fApi->Pon();
  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
  } while (sw.RealTime() < 0.5);
  LOG(logINFO) << "powercycled";
 }

// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::doTest() {

  TStopwatch t;

  fDirectory->cd();
  PixTest::update();
  bigBanner(Form("PixTestOnShellQuickTest::doTest()"));

  fProblem = false;

  // test signal levels
  signalTest();

  // powercycle module, since after the "adctest" test, module can be left in bad state
  powercycleModule();

  // "pretest"
  programROC();
  setVana();
  findTiming();
  findWorkingPixel();
  setVthrCompCalDel();

  // PixelAlive with HV on/off
  hvQuickTest();

  // CalS signals, threshold set to value of fulltest. Pixels with missing bumps have too low signal and are below threshold
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
void PixTestOnShellQuickTest::signalTest() {
  std::string cmdString = "adctest > " + fPixSetup->getConfigParameters()->getDirectory() + "/adctest.log";
  banner(Form("PixTestOnShellQuickTest::signalTest() "));
  PixTestFactory *factory = PixTestFactory::instance();
  PixTest *t =  factory->createTest("cmd", fPixSetup);
  t->runCommand(cmdString);
  delete t;
  std::string adcFileName = fPixSetup->getConfigParameters()->getDirectory() + "/adctest.log";
  ifstream adcFile(adcFileName);
  if (adcFile) {
    std::string line;
    while (std::getline(adcFile, line)) {
        LOG(logINFO) << line;
    }
    adcFile.close();
  }

}

// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::bbQuickTest() {

  cacheDacs();
  int bbVthrcompMax = 105; //limit threshold to prevent noisy rocs

  banner(Form("PixTestOnShellQuickTest::bbQuickTest()"));
  if (fBBVthrcomp < 0) {
    LOG(logWARNING) << "no Vthrcomp cut defined, using default value!";
    fBBVthrcomp = 105;
  }
  if (fBBVthrcomp > bbVthrcompMax) {
    fBBVthrcomp = bbVthrcompMax;
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
  /*
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
   */


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
      if (bbVthrcompsRoc[getIdFromIdx(idxRoc)] > bbVthrcompMax) {
      bbVthrcompsRoc[getIdFromIdx(idxRoc)] = bbVthrcompMax;
      }
      fApi->setDAC("vthrcomp", bbVthrcompsRoc[getIdFromIdx(idxRoc)]+5, getIdFromIdx(idxRoc));
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

  PixTest::update();
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

  // pixel alive WITH HV
  fApi->HVon();
  vector<TH2D*> aliveMapsHV = efficiencyMaps("PixelAlive", nTrig, FLAG_FORCE_MASKED);

  // pixel alive WITHOUT HV
  fApi->HVoff();
  vector<TH2D*> aliveMaps = efficiencyMaps("PixelAlive", nTrig, FLAG_FORCE_MASKED);

  fApi->HVon();

  // count inefficient pixzels
  vector<int> deltaInefficienctPixels;
  vector<int> deadPixels;
  vector<int> inefficientPixels;

  for (unsigned int idxRoc=0;idxRoc<aliveMaps.size();idxRoc++) {
    int inefficientPixelsRoc = 0;
    int inefficientPixelsHVRoc = 0;
    int deadPixelsRoc = 0;

    for (int ix = 0; ix < aliveMaps[idxRoc]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < aliveMaps[idxRoc]->GetNbinsY(); ++iy) {
        int aliveCount = aliveMaps[idxRoc]->GetBinContent(1+ix, 1+iy);
        int aliveCountHV = aliveMapsHV[idxRoc]->GetBinContent(1+ix, 1+iy);
        if (aliveCount < nTrig) {
          inefficientPixelsRoc++;
        }
        if (aliveCountHV < nTrig) {
          inefficientPixelsHVRoc++;
        }
        if (aliveCountHV < 1) {
          deadPixelsRoc++;
        }
      }
    }

    int deltaInefficienctPixelsRoc = inefficientPixelsRoc - inefficientPixelsHVRoc;
    deltaInefficienctPixels.push_back(deltaInefficienctPixelsRoc);
    inefficientPixels.push_back(inefficientPixelsHVRoc);
    deadPixels.push_back(deadPixelsRoc);
  }

  // save histograms
  for (unsigned int idxRoc=0;idxRoc<aliveMapsHV.size();idxRoc++) {
    fHistList.push_back(aliveMapsHV[idxRoc]);
    fHistOptions.insert( make_pair(aliveMapsHV[idxRoc], "colz")  );
  }
  PixTest::update();

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
  std::stringstream ss3;
  ss3 << "Inefficient pixels per ROC: ";
  for (unsigned int idxRoc=0;idxRoc<inefficientPixels.size();idxRoc++) {
    ss3 << inefficientPixels[idxRoc] << "  ";
  }
  LOG(logINFO) << ss3.str();

  std::stringstream ss;
  ss << "Delta Pixel Alive with HV per ROC: ";
  for (unsigned int idxRoc=0;idxRoc<deltaInefficienctPixels.size();idxRoc++) {
    ss << deltaInefficienctPixels[idxRoc] << "  ";
  }
  LOG(logINFO) << ss.str();


  // database comparison
  std::vector<int> dbBumpDefects = readParametersFile("databaseDeadPixels.dat");
  if (dbBumpDefects.size() > 0) {
    std::stringstream ss;
    LOG(logINFO) << "Database: Dead Pixels";
    ss << "  Dead pixels per ROC: ";
    for (unsigned int idxRoc=0;idxRoc<dbBumpDefects.size();idxRoc++) {
      ss << dbBumpDefects[idxRoc] << "  ";
    }
    LOG(logINFO) << ss.str();
  }

  // dead pixels
  std::stringstream ss2;
  LOG(logINFO) << "Quicktest: Dead Pixels";
  ss2 << "  Dead pixels per ROC: ";
  for (unsigned int idxRoc=0;idxRoc<deadPixels.size();idxRoc++) {
    ss2 << deadPixels[idxRoc] << "  ";
  }
  LOG(logINFO) << ss2.str();


  // restore HV on/off state
  if (!hvOn) {
    fApi->HVoff();
  }

  // restore DACs
  restoreDacs();

  PixTest::update();
  dutCalibrateOff();
}

// copied from pretest
// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::programROC() {
  cacheDacs();
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestOnShellQuickTest::programROC() "));

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  unsigned int nRocs = rocIds.size();
  TH1D *h1 = bookTH1D("programROC", "#Delta(Iana) vs ROC", nRocs, 0., nRocs);
  fHistList.push_back(h1);

  vector<int> vanaStart;
  for (unsigned int iroc = 0; iroc < nRocs; ++iroc) {
    vanaStart.push_back(fApi->_dut->getDAC(rocIds[iroc], "vana"));
    fApi->setDAC("vana", 0, rocIds[iroc]);
  }

  pxar::mDelay(2000);
  double iA0 = fApi->getTBia()*1E3;
  //  cout << "iA0 = " << iA0 << endl;

  double iA, dA;
  string result("ROCs");
  bool problem(false);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    fApi->setDAC("vana", vanaStart[iroc], rocIds[iroc]);
    pxar::mDelay(1000);
    iA = fApi->getTBia()*1E3;
    dA = iA - iA0;
    if (dA < 5) {
      result += Form(" %d", rocIds[iroc]);
      problem = true;
    }
    h1->SetBinContent(iroc+1, dA);
    fApi->setDAC("vana", 0, rocIds[iroc]);

    gSystem->ProcessEvents();
    if (fStopTest) break;

  }

  if (problem) {
    result += " cannot be programmed! Error!";
    fProblem = true;
  } else {
    result += " are all programmable";
  }

  // -- summary printout
  string dIaString("");
  for (unsigned int i = 0; i < nRocs; ++i) {
    dIaString += Form(" %3.1f", h1->GetBinContent(i+1));
  }

  LOG(logINFO) << "PixTestOnShellQuickTest::programROC() done: " << result;
  LOG(logINFO) << "IA differences per ROC: " << dIaString;

  h1 = (TH1D*)(fHistList.back());
  h1->Draw(getHistOption(h1).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  PixTest::update();

  dutCalibrateOff();
  restoreDacs();
}

// copied from pretest
// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::findWorkingPixel() {

  gStyle->SetPalette(1);
  cacheDacs();
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestOnShellQuickTest::findWorkingPixel()"));


  vector<pair<int, int> > pixelList;
  pixelList.push_back(make_pair(12,22));
  pixelList.push_back(make_pair(5,5));
  pixelList.push_back(make_pair(15,26));
  pixelList.push_back(make_pair(20,32));
  pixelList.push_back(make_pair(25,36));
  pixelList.push_back(make_pair(30,42));
  pixelList.push_back(make_pair(35,50));
  pixelList.push_back(make_pair(40,60));
  pixelList.push_back(make_pair(45,70));
  pixelList.push_back(make_pair(50,75));

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  uint16_t FLAGS = FLAG_FORCE_MASKED;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  TH2D *h2(0);
  map<string, TH2D*> maps;

  bool gofishing(false);
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  rresults;
  int ic(-1), ir(-1);
  for (unsigned int ifwp = 0; ifwp < pixelList.size(); ++ifwp) {
    gofishing = false;
    ic = pixelList[ifwp].first;
    ir = pixelList[ifwp].second;
    fApi->_dut->testPixel(ic, ir, true);
    fApi->_dut->maskPixel(ic, ir, false);

    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
      h2 = bookTH2D(Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]),
                    Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]),
                    256, 0., 256., 256, 0., 256.);
      h2->SetMinimum(0.);
      h2->SetDirectory(fDirectory);
      fHistOptions.insert(make_pair(h2, "colz"));
      maps.insert(make_pair(Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]), h2));
    }

    rresults.clear();
    try{
      rresults = fApi->getEfficiencyVsDACDAC("caldel", 0, 255, "vthrcomp", 0, 180, FLAGS, 5);
    } catch(pxarException &e) {
      LOG(logCRITICAL) << "pXar execption: "<< e.what();
      gofishing = true;
    }

    fApi->_dut->testPixel(ic, ir, false);
    fApi->_dut->maskPixel(ic, ir, true);
    if (gofishing) continue;

    string hname;
    for (unsigned i = 0; i < rresults.size(); ++i) {
      pair<uint8_t, pair<uint8_t, vector<pixel> > > v = rresults[i];
      int idac1 = v.first;
      pair<uint8_t, vector<pixel> > w = v.second;
      int idac2 = w.first;
      vector<pixel> wpix = w.second;
      for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
        hname = Form("fwp_c%d_r%d_C%d", ic, ir, wpix[ipix].roc());
        if (maps.count(hname) > 0) {
          maps[hname]->Fill(idac1, idac2, wpix[ipix].value());
        } else {
          LOG(logDEBUG) << "bad pixel address decoded: " << hname << ", skipping";
        }
      }
    }


    bool okVthrComp(false), okCalDel(false);
    bool okAllRocs(true);
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
      okVthrComp = okCalDel = false;
      hname = Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]);
      h2 = maps[hname];

      h2->Draw("colz");
      PixTest::update();

      TH1D *hy = h2->ProjectionY("_py", 5, h2->GetNbinsX());
      double vcthrMax = hy->GetMaximum();
      double bottom   = hy->FindFirstBinAbove(0.5*vcthrMax);
      double top      = hy->FindLastBinAbove(0.5*vcthrMax);
      double vthrComp = top - 50;
      delete hy;
      if (vthrComp > bottom) {
        okVthrComp = true;
      }

      TH1D *hx = h2->ProjectionX("_px", vthrComp, vthrComp);
      double cdMax   = hx->GetMaximum();
      double cdFirst = hx->GetBinLowEdge(hx->FindFirstBinAbove(0.5*cdMax));
      double cdLast  = hx->GetBinLowEdge(hx->FindLastBinAbove(0.5*cdMax));
      delete hx;
      if (cdLast - cdFirst > 30) {
        okCalDel = true;
      }

      if (!okVthrComp || !okCalDel) {
        okAllRocs = false;
        LOG(logINFO) << hname << " does not pass: vthrComp = " << vthrComp
        << " Delta(CalDel) = " << cdLast - cdFirst << ((ifwp != pixelList.size() - 1) ? ", trying another" : ".");
        break;
      } else{
        LOG(logDEBUG) << hname << " OK, with vthrComp = " << vthrComp << " and Delta(CalDel) = " << cdLast - cdFirst;
      }
    }
    if (okAllRocs) {
      for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
        string name = Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]);
        TH2D *h = maps[name];
        fHistList.push_back(h);
        h->Draw(getHistOption(h).c_str());
        fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
        PixTest::update();
      }
      break;
    } else {
      for (map<string, TH2D*>::iterator il = maps.begin(); il != maps.end(); ++il) {
        delete (*il).second;
      }
      maps.clear();
    }
  }

  if (maps.size()) {
    LOG(logINFO) << "Found working pixel in all ROCs: col/row = " << ic << "/" << ir;
    clearSelectedPixels();
    fPIX.push_back(make_pair(ic, ir));
    addSelectedPixels(Form("%d,%d", ic, ir));
  } else {
    LOG(logINFO) << "Something went wrong...";
    LOG(logINFO) << "Didn't find a working pixel in all ROCs.";
    for (size_t iroc = 0; iroc < rocIds.size(); iroc++) {
      LOG(logINFO) << "our roc list from in the dut: " << static_cast<int>(rocIds[iroc]);
    }
    fProblem = true;
  }

  dutCalibrateOff();
  restoreDacs();

}

// copied from pretest
// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::setVthrCompCalDel() {
  uint16_t FLAGS = FLAG_FORCE_MASKED;

  gStyle->SetPalette(1);
  cacheDacs();
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestOnShellQuickTest::setVthrCompCalDel()"));

  string name("pretestVthrCompCalDel");

  fApi->setDAC("CtrlReg", 0);
  fApi->setDAC("Vcal", 250);

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();

  TH1D *h1(0);
  h1 = bookTH1D(Form("pretestCalDel"), Form("pretestCalDel"), rocIds.size(), 0., rocIds.size());
  h1->SetMinimum(0.);
  h1->SetDirectory(fDirectory);
  setTitles(h1, "ROC", "CalDel DAC");

  TH2D *h2(0);

  vector<int> calDel(rocIds.size(), -1);
  vector<int> vthrComp(rocIds.size(), -1);
  vector<int> calDelE(rocIds.size(), -1);

  int ip = 0;

  fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, true);
  fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, false);

  LOG(logINFO) << "/1";
  map<int, TH2D*> maps;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    h2 = bookTH2D(Form("%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]),
                  Form("%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]),
                  255, 0., 255., 255, 0., 255.);
    fHistOptions.insert(make_pair(h2, "colz"));
    maps.insert(make_pair(rocIds[iroc], h2));
    h2->SetMinimum(0.);
    h2->SetMaximum(fParNtrig);
    h2->SetDirectory(fDirectory);
    setTitles(h2, "CalDel", "VthrComp");
  }

  LOG(logINFO) << "/2";
  bool done = false;
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  rresults;
  while (!done) {
    rresults.clear();
    int cnt(0);
    gSystem->ProcessEvents();
    if (fStopTest) break;

    try{
      rresults = fApi->getEfficiencyVsDACDAC("caldel", 0, 255, "vthrcomp", 0, 180, FLAGS, fParNtrig);
      done = true;
    } catch(DataMissingEvent &e){
      LOG(logCRITICAL) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events";
      ++cnt;
      if (e.numberMissing > 10) done = true;
    } catch(pxarException &e) {
      LOG(logCRITICAL) << "pXar execption: "<< e.what();
      ++cnt;
    }
    done = (cnt>5) || done;
  }

  LOG(logINFO) << "/3";
  fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, false);
  fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, true);

  for (unsigned i = 0; i < rresults.size(); ++i) {
    pair<uint8_t, pair<uint8_t, vector<pixel> > > v = rresults[i];
    int idac1 = v.first;
    pair<uint8_t, vector<pixel> > w = v.second;
    int idac2 = w.first;
    vector<pixel> wpix = w.second;
    for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
      maps[wpix[ipix].roc()]->Fill(idac1, idac2, wpix[ipix].value());
    }
  }
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    h2 = maps[rocIds[iroc]];
    TH1D *hy = h2->ProjectionY("_py", 5, h2->GetNbinsX());
    double vcthrMax = hy->GetMaximum();
    double bottom   = hy->FindFirstBinAbove(0.5*vcthrMax);
    double top      = hy->FindLastBinAbove(0.5*vcthrMax);
    delete hy;

    if (fParDeltaVthrComp>0) {
      vthrComp[iroc] = bottom + fParDeltaVthrComp;
    } else {
      vthrComp[iroc] = top + fParDeltaVthrComp;
    }

    TH1D *h0 = h2->ProjectionX("_px", vthrComp[iroc], vthrComp[iroc]);
    double cdMax   = h0->GetMaximum();
    double cdFirst = h0->GetBinLowEdge(h0->FindFirstBinAbove(0.5*cdMax));
    double cdLast  = h0->GetBinLowEdge(h0->FindLastBinAbove(0.5*cdMax));
    calDelE[iroc] = static_cast<int>(cdLast - cdFirst);
    calDel[iroc] = static_cast<int>(cdFirst + fParFracCalDel*calDelE[iroc]);
    TMarker *pm = new TMarker(calDel[iroc], vthrComp[iroc], 21);
    pm->SetMarkerColor(kWhite);
    pm->SetMarkerSize(2);
    h2->GetListOfFunctions()->Add(pm);
    pm = new TMarker(calDel[iroc], vthrComp[iroc], 7);
    pm->SetMarkerColor(kBlack);
    pm->SetMarkerSize(0.2);
    h2->GetListOfFunctions()->Add(pm);
    delete h0;

    h1->SetBinContent(rocIds[iroc]+1, calDel[iroc]);
    h1->SetBinError(rocIds[iroc]+1, 0.5*calDelE[iroc]);
    LOG(logDEBUG) << "CalDel: " << calDel[iroc] << " +/- " << 0.5*calDelE[iroc];

    h2->Draw(getHistOption(h2).c_str());
    PixTest::update();

    fHistList.push_back(h2);
  }

  fHistList.push_back(h1);

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  PixTest::update();

  restoreDacs();
  string caldelString(""), vthrcompString("");
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    if (calDel[iroc] > 0) {
      fApi->setDAC("CalDel", calDel[iroc], rocIds[iroc]);
      caldelString += Form("  %4d", calDel[iroc]);
    } else {
      caldelString += Form(" _%4d", fApi->_dut->getDAC(rocIds[iroc], "caldel"));
    }
    fApi->setDAC("VthrComp", vthrComp[iroc], rocIds[iroc]);
    vthrcompString += Form("  %4d", vthrComp[iroc]);
  }

  // -- summary printout
  LOG(logINFO) << "PixTestOnShellQuickTest::setVthrCompCalDel() done";
  LOG(logINFO) << "CalDel:   " << caldelString;
  LOG(logINFO) << "VthrComp: " << vthrcompString;

  dutCalibrateOff();
}


// copied from pretest
// ----------------------------------------------------------------------
void PixTestOnShellQuickTest::setVana() {

  cacheDacs();
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestOnShellQuickTest::setVana() target Ia = %d mA/ROC", fTargetIa));

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  vector<uint8_t> vanaStart;
  vector<double> rocIana;

  // -- cache setting and switch off all(!) ROCs
  int nRocs = fApi->_dut->getNRocs();
  for (int iroc = 0; iroc < nRocs; ++iroc) {
    vanaStart.push_back(fApi->_dut->getDAC(iroc, "vana"));
    rocIana.push_back(0.);
    fApi->setDAC("vana", 0, iroc);
  }

  double i016 = fApi->getTBia()*1E3;

  // FIXME this should not be a stopwatch, but a delay
  TStopwatch sw;
  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    i016 = fApi->getTBia()*1E3;
  } while (sw.RealTime() < 0.1);

  // subtract one ROC to get the offset from the other Rocs (on average):
  double i015 = (nRocs-1) * i016 / nRocs; // = 0 for single chip tests
  LOG(logDEBUG) << "offset current from other " << nRocs-1 << " ROCs is " << i015 << " mA";

  // tune per ROC:

  const double extra = 0.1; // [mA] besser zu viel als zu wenig
  const double eps = 0.25; // [mA] convergence
  const double slope = 6; // 255 DACs / 40 mA

  for (int roc = 0; roc < nRocs; ++roc) {

    gSystem->ProcessEvents();
    if (fStopTest) break;

    if (!selectedRoc(roc)) {
      LOG(logDEBUG) << "skipping ROC idx = " << roc << " (not selected) for Vana tuning";
      continue;
    }
    int vana = vanaStart[roc];
    fApi->setDAC("vana", vana, roc); // start value

    double ia = fApi->getTBia()*1E3; // [mA], just to be sure to flush usb
    sw.Start(kTRUE); // reset
    do {
      sw.Start(kFALSE); // continue
      ia = fApi->getTBia()*1E3; // [mA]
    } while (sw.RealTime() < 0.1);

    double diff = fTargetIa + extra - (ia - i015);

    int iter = 0;
    LOG(logDEBUG) << "ROC " << roc << " iter " << iter
    << " Vana " << vana
    << " Ia " << ia-i015 << " mA";

    while (TMath::Abs(diff) > eps && iter < 11 && vana >= 0 && vana < 255) {

      int stp = static_cast<int>(TMath::Abs(slope*diff));
      if (stp == 0) stp = 1;
      if (diff < 0) stp = -stp;

      vana += stp;

      if (vana < 0) {
        vana = 0;
      } else {
        if (vana > 255) {
          vana = 255;
        }
      }

      fApi->setDAC("vana", vana, roc);
      iter++;

      sw.Start(kTRUE); // reset
      do {
        sw.Start(kFALSE); // continue
        ia = fApi->getTBia()*1E3; // [mA]
      }
      while( sw.RealTime() < 0.1 );

      diff = fTargetIa + extra - (ia - i015);

      LOG(logDEBUG) << "ROC " << setw(2) << roc
      << " iter " << setw(2) << iter
      << " Vana " << setw(3) << vana
      << " Ia " << ia-i015 << " mA";
    } // iter

    rocIana[roc] = ia-i015; // more or less identical for all ROCS?!
    vanaStart[roc] = vana; // remember best
    fApi->setDAC( "vana", 0, roc ); // switch off for next ROC

  } // rocs

  TH1D *hsum = bookTH1D("VanaSettings", "Vana per ROC", nRocs, 0., nRocs);
  setTitles(hsum, "ROC", "Vana [DAC]");
  hsum->SetStats(0);
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);

  TH1D *hcurr = bookTH1D("Iana", "Iana per ROC", nRocs, 0., nRocs);
  setTitles(hcurr, "ROC", "Iana [mA]");
  hcurr->SetStats(0); // no stats
  hcurr->SetMinimum(0);
  hcurr->SetMaximum(30.0);
  fHistList.push_back(hcurr);


  restoreDacs();
  for (int roc = 0; roc < nRocs; ++roc) {
    // -- reset all ROCs to optimum or cached value
    fApi->setDAC( "vana", vanaStart[roc], roc );
    LOG(logDEBUG) << "ROC " << setw(2) << roc << " Vana " << setw(3) << int(vanaStart[roc]);
    // -- histogramming only for those ROCs that were selected
    if (!selectedRoc(roc)) continue;
    hsum->Fill(roc, vanaStart[roc] );
    hcurr->Fill(roc, rocIana[roc]);
  }

  double ia16 = fApi->getTBia()*1E3; // [mA]

  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    ia16 = fApi->getTBia()*1E3; // [mA]
  }
  while( sw.RealTime() < 0.1 );


  hsum->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), hsum);
  PixTest::update();


  // -- test that current drops when turning off single ROCs
  cacheDacs();
  double iAll = fApi->getTBia()*1E3;
  sw.Start(kTRUE);
  do {
    sw.Start(kFALSE);
    iAll = fApi->getTBia()*1E3;
  } while (sw.RealTime() < 0.1);

  double iMinus1(0), vanaOld(0);
  vector<double> iLoss;
  for (int iroc = 0; iroc < nRocs; ++iroc) {
    vanaOld = fApi->_dut->getDAC(iroc, "vana");
    fApi->setDAC("vana", 0, iroc);

    iMinus1 = fApi->getTBia()*1E3; // [mA], just to be sure to flush usb
    sw.Start(kTRUE); // reset
    do {
      sw.Start(kFALSE); // continue
      iMinus1 = fApi->getTBia()*1E3; // [mA]
    } while (sw.RealTime() < 0.1);
    iLoss.push_back(iAll-iMinus1);

    fApi->setDAC("vana", vanaOld, iroc);
  }

  string vanaString(""), vthrcompString("");
  for (int iroc = 0; iroc < nRocs; ++iroc){
    if (iLoss[iroc] < 15) {
      vanaString += Form("  ->%3.1f<-", iLoss[iroc]);
      fProblem = true;
    } else {
      vanaString += Form("  %3.1f", iLoss[iroc]);
    }
  }
  // -- summary printout
  LOG(logINFO) << "PixTestOnShellQuickTest::setVana() done, Module Ia " << ia16 << " mA = " << ia16/nRocs << " mA/ROC";
  LOG(logINFO) << "i(loss) [mA/ROC]:   " << vanaString;


  restoreDacs();


  dutCalibrateOff();
}

// copied from pretest
// ----------------------------------------------------------------------
// this is quite horrible, but a consequence of the parallel world in PixTestCmd which I do not intend to duplicate here
void PixTestOnShellQuickTest::findTiming() {

  banner(Form("PixTestOnShellQuickTest::findTiming() "));
  // do a test of current settings first

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  fApi->_dut->testPixel(13,37,true);
  fApi->_dut->maskPixel(13,37,false);

  TLogLevel UserReportingLevel = Log::ReportingLevel();
  size_t nTBMs = fApi->_dut->getNTbms();
  if (nTBMs==0) {
    LOG(logINFO) << "Timing test not needed for single ROC.";
    return;
  }
  int nTokenChains = 0;
  std::vector<tbmConfig> enabledTBMs = fApi->_dut->getEnabledTbms();
  for(std::vector<tbmConfig>::iterator enabledTBM = enabledTBMs.begin(); enabledTBM != enabledTBMs.end(); enabledTBM++) nTokenChains += enabledTBM->tokenchains.size();

  int NTrig = 10000;
  uint16_t period = 300;
  int TrigBuffer = 3;

  vector<rawEvent> daqRawEv;
  vector<Event> daqEv;

  bool GoodDelaySettings = false;
  for (int itry = 0; itry < 3 && !GoodDelaySettings; itry++) {
    LOG(logINFO) << "Testing Timing: Attempt #" << itry+1;
    fApi->daqStart();
    Log::ReportingLevel() = Log::FromString("QUIET");
    bool goodreadback = checkReadBackBits(period);
    LOG(logINFO) << "readback = " << (int)goodreadback;
    LOG(logINFO) << "ignore it in this test";
    goodreadback = true; //we don't need readback for now
    if (goodreadback) {
      statistics results = getEvents(NTrig, period, TrigBuffer);
      Log::ReportingLevel() = UserReportingLevel;
      int NEvents = (results.info_events_empty()+results.info_events_valid())/nTokenChains;
      int NErrors = results.errors_tbm_header() + results.errors_tbm_trailer() + results.errors_roc_missing();
      if (NEvents==NTrig && NErrors==0) GoodDelaySettings=true;
      LOG(logINFO) << "NErrors = " << (int)NErrors;
      LOG(logINFO) << "NEvents = " << (int)NEvents;
    }
    Log::ReportingLevel() = UserReportingLevel;
    fApi->daqStop();
  }

  if (!GoodDelaySettings) {
    powercycleModule();
      for (int itry = 3; itry < 6 && !GoodDelaySettings; itry++) {
        LOG(logINFO) << "Testing Timing: Attempt #" << itry+1;
        fApi->daqStart();
        Log::ReportingLevel() = Log::FromString("QUIET");
        bool goodreadback = checkReadBackBits(period);
        LOG(logINFO) << "readback = " << (int)goodreadback;
        LOG(logINFO) << "ignore it in this test";
        goodreadback = true; //we don't need readback for now
        if (goodreadback) {
          statistics results = getEvents(NTrig, period, TrigBuffer);
          Log::ReportingLevel() = UserReportingLevel;
          int NEvents = (results.info_events_empty()+results.info_events_valid())/nTokenChains;
          int NErrors = results.errors_tbm_header() + results.errors_tbm_trailer() + results.errors_roc_missing();
          if (NEvents==NTrig && NErrors==0) GoodDelaySettings=true;
          LOG(logINFO) << "NErrors = " << (int)NErrors;
          LOG(logINFO) << "NEvents = " << (int)NEvents;
        }
        Log::ReportingLevel() = UserReportingLevel;
        fApi->daqStop();
      }
  }


  if (GoodDelaySettings) {
    LOG(logINFO) << "Timings are already good, no scan needed!";
    return;
  } else {
    LOG(logINFO) << "Timings are not good, scanning... ";
  }

  PixTestFactory *factory = PixTestFactory::instance();
  PixTest *t =  factory->createTest("cmd", fPixSetup);
  t->runCommand("timing");
  delete t;

  // -- parse output file
  ifstream INS;
  char buffer[1000];
  string sline, sparameters, ssuccess;
  string::size_type s1;
  vector<double> x;
  INS.open("pxar_timing.log");
  while (INS.getline(buffer, 1000, '\n')) {
    sline = buffer;
    s1 = sline.find("selecting");
    if (string::npos == s1) continue;
    sparameters = sline;
    INS.getline(buffer, 1000, '\n');
    ssuccess = buffer;
  }
  INS.close();

  INS.open("pxar_timing.log");
  while (INS.getline(buffer, 1000, '\n')) {
    LOG(logINFO) << ">" << buffer;
  }
  INS.close();

  // -- parse relevant lines
  int tries(-1), success(-1);
  istringstream istring(ssuccess);
  istring >> sline >> sline >> success >> sline >> tries;
  istring.clear();
  istring.str(sparameters);
  int i160(-1), i400(-1), iroc(-1), iht(-1), itoken(-1), iwidth(-1);
  istring >> sline >> i160 >> i400 >> iroc >> iht >> itoken >> sline >> sline >> iwidth;
  LOG(logINFO) << "TBM phases:  160MHz: " << i160 << ", 400MHz: " << i400
  << ", TBM delays: ROC(0/1):" << iroc << ", header/trailer: " << iht << ", token: " << itoken;
  LOG(logINFO) << "(success/tries = " << success << "/" << tries << "), width = " << iwidth;

  uint8_t value= ((i160 & 0x7)<<5) + ((i400 & 0x7)<<2);
  int stat = tbmSet("basee", 0, value);
  if (stat > 0){
    LOG(logWARNING) << "error setting delay  base E " << hex << value << dec;
  }

  if (iroc >= 0){
    value = ((itoken & 0x1)<<7) + ((iht & 0x1)<<6) + ((iroc & 0x7)<<3) + (iroc & 0x7);
    stat = tbmSet("basea",2, value);
    if (stat > 0){
      LOG(logWARNING) << "error setting delay  base A " << hex << value << dec;
    }
  }
  tbmSet("base4", 2, 0x80); // reset once after changing phases

  if (success < 0) fProblem = true;
  powercycleModule();

}

