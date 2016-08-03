#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find

#include "PixTestPhRunLut.hh"
#include "log.h"
#include "TStopwatch.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestPhRunLut)

//------------------------------------------------------------------------------
PixTestPhRunLut::PixTestPhRunLut( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestPhRunLut::PixTestPhRunLut() : PixTest()
{
  //  LOG(logDEBUG) << "PixTestPhRunLut ctor()";
}

//------------------------------------------------------------------------------
bool PixTestPhRunLut::setParameter( string parName, string sval )
{
  bool found(false);

  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {

      found = true;
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end() );

      if( !parName.compare( "ntrig" ) ) {
	fParNtrig = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig
		     << "<- from sval = " << sval;
      }

      if( !parName.compare( "nevents" ) ) {
  fParNevents = atoi( sval.c_str() );
  LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig
         << "<- from sval = " << sval;
      }

      if( !parName.compare( "nloops" ) ) {
  fParNloops = atoi( sval.c_str() );
  LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig
         << "<- from sval = " << sval;
      }


      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestPhRunLut::init()
{
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestPhRunLut::setToolTips()
{
  fTestTip = string( "low rate data taking with online PH calibration");
  fSummaryTip = string("summary plot to be implemented");
}

//------------------------------------------------------------------------------
void PixTestPhRunLut::bookHist(string name) // general booking routine
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestPhRunLut::~PixTestPhRunLut()
{
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il ) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory( fDirectory );
    (*il)->Write();
  }
}

class PixPHLut {

public:
  PixPHLut() {
    lut.resize(256, std::vector<uint8_t>(256, (uint8_t)0));
  }

  // vector of std::pair<int (Vcal), int (ADC)>
  void fill(std::vector< std::pair<int, int> >);
  void dump();
  std::vector< double > get(int);

private:
  std::vector< std::vector< uint8_t > > lut;

};

void PixPHLut::fill(std::vector< std::pair<int, int> > data) {
  int dataLength = data.size();
  for (int i=0;i<dataLength;i++) {
    lut[data[i].first][data[i].second]++;
  }
}

std::vector< double > PixPHLut::get(int ADC) {
  std::vector<double> ret(256, 0);

  //normalization
  long sum=0;
  for (int i=0;i<256;i++) {
    sum += lut[i][ADC];
  }
  double norm = 0.0;
  if (sum > 0) {
    norm = 1.0/(double)sum;
  }

  //copy
  for (int i=0;i<256;i++) {
    ret[i] = lut[i][ADC] * norm;
  }
  return ret;
}

void PixPHLut::dump() {
  LOG(logINFO) << "PixPHLut::dump()";
  for (int i=0;i<256;i+=4) {
    std::cout << (int)i << ":";
    for (int j=0;j<256;j+=4) {
      std::cout << (int)(lut[i][j]+lut[i][j+1]+lut[i][j+2]+lut[i][j+3]) << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::flush;
}

//------------------------------------------------------------------------------
void PixTestPhRunLut::doTest()
{
  LOG(logINFO) << "PixTestPhRunLut::doTest() ";

  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  TStopwatch t;
  std::string name = "phRunLut";
  int totalPeriod = 1000;
  int nEvents = fParNevents;

  // get enabled rocs
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  unsigned nrocs = rocIds.size(); 

  //-- Sets up the histogram
  TH1D *h1(0);
  TH2D *h2(0);
  for (unsigned int iroc = 0; iroc < nrocs; ++iroc){
    h2 = bookTH2D(Form("hitMap_%s_C%d", name.c_str(), rocIds[iroc]), Form("hitMap_%s_C%d", name.c_str(), rocIds[iroc]), 
      52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);
    h2->SetDirectory(fDirectory);
    fHitMap.push_back(h2);

    h1 = bookTH1D(Form("charge_%s_C%d", name.c_str(), rocIds[iroc]), Form("charge_%s_C%d", name.c_str(), rocIds[iroc]), 1800, 0, 1800);
    h1->SetMinimum(0.);
    h1->SetDirectory(fDirectory);
    fCharge.push_back(h1);
  }
  copy(fHitMap.begin(), fHitMap.end(), back_inserter(fHistList));
  copy(fCharge.begin(), fCharge.end(), back_inserter(fHistList));


   // -- unmask entire chip and then mask hot pixels
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(false);
  int nMaskedPixels = 0;
  for (unsigned int i = 0; i < fHotPixels.size(); ++i) {
    vector<pair<int, int> > hot = fHotPixels[i]; 
    for (unsigned int ipix = 0; ipix < hot.size(); ++ipix) {
      fApi->_dut->maskPixel(hot[ipix].first, hot[ipix].second, true, getIdFromIdx(i)); 
      nMaskedPixels++;
    }
  }
  LOG(logINFO) << nMaskedPixels << " hot pixels are masked.";
  maskPixels(); 


  // test lut...

/*
  PixPHLut pixlut;
  LOG(logINFO) << "test 0a";
  std::vector< std::pair<int, int> > testData;
  LOG(logINFO) << "test 0b";

  testData.push_back( make_pair(3,1));
  testData.push_back( make_pair(3,2));
  testData.push_back( make_pair(3,2));
  testData.push_back( make_pair(3,3));
  testData.push_back( make_pair(3,3));
  testData.push_back( make_pair(3,3));
  testData.push_back( make_pair(3,3));
  testData.push_back( make_pair(3,3));
  testData.push_back( make_pair(3,4));
  testData.push_back( make_pair(3,7));
  testData.push_back( make_pair(3,9));

  testData.push_back( make_pair(4,3));
  testData.push_back( make_pair(4,4));
  testData.push_back( make_pair(4,4));
  testData.push_back( make_pair(4,4));
  testData.push_back( make_pair(4,4));
  testData.push_back( make_pair(4,4));
  testData.push_back( make_pair(4,4));
  testData.push_back( make_pair(4,4));
  testData.push_back( make_pair(4,4));
  testData.push_back( make_pair(4,5));
  testData.push_back( make_pair(4,8));
  testData.push_back( make_pair(4,9));

  testData.push_back( make_pair(5,4));
  testData.push_back( make_pair(5,5));
  testData.push_back( make_pair(5,5));
  testData.push_back( make_pair(5,5));
  testData.push_back( make_pair(5,5));
  testData.push_back( make_pair(5,6));

  testData.push_back( make_pair(6,4));
  testData.push_back( make_pair(6,4));
  testData.push_back( make_pair(6,4));
  testData.push_back( make_pair(6,6));
  testData.push_back( make_pair(6,5));

  testData.push_back( make_pair(7,6));
  testData.push_back( make_pair(7,7));
  testData.push_back( make_pair(7,7));
  testData.push_back( make_pair(7,7));
  testData.push_back( make_pair(7,7));
  testData.push_back( make_pair(7,7));
  testData.push_back( make_pair(7,7));
  testData.push_back( make_pair(7,8));

  testData.push_back( make_pair(8,7));
  testData.push_back( make_pair(8,8));
  testData.push_back( make_pair(8,8));
  testData.push_back( make_pair(8,8));
  testData.push_back( make_pair(8,8));
  testData.push_back( make_pair(8,8));
  testData.push_back( make_pair(8,9));
  testData.push_back( make_pair(8,10));

  LOG(logINFO) << "test 1";

  pixlut.fill(testData);

  LOG(logINFO) << "test 2";

  std::vector< double > returnData(256,0);
  returnData = pixlut.get(4);

  LOG(logINFO) << "test 3";

  LOG(logINFO) << "ADC=4";
  for (int i=0;i<returnData.size();i++) {
    LOG(logINFO) << returnData[i];
  }

  LOG(logINFO) << "test 4";
  */


  // DAQ loop
  int counter = 0;
  while (counter++ < fParNloops) {

    // set pattern generator
    fPg_setup.clear();
    fPg_setup.push_back(make_pair("trg", 50));
    fPg_setup.push_back(make_pair("tok", 0));
    fApi->setPatternGenerator(fPg_setup);

    LOG(logINFO) << "Pattern generator has been set";

    // start DAQ
    fApi->daqStart(FLAG_DUMP_FLAWED_EVENTS);

    // collect events
    LOG(logINFO) << "PixTestPhRunLut::collecting " << nEvents << " events..."; 
    t.Start(kTRUE);
    int finalPeriod = fApi->daqTrigger(nEvents, totalPeriod);
    fApi->daqStop();
    int seconds = t.RealTime();  
    LOG(logINFO) << "... took " << seconds << " seconds..."; 

    // read-out
    vector<pxar::Event> daqdat;
    std::vector< std::vector< float > > hits ( 52, std::vector<float> ( 80, 0 ) );
    std::vector< std::pair<int,int> > pixels;
    daqdat = fApi->daqGetEventBuffer();
    for (std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
      for (int j=0; j<it->pixels.size();  j++) {
        if (hits[it->pixels.at(j).column()][it->pixels.at(j).row()] == 0) {
          hits[it->pixels.at(j).column()][it->pixels.at(j).row()] = 1;
          pixels.push_back(std::make_pair(it->pixels.at(j).column(), it->pixels.at(j).row()));
        }
        LOG(logINFO) << "found a hit! " << (int)it->pixels.at(j).column() << "/" << (int)it->pixels.at(j).row(); 
      }
    }

    LOG(logINFO) << "PH calibration has to be done for " << pixels.size() << " pixels!";

    if (pixels.size() > 0) {
      // now create the big LUTs, (up to 260 MB per ROC!!! but for low rate should be much less)
      std::vector< PixPHLut > luts(pixels.size());

      // reset to default PG (with calibrate signals)
      pgToDefault();
      fApi->_dut->testAllPixels(false);

      // calibration run
      fApi->daqStart();
      int nTrig = fParNtrig;
      int nVcals = 256;

      vector<pxar::Event> caldat;
      vector<pxar::Event> caldatBuffer;

      for (int iPix=0;iPix<pixels.size();iPix++) {
        fApi->_dut->testPixel(pixels[iPix].first, pixels[iPix].second,true);
        fApi->SetCalibrateBits(true);

        for (int Vcal=0;Vcal<nVcals;Vcal++) {
          fApi->daqSingleSignal("resetroc");
          fApi->setDAC("vcal", Vcal);
          fApi->daqTrigger(nTrig, 2500);
        }

        fApi->_dut->testPixel(pixels[iPix].first, pixels[iPix].second,false);

        if (iPix % 10 == 0 && iPix > 0) {

          LOG(logINFO) << "stop DAQ"; 
          fApi->daqStop();
          try {
            caldatBuffer = fApi->daqGetEventBuffer();
            caldat.insert(caldat.end(), caldatBuffer.begin(), caldatBuffer.end());
          } catch(DataNoEvent) {}
          fApi->daqStart();
          LOG(logINFO) << "continue DAQ"; 

        }

      }
      fApi->daqStop();
      fApi->SetCalibrateBits(false);

      // calibration read-out
      LOG(logINFO) << "reading calibration data, expecting " << pixels.size()*nTrig*nVcals << " events..."; 
      try {
      caldatBuffer = fApi->daqGetEventBuffer();
      caldat.insert(caldat.end(), caldatBuffer.begin(), caldatBuffer.end());
       } catch(DataNoEvent) {}

      LOG(logINFO) << "... done."; 

      // process calibration data
      LOG(logINFO) << "Processing calibration data..."; 
      int iEvent = 0;
      for (std::vector<pxar::Event>::iterator it = caldat.begin(); it != caldat.end(); ++it) {
        int iPix = iEvent / (nTrig*nVcals);
        int Vcal = (iEvent % (nTrig*nVcals)) / nTrig;

        for (int i=0;i<it->pixels.size();i++) {

          // check for the tested pixel
          if (it->pixels[i].column() == pixels[iPix].first && it->pixels[i].row() == pixels[iPix].second) {

            // fill LUT
            std::vector< std::pair<int, int> > testData;
            testData.push_back( std::make_pair( Vcal, it->pixels[i].value() ) );
            luts[iPix].fill(testData);

          }
        }

        iEvent++;
      }

      luts[0].dump();
      // fill online PH calibrated charge histogram
      for (std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
        for (int j=0; j<it->pixels.size();  j++) {
          for (int k=0; k< pixels.size(); k++) {

            // search for pixel-specific LUT
            if (it->pixels[j].column() == pixels[k].first && it->pixels[j].row() == pixels[k].second ) {

              std::vector< double > returnData(256,0);
              returnData = luts[k].get( (int)it->pixels[j].value());

              double chargeMean = 0;

              /*
              for (int l=0;l<returnData.size();l++) {
                chargeMean += l *returnData[l];
              }
              fCharge[0]->Fill(chargeMean);
              */

              for (int l=0;l<returnData.size();l++) {
                fCharge[0]->Fill(l, returnData[l]);
              }

              break;
            }

          }

        }
      }
    } else {
      LOG(logINFO) << "no hits detected, skipping calibration!"; 
    }

    gSystem->ProcessEvents();
    fCharge[0]->GetXaxis()->SetRangeUser(0, 400);
    fCharge[0]->Draw();
    PixTest::update();
    
    gSystem->ProcessEvents();
  }


  fCharge[0]->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), fCharge[0]);
  PixTest::update();

  // reset pattern generator to default
  pgToDefault();
}
