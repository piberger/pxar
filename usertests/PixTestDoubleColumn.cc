#include <stdlib.h>     
#include <algorithm>    
#include <iostream>
#include <fstream>

#include <TH1.h>
#include <TRandom.h>
#include <TStopwatch.h>
#include <TStyle.h>

#include "PixTestDoubleColumn.hh"
#include "PixUtil.hh"
#include "log.h"
#include "helper.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestDoubleColumn)

/*

-- DoubleColumn
min                 0
max                 23
npix                2
delay               1000
data                button
*/

// ----------------------------------------------------------------------
PixTestDoubleColumn::PixTestDoubleColumn(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(1), fParNpix(1), fParDelay(1000),fParTsMin(0),fParTsMax(23),fParDaqDatRead(false) {
  PixTest::init();
  init(); 
  //  LOG(logINFO) << "PixTestDoubleColumn ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestDoubleColumn::PixTestDoubleColumn() : PixTest() {
  //  LOG(logINFO) << "PixTestDoubleColumn ctor()";
}

// ----------------------------------------------------------------------
bool PixTestDoubleColumn::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      if (!parName.compare("ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
      }
      if (!parName.compare("min")) {
  fParTsMin = atoi(sval.c_str()); 
      }
      if (!parName.compare("max")) {
  fParTsMax = atoi(sval.c_str()); 
      }
      if (!parName.compare("npix")) {
	fParNpix = atoi(sval.c_str()); 
  if (fParNpix > 4) {
    fParNpix = 4;
  }
      }
      if (!parName.compare("delay")) {
  fParDelay = atoi(sval.c_str()); 
      }
      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestDoubleColumn::init() {
  setToolTips(); 
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestDoubleColumn::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("data")) {
    testData();
    return;
  }

  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestDoubleColumn::setToolTips() {
  fTestTip    = string("")
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;
}

// ----------------------------------------------------------------------
void PixTestDoubleColumn::bookHist(string name) {
  fDirectory->cd(); 

  LOG(logDEBUG) << "nothing done with " << name;
}


//----------------------------------------------------------
PixTestDoubleColumn::~PixTestDoubleColumn() {
  LOG(logDEBUG) << "PixTestDoubleColumn dtor";
}


void PixTestDoubleColumn::resetDaq() {
fParDaqDatRead = false;
}

std::vector< std::vector<int> > PixTestDoubleColumn::readData(int nEv) {

  if (!fParDaqDatRead) {
    try { 
      fParDaqDat = fApi->daqGetEventBuffer(); 
      fParDaqDatRead = true;
    }
    catch(pxar::DataNoEvent &) {}
  }
  
  int nRocs = fApi->_dut->getNEnabledRocs();
  std::vector<int> doubleColumnHitsRoc(52,0);
  std::vector< std::vector<int> > doubleColumnHits(nRocs, doubleColumnHitsRoc);

  //for (std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
  if (nEv < fParDaqDat.size()) {
    pxar::Event it = fParDaqDat.at(nEv);
    int idx(0); 
    for (unsigned int ipix = 0; ipix < it.pixels.size(); ++ipix) {   
      idx = getIdxFromId(it.pixels[ipix].roc());
      int doubleColumn = it.pixels[ipix].column() / 2;
      //LOG(logINFO) << "col " << (int)it.pixels[ipix].column();
      doubleColumnHits[idx][doubleColumn]++;
    }
  }
  //}
  return doubleColumnHits;
}


void PixTestDoubleColumn::testBuffers(std::vector<TH2D*> hX, int tsMin, int tsMax) {
  int nRocs = fApi->_dut->getNEnabledRocs();

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  uint8_t wbc = 240;
  uint8_t delay = 6;

  fApi->setDAC("wbc", wbc);
  resetROC();

  fPg_setup.clear();
  resetROC();

  for (int nTestTimestamps=tsMin;nTestTimestamps<tsMax+1;nTestTimestamps++) {
    LOG(logINFO) << "#timestamps =  " << (int)nTestTimestamps << ", #hits = " << nTestTimestamps*fParNpix;
    // -- pattern generator setup without resets
    fPg_setup.push_back(std::make_pair("resetroc",15));    // PG_REST
    fPg_setup.push_back(std::make_pair("delay",15));    // PG_REST
    for (int i=0;i<nTestTimestamps;i++) {
    fPg_setup.push_back(std::make_pair("calibrate",65)); // PG_CAL
    }
    fPg_setup.push_back(std::make_pair("calibrate",wbc+delay)); // PG_CAL
    if (nTestTimestamps == tsMax) {
      fPg_setup.push_back(std::make_pair("trigger;sync",0));     // PG_TRG PG_SYNC
    } else {
      fPg_setup.push_back(std::make_pair("trigger;sync",250));     // PG_TRG PG_SYNC
      fPg_setup.push_back(std::make_pair("delay",250));    // PG_REST
    }

}

  int period = 22000;
  LOG(logDEBUG) << "set pattern generator to:";
  for (unsigned int i = 0; i < fPg_setup.size(); ++i) LOG(logDEBUG) << fPg_setup[i].first << ": " << (int)fPg_setup[i].second;
  fApi->setPatternGenerator(fPg_setup);

  for (int iDcTest=0;iDcTest<26;iDcTest++) {
    LOG(logINFO) << "testing double column " << (int)iDcTest;
    if (fParNpix == 1) {
      fApi->_dut->testPixel(2*iDcTest,10,true);
      fApi->_dut->maskPixel(2*iDcTest,10,false);
    } else if (fParNpix == 2) {
      fApi->_dut->testPixel(2*iDcTest,10,true);
      fApi->_dut->maskPixel(2*iDcTest,10,false);
      fApi->_dut->testPixel(2*iDcTest+1,10,true);
      fApi->_dut->maskPixel(2*iDcTest+1,10,false);
    } else if (fParNpix == 3) {
      fApi->_dut->testPixel(2*iDcTest,10,true);
      fApi->_dut->maskPixel(2*iDcTest,10,false);
      fApi->_dut->testPixel(2*iDcTest+1,10,true);
      fApi->_dut->maskPixel(2*iDcTest+1,10,false);
      fApi->_dut->testPixel(2*iDcTest,11,true);
      fApi->_dut->maskPixel(2*iDcTest,11,false);
    } else {
      fApi->_dut->testPixel(2*iDcTest,10,true);
      fApi->_dut->maskPixel(2*iDcTest,10,false);
      fApi->_dut->testPixel(2*iDcTest+1,10,true);
      fApi->_dut->maskPixel(2*iDcTest+1,10,false);
      fApi->_dut->testPixel(2*iDcTest,11,true);
      fApi->_dut->maskPixel(2*iDcTest,11,false);
      fApi->_dut->testPixel(2*iDcTest+1,11,true);
      fApi->_dut->maskPixel(2*iDcTest+1,11,false);
    }

    // DAQ
    fApi->daqStart();
    fApi->daqTrigger(1, period);
    fApi->daqStop();
    std::vector< std::vector<int> > doubleColumnHits;

    // fill histograms
    int iEv=0;
    for (int nTestTimestamps=tsMin;nTestTimestamps<tsMax+1;nTestTimestamps++) {
      doubleColumnHits = readData(iEv);
      for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
        hX[iRocIdx]->Fill(iDcTest+0.1, nTestTimestamps+0.1, doubleColumnHits[iRocIdx][iDcTest]);
      }
      iEv++;
    }
    resetDaq();

    if (fParNpix == 1) {
      fApi->_dut->testPixel(2*iDcTest,10,false);
      fApi->_dut->maskPixel(2*iDcTest,10,true);
    } else if (fParNpix == 2) {
      fApi->_dut->testPixel(2*iDcTest,10,false);
      fApi->_dut->maskPixel(2*iDcTest,10,true);
      fApi->_dut->testPixel(2*iDcTest+1,10,false);
      fApi->_dut->maskPixel(2*iDcTest+1,10,true);
    } else if (fParNpix == 3) {
      fApi->_dut->testPixel(2*iDcTest,10,false);
      fApi->_dut->maskPixel(2*iDcTest,10,true);
      fApi->_dut->testPixel(2*iDcTest+1,10,false);
      fApi->_dut->maskPixel(2*iDcTest+1,10,true);
      fApi->_dut->testPixel(2*iDcTest,11,false);
      fApi->_dut->maskPixel(2*iDcTest,11,true);
    } else {
      fApi->_dut->testPixel(2*iDcTest,10,false);
      fApi->_dut->maskPixel(2*iDcTest,10,true);
      fApi->_dut->testPixel(2*iDcTest+1,10,false);
      fApi->_dut->maskPixel(2*iDcTest+1,10,true);
      fApi->_dut->testPixel(2*iDcTest,11,false);
      fApi->_dut->maskPixel(2*iDcTest,11,true);
      fApi->_dut->testPixel(2*iDcTest+1,11,false);
      fApi->_dut->maskPixel(2*iDcTest+1,11,true);
    }
  }
}

void PixTestDoubleColumn::testData() {
  cacheDacs();

  std::vector<TH2D*> hX;

  int nRocs = fApi->_dut->getNEnabledRocs();

  PixTest::update(); 
  for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
    TH2D* rocHist  = bookTH2D(Form("npix%d_C%d", fParNpix, getIdFromIdx(iRocIdx)),  Form("npix%d_C%d", fParNpix, getIdFromIdx(iRocIdx)),  26, 0, 26.0, fParTsMax-fParTsMin+1, fParTsMin, fParTsMax+1);
    hX.push_back(rocHist);
  }

  int ts=fParTsMin;
  int tsTo=fParTsMin;

  while (ts <= fParTsMax) {
    tsTo = ts+12;
    if (tsTo>fParTsMax) {
      tsTo = fParTsMax;
    }
    LOG(logINFO) << "testing FROM " << (int)ts << " TO " << (int)tsTo;
    testBuffers(hX, ts, tsTo);
    ts = tsTo+1;
  }

  for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
    fHistList.push_back(hX[iRocIdx]);
    fHistOptions.insert( make_pair(hX[iRocIdx], "colz")  ); 
  }

  PixTest::update(); 
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), hX[0] );
  (*fDisplayedHist)->Draw("colz");

  restoreDacs();
  PixTest::update(); 
  dutCalibrateOff();

}

// ----------------------------------------------------------------------
void PixTestDoubleColumn::doTest() {

  TStopwatch t;

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestDoubleColumn::doTest()"));

  testData();

  int seconds = t.RealTime(); 
  LOG(logINFO) << "PixTestDoubleColumn::doTest() done, duration: " << seconds << " seconds";
}