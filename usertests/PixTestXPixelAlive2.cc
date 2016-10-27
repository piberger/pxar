#include <stdlib.h>     
#include <algorithm>    
#include <iostream>
#include <fstream>

#include <TH1.h>
#include <TRandom.h>
#include <TStopwatch.h>
#include <TStyle.h>
#include <TMath.h>

#include "PixTestXPixelAlive2.hh"
#include "PixUtil.hh"
#include "log.h"
#include "helper.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestXPixelAlive2)


/*
-- XPixelAlive2
vcal                200
ntrig               10
*/


// ----------------------------------------------------------------------
PixTestXPixelAlive2::PixTestXPixelAlive2(PixSetup *a, std::string name) : PixTest(a, name), fParVcal(200), fParNtrig(50), fParReset(1),fParMask(-1),fParDelayTBM(false), fParScurveLow(50), fParScurveHigh(110) {
	PixTest::init();
	init(); 

	fPhCal.setPHParameters(fPixSetup->getConfigParameters()->getGainPedestalParameters());
	fPhCalOK = fPhCal.initialized();
	//  LOG(logINFO) << "PixTestXPixelAlive2 ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestXPixelAlive2::PixTestXPixelAlive2() : PixTest() {
	//  LOG(logINFO) << "PixTestXPixelAlive2 ctor()";
}

void PixTestXPixelAlive2::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;

  if (!command.compare("xpixelalive")) {
    xpixelalive();
    return;
  }

  if (!command.compare("caldelscan")) {
    doCalDelScan();
    return;
  }

  if (!command.compare("xnoisemaps")) {
    doXNoiseMaps();
    return;
  }

  if (!command.compare("incedgethr")) {
    increaseEdgePixelThreshold();
    return;
  }


  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}

// ----------------------------------------------------------------------
bool PixTestXPixelAlive2::setParameter(string parName, string sval) {
	bool found(false);
	string str1, str2; 
	std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
	for (unsigned int i = 0; i < fParameters.size(); ++i) {
		if (fParameters[i].first == parName) {
			found = true; 
			if (!parName.compare("resetroc")) {
				PixUtil::replaceAll(sval, "checkbox(", ""); 
				PixUtil::replaceAll(sval, ")", ""); 
				fParReset = atoi(sval.c_str()); 
				setToolTips();
			}
			if (!parName.compare("maskborder")) {
				PixUtil::replaceAll(sval, "checkbox(", ""); 
				PixUtil::replaceAll(sval, ")", ""); 
				fParMask = atoi(sval.c_str()); 
				setToolTips();
			}
			if (!parName.compare("delaytbm")) {
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fParDelayTBM = !(atoi(sval.c_str())==0);
				setToolTips();
      			}
			if (!parName.compare("ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
			}
			if (!parName.compare("vcal")) {
	fParVcal = atoi(sval.c_str()); 
			}
      if (!parName.compare("daclo")) {
  fParScurveLow = atoi(sval.c_str()); 
      }
      if (!parName.compare("dachi")) {
  fParScurveHigh = atoi(sval.c_str()); 
      }
			break;
		}
	}
	
	return found; 
}


// ----------------------------------------------------------------------
void PixTestXPixelAlive2::init() {
	setToolTips(); 
	fDirectory = gFile->GetDirectory(fName.c_str()); 
	if (!fDirectory) {
		fDirectory = gFile->mkdir(fName.c_str()); 
	} 
	fDirectory->cd(); 

}



// ----------------------------------------------------------------------
void PixTestXPixelAlive2::setToolTips() {
	fTestTip    = string(Form("send n number of calibrates to one pixel\n")
					 + string("TO BE FINISHED!!"))
		;
	fSummaryTip = string("summary plot to be implemented")
		;
}
// ----------------------------------------------------------------------
void PixTestXPixelAlive2::bookHist(string name) {
  fDirectory->cd();

}


//----------------------------------------------------------
PixTestXPixelAlive2::~PixTestXPixelAlive2() {
	LOG(logDEBUG) << "PixTestXPixelAlive2 dtor";
}


// ----------------------------------------------------------------------
void PixTestXPixelAlive2::doTest() {
	xpixelalive();

}

void PixTestXPixelAlive2::xpixelalive() {
	ConfigParameters* config = fPixSetup->getConfigParameters();

	TStopwatch t;

	vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
        unsigned nrocs = rocIds.size();

	fDirectory->cd();
	PixTest::update(); 
	bigBanner(Form("PixTestXPixelAlive2::doTest()"));

	fProblem = false;
	gStyle->SetPalette(1);
	bool verbose(false);
	cacheDacs(verbose);
	fDirectory->cd();
	PixTest::update(); 

	  // -- cache triggerdelay
  vector<pair<string, uint8_t> > oldDelays = fPixSetup->getConfigParameters()->getTbSigDelays();
  bool foundIt(false);
  for (unsigned int i = 0; i < oldDelays.size(); ++i) {
    if (oldDelays[i].first == "triggerdelay") {
      foundIt = true;
    }
    LOG(logDEBUG) << " old set: " << oldDelays[i].first << ": " << (int)oldDelays[i].second;
  }

  vector<pair<string, uint8_t> > delays = fPixSetup->getConfigParameters()->getTbSigDelays();
  if (!foundIt) {
    delays.push_back(make_pair("triggerdelay", 20));
    oldDelays.push_back(make_pair("triggerdelay", 0));
  } else {
    for (unsigned int i = 0; i < delays.size(); ++i) {
      if (delays[i].first == "triggerdelay") {
	delays[i].second = 20;
      }
    }
  }

  //fApi->setTbmReg("delays",0x40);

  for (unsigned int i = 0; i < delays.size(); ++i) {
    LOG(logDEBUG) << " setting: " << delays[i].first << ": " << (int)delays[i].second;
  }
  fApi->setTestboardDelays(delays);

	vector<pair<string, uint8_t> > a;
	
	/*uint8_t wbc = 100;
	uint8_t delay = 5;
	fApi->setDAC("Vcal", fParVcal);
	fPg_setup.clear();
	if (fParReset == 1) {
		a.push_back(make_pair("resetroc",50));    // PG_RESR b001000 
	}
	a.push_back(make_pair("calibrate",wbc+delay)); // PG_CAL  b000100
	LOG(logINFO) << config->getNtbms() ;
	if (config->getNtbms() < 1) {
	a.push_back(make_pair("trigger",16));    // PG_TRG  b000010
	a.push_back(make_pair("token",0));     // PG_TOK  b000001
	}
	else {
	a.push_back(std::make_pair("trigger;sync",0));     // PG_TRG PG_SYNC
	}
	for (unsigned i = 0; i < a.size(); ++i) {
		fPg_setup.push_back(a[i]);
	}



	fApi->setPatternGenerator(fPg_setup);*/

if (fParReset == 0) {
  fPg_setup.clear();
  vector<pair<string, uint8_t> > pgtmp = fPixSetup->getConfigParameters()->getTbPgSettings();
  for (unsigned i = 0; i < pgtmp.size(); ++i) {
    if (string::npos != pgtmp[i].first.find("resetroc")) continue;
    if (string::npos != pgtmp[i].first.find("resettbm")) continue;
    fPg_setup.push_back(pgtmp[i]);
  }
  if (0) for (unsigned int i = 0; i < fPg_setup.size(); ++i) cout << fPg_setup[i].first << ": " << (int)fPg_setup[i].second << endl;

  fApi->setPatternGenerator(fPg_setup);
}

  //fPg_setup.clear();
  //fPg_setup.push_back(make_pair("resetroc",50));
  //fPg_setup.push_back(make_pair("calibrate",105)); // PG_CAL
  //fPg_setup.push_back(make_pair("trigger;sync",0));     // PG_TRG PG_SYNC
  //fApi->setPatternGenerator(fPg_setup);



	fApi->_dut->maskAllPixels(false);

	fApi->_dut->testAllPixels(false);
	if (fParMask == 1) {
		for (int i=0;i<52;i++) {
			fApi->_dut->maskPixel(i,0,true);
			fApi->_dut->maskPixel(i,79,true);
		}
		for (int i=0;i<80;i++) {
			fApi->_dut->maskPixel(0,i,true);
			fApi->_dut->maskPixel(51,i,true);
		}
	}
	
  // mask all pixels from mask file!!!
  maskPixels();
	
	pair<vector <TH2D*>,vector <TH2D*> > maps = getdata();

	vector <TH2D*> h_alive = maps.first;
	vector <TH2D*> h_hitmap = maps.second;

    vector<int> deadPixel(nrocs, 0);
    vector<int> probPixel(nrocs, 0);
    vector<int> xHits(nrocs,0);
    vector<int> fidHits(nrocs,0);
    vector<int> allHits(nrocs,0);
    vector<int> fidPixels(nrocs,0);
 TH1D *h1(0), *h2(0);
	for (int r=0; r<nrocs; r++) {
	h1 = bookTH1D(Form("HR_Overall_Efficiency_C%d", getIdFromIdx(r)),  Form("HR_Overall_Efficiency_C%d", getIdFromIdx(r)),  201, 0., 1.005);
    fHistList.push_back(h1);
    h2 = bookTH1D(Form("HR_Fiducial_Efficiency_C%d", getIdFromIdx(r)),  Form("HR_Fiducial_Efficiency_C%d", getIdFromIdx(r)),  201, 0., 1.005);
    fHistList.push_back(h2);


	for (int ix = 0; ix < h_alive[r]->GetNbinsX(); ++ix) {
    for (int iy = 0; iy < h_alive[r]->GetNbinsY(); ++iy) {
      allHits[r] += static_cast<int>(h_alive[r]->GetBinContent(ix+1, iy+1));
      h1->Fill(h_alive[r]->GetBinContent(ix+1, iy+1)/fParNtrig);
  		if ((ix > 0) && (ix < 51) && (iy < 79) && (h_alive[r]->GetBinContent(ix+1, iy+1) > 0)) {
    		fidHits[r] += static_cast<int>(h_alive[r]->GetBinContent(ix+1, iy+1));
    		++fidPixels[r];
  		h2->Fill(h_alive[r]->GetBinContent(ix+1, iy+1)/fParNtrig);
  		}
  		// -- count dead pixels
 		  if (h_alive[r]->GetBinContent(ix+1, iy+1) < fParNtrig) {
    		++probPixel[r];
    		if (h_alive[r]->GetBinContent(ix+1, iy+1) < 1) {
      		++deadPixel[r];
    		}
  		}
  		// -- Count X-ray hits detected
  		if (h_hitmap[r]->GetBinContent(ix+1,iy+1)>0){
    		xHits[r] += static_cast<int> (h_hitmap[r]->GetBinContent(ix+1,iy+1));
  		}
    }
  }
}




	

	for (int r=0; r<nrocs; r++) {
	fHistList.push_back(h_hitmap[r]);
	fHistList.push_back(h_alive[r]);
	fHistOptions.insert(make_pair(h_hitmap[r], "colz"));
	fHistOptions.insert(make_pair(h_alive[r], "colz"));
	h_alive[r]->Draw("COLZ");
	}
	fDisplayedHist = find(fHistList.begin(), fHistList.end(), h_alive[nrocs-1]);
	

	if (fProblem) {
		LOG(logINFO) << "PixTestXPixelAlive2::doTest() aborted because of problem ";
		return;
	}

	double sensorArea = 0.015 * 0.010 * 54 * 81; // in cm^2, accounting for larger edge pixels (J. Hoss 2014/10/21)
	if (fParMask == 1) {
		sensorArea = 0.015 * 0.010 * 50 * 78;
	}
	string deadPixelString, probPixelString, xHitsString, numTrigsString,
	fidCalHitsString, allCalHitsString,
	fidCalEfficiencyString, allCalEfficiencyString,
	xRayRateString;
	for (unsigned int i = 0; i < probPixel.size(); ++i) {
		float fidefficiency = fidHits[i]/static_cast<double>(fidPixels[i]*fParNtrig);
		probPixelString += Form(" %4d", probPixel[i]);
		deadPixelString += Form(" %4d", deadPixel[i]);
		xHitsString     += Form(" %4d", xHits[i]);
		allCalHitsString += Form(" %4d", allHits[i]);
		fidCalHitsString += Form(" %4d", fidHits[i]);
		int numTrigs = fParNtrig * 4160;
		numTrigsString += Form(" %4d", numTrigs );
		fidCalEfficiencyString += Form(" %.3f", fidHits[i]/static_cast<double>(fidPixels[i]*fParNtrig)*100);
		allCalEfficiencyString += Form(" %.3f", allHits[i]/static_cast<double>(numTrigs)*100);
		xRayRateString += Form(" %.1f", xHits[i]/static_cast<double>(numTrigs)/25./sensorArea*1000./fidefficiency);
	}

	int seconds = t.RealTime(); 
	LOG(logINFO) << "number of dead pixels (per ROC): " << deadPixelString;
	LOG(logINFO) << "number of red-efficiency pixels: " << probPixelString;
	LOG(logINFO) << "number of X-ray hits detected:   " << xHitsString;
	LOG(logINFO) << "number of triggers sent (total per ROC): " << numTrigsString;
	LOG(logINFO) << "number of Vcal hits detected: " << allCalHitsString;
	LOG(logINFO) << "Vcal hit fiducial efficiency (%): " << fidCalEfficiencyString;
	LOG(logINFO) << "Vcal hit overall efficiency (%): " << allCalEfficiencyString;
	LOG(logINFO) << "X-ray hit rate [MHz/cm2]: " <<  xRayRateString;
	LOG(logINFO) << "XPixelAlive2::doTest() done, duration: " << seconds << " seconds";

	PixTest::update(); 
}












void PixTestXPixelAlive2::doCalDelScan() {
ConfigParameters* config = fPixSetup->getConfigParameters();

	TStopwatch t;

	vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
        unsigned nrocs = rocIds.size();

	fDirectory->cd();
	PixTest::update(); 
	bigBanner(Form("PixTestXPixelAlive2::doTest()"));

	fProblem = false;
	gStyle->SetPalette(1);
	bool verbose(false);
	cacheDacs(verbose);
	fDirectory->cd();
	PixTest::update(); 

	  // -- cache triggerdelay
  vector<pair<string, uint8_t> > oldDelays = fPixSetup->getConfigParameters()->getTbSigDelays();
  bool foundIt(false);
  for (unsigned int i = 0; i < oldDelays.size(); ++i) {
    if (oldDelays[i].first == "triggerdelay") {
      foundIt = true;
    }
    LOG(logDEBUG) << " old set: " << oldDelays[i].first << ": " << (int)oldDelays[i].second;
  }

  vector<pair<string, uint8_t> > delays = fPixSetup->getConfigParameters()->getTbSigDelays();
  if (!foundIt) {
    delays.push_back(make_pair("triggerdelay", 20));
    oldDelays.push_back(make_pair("triggerdelay", 0));
  } else {
    for (unsigned int i = 0; i < delays.size(); ++i) {
      if (delays[i].first == "triggerdelay") {
	delays[i].second = 20;
      }
    }
  }

  //fApi->setTbmReg("delays",0x40);

  for (unsigned int i = 0; i < delays.size(); ++i) {
    LOG(logDEBUG) << " setting: " << delays[i].first << ": " << (int)delays[i].second;
  }
  fApi->setTestboardDelays(delays);
	

if (fParReset == 0) {
  fPg_setup.clear();
  vector<pair<string, uint8_t> > pgtmp = fPixSetup->getConfigParameters()->getTbPgSettings();
  for (unsigned i = 0; i < pgtmp.size(); ++i) {
    if (string::npos != pgtmp[i].first.find("resetroc")) continue;
    if (string::npos != pgtmp[i].first.find("resettbm")) continue;
    fPg_setup.push_back(pgtmp[i]);
  }
  if (0) for (unsigned int i = 0; i < fPg_setup.size(); ++i) cout << fPg_setup[i].first << ": " << (int)fPg_setup[i].second << endl;

  fApi->setPatternGenerator(fPg_setup);
}
	fApi->_dut->maskAllPixels(false);

	fApi->_dut->testAllPixels(false);
	if (fParMask == 1) {
		for (int i=0;i<52;i++) {
			fApi->_dut->maskPixel(i,0,true);
			fApi->_dut->maskPixel(i,79,true);
		}
		for (int i=0;i<80;i++) {
			fApi->_dut->maskPixel(0,i,true);
			fApi->_dut->maskPixel(51,i,true);
		}
	}
	int ip = 0;
  fPIX.clear();
  fPIX.push_back(make_pair(11,20));
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, true);
  fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, false);

  bool done = false;
  vector<pair<uint8_t, vector<pixel> > >  results;
  while (!done) {
    results.clear();
    int cnt(0);
    try{
      results = fApi->getEfficiencyVsDAC("caldel", 0, 255, FLAG_FORCE_MASKED, 3);
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
  fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, false);
  fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, true);
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);


  TH1D *h1(0);
  vector<TH1D*> maps;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h1 = bookTH1D(Form("hrCalDelScan_C%d", rocIds[iroc]), Form("hrCalDelScan_C%d", rocIds[iroc]), 256, 0., 256.);
    h1->SetMinimum(0.);
    h1->SetDirectory(fDirectory);
    fHistList.push_back(h1);
    maps.push_back(h1);
  }

  int idx(-1);
  for (unsigned int i = 0; i < results.size(); ++i) {
    int caldel = results[i].first;
    vector<pixel> pixels = results[i].second;
    for (unsigned int ipix = 0; ipix < pixels.size(); ++ipix) {
      idx = getIdxFromId(pixels[ipix].roc());
      h1 = maps[idx];
      if (h1) {
	h1->Fill(caldel, pixels[ipix].value());
      } else {
	LOG(logDEBUG) << "no histogram found for ROC " << pixels[ipix].roc() << " with index " << idx;
      }
    }
  }

  vector<int> calDelLo(rocIds.size(), -1);
  vector<int> calDelHi(rocIds.size(), -1);
  int DeltaCalDelMax(-1), reserve(1);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    double cdMax   = maps[iroc]->GetMaximum();
    calDelLo[iroc] = static_cast<int>(maps[iroc]->GetBinLowEdge(maps[iroc]->FindFirstBinAbove(0.8*cdMax) + reserve));
    calDelHi[iroc] = static_cast<int>(maps[iroc]->GetBinLowEdge(maps[iroc]->FindLastBinAbove(0.8*cdMax) - reserve));
    if (calDelHi[iroc] - calDelLo[iroc] > DeltaCalDelMax) {
      DeltaCalDelMax = calDelHi[iroc] - calDelLo[iroc];
    }
  }

vector<pair<int, double> > calDelMax(rocIds.size(), make_pair(-1, -1)); // (CalDel,meanEff)
  vector<TH1D*> calDelEffHist;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h1 = bookTH1D(Form("HR_CalDelScan_eff_C%d", getIdFromIdx(iroc)),
		  Form("HR_CalDelScan_eff_C%d", getIdFromIdx(iroc)), 256, 0., 256);
    calDelEffHist.push_back(h1);
  }

  int nsteps(10);
  for (int istep = 0; istep < nsteps; ++istep) {

    // -- set CalDel per ROC
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      int caldel = calDelLo[iroc] + istep*(calDelHi[iroc]-calDelLo[iroc])/(nsteps-1);
      fApi->setDAC("CalDel", caldel, rocIds[iroc]);
    }

    LOG(logINFO) << " step " << (istep+1) << " of " << (nsteps) << ", expecting " << 4160*fParNtrig << " events";
    pair<vector <TH2D*>,vector <TH2D*> > maps = getdata();

    vector <TH2D*> test2 = maps.first;
    for (unsigned int iroc = 0; iroc < test2.size(); ++iroc) {
      fHistOptions.insert(make_pair(test2[iroc], "colz"));
      h1 = bookTH1D(Form("HR_CalDelScan_step%d_C%d", istep, getIdFromIdx(iroc)),
        Form("HR_CalDelScan_step%d_C%d", istep, getIdFromIdx(iroc)),  201, 0., 1.005);
      fHistList.push_back(h1);
      for (int ix = 0; ix < test2[iroc]->GetNbinsX(); ++ix) {
        for (int iy = 0; iy < test2[iroc]->GetNbinsY(); ++iy) {
          h1->Fill(test2[iroc]->GetBinContent(ix+1, iy+1)/fParNtrig);
        }
      }
      int caldel = fApi->_dut->getDAC(rocIds[iroc], "CalDel");
      calDelEffHist[iroc]->SetBinContent(caldel, h1->GetMean());

      if (h1->GetMean() > calDelMax[iroc].second) {
        calDelMax[iroc].first  = caldel;
        calDelMax[iroc].second = h1->GetMean();
      }
    }

    for (unsigned int i = 0; i < test2.size(); ++i) {
      delete test2[i];
    }
    test2.clear();
  }


  restoreDacs();
  for (unsigned int i = 0; i < calDelMax.size(); ++i) {
    LOG(logDEBUG) << "roc " << Form("%2d", i) << ": caldel = " << calDelMax[i].first << " eff = " << calDelMax[i].second;
    fApi->setDAC("CalDel", calDelMax[i].first, rocIds[i]);
  }

  copy(calDelEffHist.begin(), calDelEffHist.end(), back_inserter(fHistList));

  calDelEffHist[0]->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), calDelEffHist[0]);
  PixTest::update();


}









void PixTestXPixelAlive2::doXNoiseMaps() {
  ConfigParameters* config = fPixSetup->getConfigParameters();

	TStopwatch t;

	vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  unsigned nrocs = rocIds.size();

	fDirectory->cd();
	PixTest::update(); 
	bigBanner(Form("PixTestXPixelAlive2::doTest()"));
  LOG(logINFO) << " Vcal scanned from " << fParScurveLow << " to " << fParScurveHigh << ", ntrig: " << fParNtrig;

	fProblem = false;
	gStyle->SetPalette(1);
	bool verbose(false);
	cacheDacs(verbose);
	fDirectory->cd();
	PixTest::update(); 

	// -- cache triggerdelay
  vector<pair<string, uint8_t> > oldDelays = fPixSetup->getConfigParameters()->getTbSigDelays();
  bool foundIt(false);
  for (unsigned int i = 0; i < oldDelays.size(); ++i) {
    if (oldDelays[i].first == "triggerdelay") {
      foundIt = true;
    }
    LOG(logDEBUG) << " old set: " << oldDelays[i].first << ": " << (int)oldDelays[i].second;
  }

  vector<pair<string, uint8_t> > delays = fPixSetup->getConfigParameters()->getTbSigDelays();
  if (!foundIt) {
    delays.push_back(make_pair("triggerdelay", 20));
    oldDelays.push_back(make_pair("triggerdelay", 0));
  } 
  else {
    for (unsigned int i = 0; i < delays.size(); ++i) {
      if (delays[i].first == "triggerdelay") {
	      delays[i].second = 20;
      }
    }
  }

 

  for (unsigned int i = 0; i < delays.size(); ++i) {
    LOG(logDEBUG) << " setting: " << delays[i].first << ": " << (int)delays[i].second;
  }
  fApi->setTestboardDelays(delays);
	

  if (fParReset == 0) {
    fPg_setup.clear();
    vector<pair<string, uint8_t> > pgtmp = fPixSetup->getConfigParameters()->getTbPgSettings();
    for (unsigned i = 0; i < pgtmp.size(); ++i) {
      if (string::npos != pgtmp[i].first.find("resetroc")) continue;
      if (string::npos != pgtmp[i].first.find("resettbm")) continue;
      fPg_setup.push_back(pgtmp[i]);
    }
    if (0) for (unsigned int i = 0; i < fPg_setup.size(); ++i) cout << fPg_setup[i].first << ": " << (int)fPg_setup[i].second << endl;

    fApi->setPatternGenerator(fPg_setup);
  }
	fApi->_dut->maskAllPixels(false);
	fApi->_dut->testAllPixels(false);

  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  for (unsigned int i = 0; i < fHotPixels.size(); ++i) {
    vector<pair<int, int> > hot = fHotPixels[i];
    for (unsigned int ipix = 0; ipix < hot.size(); ++ipix) {
      LOG(logINFO) << "ROC " << getIdFromIdx(i) << " masking hot pixel " << hot[ipix].first << "/" << hot[ipix].second;
      fApi->_dut->maskPixel(hot[ipix].first, hot[ipix].second, true, getIdFromIdx(i));
    }
  }
  maskPixels();

 if (fParMask == 1) {
		for (int i=0;i<52;i++) {
			fApi->_dut->maskPixel(i,0,true);
			fApi->_dut->maskPixel(i,79,true);
		}
		for (int i=0;i<80;i++) {
			fApi->_dut->maskPixel(0,i,true);
			fApi->_dut->maskPixel(51,i,true);
		}
	}

  int res(0xf);
  fOutputFilename = "XSCurveData";

  vector < vector < vector <shist256*> > > scurves;

  /*for (unsigned int iroc = 0; iroc < nrocs; ++iroc) {
	  vector < vector <shist256*> > rocs;
	  scurves.push_back(rocs);
	  for (unsigned int icol = 0; icol < 52; ++icol) {
		  vector <shist256*> cols;
		  scurves[iroc].push_back(cols);
		  for (unsigned int irow = 0; irow < 52; ++irow) {
			  shist256* s;
			  scurves[iroc][icol].push_back(s);
      			//scurves[iroc][icol].push_back(bookTH1D(Form("Vcal_col%d_row%d_C%d", icol, irow, getIdFromIdx(iroc)), Form("Vcal_col%d_row%d_C%d", icol, irow, getIdFromIdx(iroc)),  256, 0, 256));
		  }
	  }
  }*/
  /*shist256* m;
  m->clear();
  m->fill(3,25.);*/

  vector<shist256*>  maps;
  vector<TH1*>       resultMaps;
  resultMaps.clear();

  shist256 *pshistBlock  = new (fPixSetup->fPxarMemory) shist256[16*52*80];
  shist256 *ph;

  int idx(0);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    for (unsigned int ic = 0; ic < 52; ++ic) {
      for (unsigned int ir = 0; ir < 80; ++ir) {
        idx = PixUtil::rcr2idx(iroc, ic, ir);
        ph = pshistBlock + idx;
        maps.push_back(ph);
      }
    }
  }

  vector<TH2D*> test3;
  //vector<TH1*> test2;
  for (unsigned int iroc = 0; iroc < nrocs; ++iroc) {
	  test3.push_back(bookTH2D(Form("vcal_xraymap_C%d", getIdFromIdx(iroc)), Form("vcal_xraymap_C%d", getIdFromIdx(iroc)), 52,0,52,80,0,80));
	  fHistOptions.insert(make_pair(test3[iroc], "colz"));
	}

  int nsteps =0;
  for (int istep = fParScurveLow; istep <= fParScurveHigh; ++istep) {
    LOG(logINFO) << " step " << (istep-fParScurveLow+1) << " of " << (fParScurveHigh-fParScurveLow+1) << ", Vcal = " << istep << ", expecting " << 4160*fParNtrig << " events";
    nsteps++;
    // -- set Vcal per ROC
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      fApi->setDAC("Vcal", istep, rocIds[iroc]);
    }

	  pair<vector <TH2D*>,vector <TH2D*> > maps2 = getdata();

	  vector <TH2D*> test = maps2.first;
	  vector <TH2D*> xray = maps2.second;
    for (unsigned int iroc = 0; iroc < test.size(); ++iroc) {
      for (int ix = 0; ix < test[iroc]->GetNbinsX(); ++ix) {
	      for (int iy = 0; iy < test[iroc]->GetNbinsY(); ++iy) {
		int idx = PixUtil::rcr2idx(getIdxFromId(iroc), ix, iy);
	        maps[idx]->fill(istep,test[iroc]->GetBinContent(ix+1, iy+1));
	        test3[iroc]->Fill(ix,iy,xray[iroc]->GetBinContent(ix+1, iy+1));
		//cout << xray[iroc]->GetBinContent(ix+1, iy+1) << endl;
	      }
      }
    }

  }  
  int results(0xf);


  scurveAna(maps,resultMaps,res);
	

  vector<TH1*> test2 = resultMaps;
  //vector<TH2D*> test3 = getXrayMaps();


  copy(test3.begin(), test3.end(), back_inserter(fHistList));

  TH2D *h = (TH2D*)(fHistList.back());
  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update();

  // -- reset sig delays
  fApi->setTestboardDelays(oldDelays);
  for (unsigned int i = 0; i < oldDelays.size(); ++i) {
    LOG(logDEBUG) << " resetting: " << oldDelays[i].first << ": " << (int)oldDelays[i].second;
  }
  finalCleanup();

  if (test2.size() < 1) {
    LOG(logERROR) << "no scurve result histograms received?!";
    return;

  }

  vector<int> xHits(test3.size(),0);
  string hname(""), scurvesMeanString(""), scurvesRmsString("");
  for (unsigned int i = 0; i < test2.size(); ++i) {
    if (!test2[i]) continue;
    hname = test2[i]->GetName();

    // -- skip sig_ and thn_ histograms
    if (string::npos == hname.find("dist_thr_")) continue;
    scurvesMeanString += Form("%6.2f ", test2[i]->GetMean());
    scurvesRmsString += Form("%6.2f ", test2[i]->GetRMS());

  }

  for (unsigned int i = 0; i < test3.size(); ++i) {
    for (int ix = 0; ix < test3[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test3[i]->GetNbinsY(); ++iy) {
        // -- Count X-ray hits detected
        if (test3[i]->GetBinContent(ix+1,iy+1)>0){
          xHits[i] += static_cast<int> (test3[i]->GetBinContent(ix+1,iy+1));
        }
      }
    }
  }

  double sensorArea = 0.015 * 0.010 * 54 * 81; // in cm^2, accounting for larger edge pixels (J. Hoss 2014/10/21)
	if (fParMask == 1) {
		sensorArea = 0.015 * 0.010 * 50 * 78;
	}

  // -- summary printout
  //  int nrocs = fApi->_dut->getNEnabledRocs();
  string xHitsString, numTrigsString, xRayRateString;
  for (unsigned int i = 0; i < test3.size(); ++i) {
    int numTrigs = fParNtrig * 4160 * nsteps;
    xHitsString     += Form(" %4d", xHits[i]);
    numTrigsString += Form(" %4d", numTrigs );
    xRayRateString += Form(" %.1f", xHits[i]/static_cast<double>(numTrigs)/25./sensorArea*1000.);
  }

  LOG(logINFO) << "vcal mean: " << scurvesMeanString;
  LOG(logINFO) << "vcal RMS:  " << scurvesRmsString;
  LOG(logINFO) << "number of X-ray hits detected:   " << xHitsString;
  LOG(logINFO) << "number of triggers sent (total per ROC): " << numTrigsString;
  LOG(logINFO) << "X-ray hit rate [MHz/cm2]: " <<  xRayRateString;
  LOG(logINFO) << "PixTestHighRate::doXNoiseMaps() done";




}













pair<vector <TH2D*>,vector <TH2D*> > PixTestXPixelAlive2::getdata() {

  fApi->_dut->maskPixel(0,79, true);
  fApi->_dut->maskPixel(51,79, true);
  
	vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
        unsigned nrocs = rocIds.size();

	vector <TH2D*> h_alive;
	vector <TH2D*> h_hitmap;

	for (int i=0; i<nrocs; i++) {
	h_alive.push_back(bookTH2D(Form("highRate_C%d", getIdFromIdx(i)), Form("highRate_C%d", getIdFromIdx(i)), 52,0,52,80,0,80));
	h_hitmap.push_back(bookTH2D(Form("highRate_xraymap_C%d", getIdFromIdx(i)), Form("highRate_xraymap_C%d", getIdFromIdx(i)), 52,0,52,80,0,80));

	}

  vector<pxar::Event> daqdat;
  vector<pxar::Event> daqdat0;
  uint8_t perFull;
	//fApi->flushTestboard();

  	fApi->_dut->testAllPixels(false);

	fApi->daqSingleSignal("resetroc");
	fApi->daqStart(FLAG_DUMP_FLAWED_EVENTS);

	for (int co = 0; co<52; co++) {
		for (int ro = 0; ro<80; ro++) {
			fApi->_dut->testPixel(co,ro,true);
			fApi->SetCalibrateBits(true);

			fApi->daqTrigger(fParNtrig, 1000);

			fApi->_dut->testPixel(co,ro,false);
			fApi->SetCalibrateBits(false);
		}
    fApi->daqStatus(perFull);
    if (perFull > 80) {
      LOG(logINFO) << "pausing triggers to readout";
      try { 
        daqdat0 = fApi->daqGetEventBuffer(); 
        std::copy (daqdat0.begin(), daqdat0.end(), std::back_inserter(daqdat));
      } catch(pxar::DataNoEvent &) {
        LOG(logERROR) << "no data";
      }
      LOG(logINFO) << "resuming triggers";
    }
	}


	fApi->daqStop();

  try { 
    daqdat0 = fApi->daqGetEventBuffer(); 
    std::copy (daqdat0.begin(), daqdat0.end(), std::back_inserter(daqdat));
  } catch(pxar::DataNoEvent &) {
    LOG(logERROR) << "no data";
  }


	int EventId=0;

	for (std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
		int ro = (EventId/fParNtrig)%80;
		int co = (EventId/fParNtrig)/80;
	std::stringstream ss("");
	ss << "Event " << EventId << ": ";
		for (unsigned int i=0;i<it->pixels.size();i++) {
			/*
			if (it->pixels[i].row() != 20 || it->pixels[i].column() != 11) {
				ss << "[";
			}
			ss << (int)it->pixels[i].column() << "," << (int)it->pixels[i].row() << ": " << it->pixels[i].value();
			if (it->pixels[i].row() != 20 || it->pixels[i].column() != 11) {
				ss << "]";
			}*/
			int rocid = it->pixels[i].roc();
			int rocidx = getIdxFromId(rocid);
			if (it->pixels[i].row() == ro && it->pixels[i].column() == co) {
				h_alive[rocidx]->Fill(co,ro,1);
				
			} else {
				h_hitmap[rocidx]->Fill(it->pixels[i].column(), it->pixels[i].row(), 1);
			}
		}

	EventId++;


	}

return make_pair(h_alive,h_hitmap);

}

void PixTestXPixelAlive2::scurveAna(vector<shist256*> maps, vector<TH1*> & resultMaps, int result) {

 fDirectory->cd();
  TH1* h2(0), *h3(0), *h4(0);
  //  string fname("SCurveData");
  ofstream OutputFile;
  string line;
  string empty("32  93   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0 ");
  bool dumpFile(false);
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  int roc(0), ic(0), ir(0);
  TH1D *h1 = new TH1D("h1", "h1", 256, 0., 256.); h1->Sumw2();


    string dac = "vcal";
    string name = "xNoiseMaps";
   
for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    LOG(logDEBUG) << "analyzing ROC " << static_cast<int>(rocIds[iroc]);
    h2 = bookTH2D(Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]),
                  Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]),
                  52, 0., 52., 80, 0., 80.);
    fHistOptions.insert(make_pair(h2, "colz"));

    h3 = bookTH2D(Form("sig_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]),
                  Form("sig_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]),
                  52, 0., 52., 80, 0., 80.);
    fHistOptions.insert(make_pair(h3, "colz"));

    h4 = bookTH2D(Form("thn_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]),
                  Form("thn_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]),
                  52, 0., 52., 80, 0., 80.);
    fHistOptions.insert(make_pair(h4, "colz"));

    string lname(name);
    transform(lname.begin(), lname.end(), lname.begin(), ::tolower);
    //    if (!name.compare("scurveVcal") || !lname.compare("scurvevcal")) {
    if (fOutputFilename != "") {
      dumpFile = true;
      LOG(logINFO) << "dumping ASCII scurve output file: " << fOutputFilename;
      OutputFile.open(Form("%s/%s_C%d.dat", fPixSetup->getConfigParameters()->getDirectory().c_str(), fOutputFilename.c_str(), iroc));
      OutputFile << "Mode 1 " << "Ntrig " << fParNtrig << endl;
    }

    for (unsigned int i = iroc*4160; i < (iroc+1)*4160; ++i) {
      PixUtil::idx2rcr(i, roc, ic, ir);
      if (maps[i]->getSumOfWeights() < 1) {
        if (dumpFile) OutputFile << empty << endl;
        continue;
      }
      // -- calculated "proper" errors
      h1->Reset();
      for (int ib = 1; ib <= 256; ++ib) {
        h1->SetBinContent(ib, maps[i]->get(ib));
        h1->SetBinError(ib, fParNtrig*PixUtil::dBinomial(static_cast<int>(maps[i]->get(ib)), fParNtrig));
      }

      bool ok = threshold(h1);
      if (((result & 0x10) && !ok) || (result & 0x20)) {
        TH1D *h1c = (TH1D*)h1->Clone(Form("scurve_%s_c%d_r%d_C%d", dac.c_str(), ic, ir, rocIds[iroc]));
        if (!ok) {
          h1c->SetTitle(Form("problematic %s scurve (c%d_r%d_C%d), thr = %4.3f", dac.c_str(), ic, ir, rocIds[iroc], fThreshold));
        } else {
          h1c->SetTitle(Form("%s scurve (c%d_r%d_C%d), thr = %4.3f", dac.c_str(), ic, ir, rocIds[iroc], fThreshold));
        }
        fHistList.push_back(h1c);
      }
      h2->SetBinContent(ic+1, ir+1, fThreshold);
      h2->SetBinError(ic+1, ir+1, fThresholdE);

      h3->SetBinContent(ic+1, ir+1, fSigma);
      h3->SetBinError(ic+1, ir+1, fSigmaE);

      h4->SetBinContent(ic+1, ir+1, fThresholdN);

      // -- write file
      if (dumpFile) {
        int NSAMPLES(32);
        int ibin = h1->FindBin(fThreshold);
        int bmin = ibin - 15;
        line = Form("%2d %3d", NSAMPLES, bmin);
        for (int ix = bmin; ix < bmin + NSAMPLES; ++ix) {
          line += string(Form(" %3d", static_cast<int>(h1->GetBinContent(ix+1))));
        }
        OutputFile << line << endl;
      }
    }
    if (dumpFile) OutputFile.close();

    if (result & 0x1) {
      resultMaps.push_back(h2);
      fHistList.push_back(h2);
    }
    if (result & 0x2) {
      resultMaps.push_back(h3);
      fHistList.push_back(h3);
    }
    if (result & 0x4) {
      resultMaps.push_back(h4);
      fHistList.push_back(h4);
    }

    if (result & 0x8) {
      if (result & 0x1) {
        TH1* d1 = distribution((TH2D*)h2, 256, 0., 256.);
        resultMaps.push_back(d1);
        fHistList.push_back(d1);
      }
      if (result & 0x2) {
        TH1* d2 = distribution((TH2D*)h3, 100, 0., 6.);
        resultMaps.push_back(d2);
        fHistList.push_back(d2);
      }
      if (result & 0x4) {
        TH1* d3 = distribution((TH2D*)h4, 256, 0., 256.);
        resultMaps.push_back(d3);
        fHistList.push_back(d3);
      }
    }

  }

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);

  delete h1;

  if (h2) h2->Draw("colz");

}

bool PixTestXPixelAlive2::threshold(TH1 *h) {

  TF1 *f = fPIF->errScurve(h);

  double lo, hi;
  f->GetRange(lo, hi);

  fThresholdN = h->FindLastBinAbove(0.5*h->GetMaximum());

  if (fPIF->doNotFit()) {
    fThreshold  = f->GetParameter(0);
    fThresholdE = 0.3;
    fSigma      = 0.;
    fSigmaE     = 0.;
    return false;
  } else {
    h->Fit(f, "qr", "", lo, hi);
    fThreshold  = f->GetParameter(0);
    fThresholdE = f->GetParError(0);
    fSigma      = 1./(TMath::Sqrt(2.)/f->GetParameter(1));
    fSigmaE     = fSigma * f->GetParError(1) / f->GetParameter(1);
  }

  //  cout << "fit status: " << gMinuit->GetStatus()  << endl;


  if (fThreshold < h->GetBinLowEdge(1)) {
    fThreshold  = -2.;
    fThresholdE = -2.;
    fSigma  = -2.;
    fSigmaE = -2.;
    fThresholdN = -2.;
    return false;
  }

  if (fThreshold > h->GetBinLowEdge(h->GetNbinsX())) {
    fThreshold  = h->GetBinLowEdge(h->GetNbinsX());
    fThresholdE = -1.;
    fSigma  = -1.;
    fSigmaE = -1.;
    fThresholdN = fThreshold;
    return false;
  }

  return true;
}


void PixTestXPixelAlive2::increaseEdgePixelThreshold() {
  PixTest::update();

  ConfigParameters* cp = fPixSetup->getConfigParameters();
  vector<vector<pixelConfig> > rocPixelConfig = cp->getRocPixelConfig();

  for (unsigned int rocIdx = 0; rocIdx < rocPixelConfig.size(); ++rocIdx) {

          for(size_t pixelIdx=0;pixelIdx<rocPixelConfig[rocIdx].size();pixelIdx++) {
            bool isHEdge = (rocPixelConfig[rocIdx][pixelIdx].column() % 51) == 0;
            bool isVEdge = rocPixelConfig[rocIdx][pixelIdx].row() == 79;

            if (isHEdge || isVEdge) {
              int trimBits = (int)(rocPixelConfig[rocIdx][pixelIdx].trim());
              trimBits += 8;
              if (isHEdge && isVEdge) {
                trimBits = 15;
              }
              if (trimBits > 15) {
                trimBits = 15;
              }

              rocPixelConfig[rocIdx][pixelIdx].setTrim(trimBits);
              bool result = fApi->_dut->updateTrimBits(rocPixelConfig[rocIdx][pixelIdx].column(), rocPixelConfig[rocIdx][pixelIdx].row(), trimBits, getIdFromIdx(rocIdx));

            }
          }
  }

  // enable all pixels, otherwise saveTrimBits() saves empty files
  fApi->_dut->testAllPixels(true);
  saveTrimBits();

}