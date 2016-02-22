// -- author:Pirmin Berger 
// plot measured value of UT61E multimeter (USB) vs 2 DACs

#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find

#include <TStopwatch.h>

#include "PixTestUT61EVsDacDac.hh"
#include "log.h"
#include "timer.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestUT61EVsDacDac)

//------------------------------------------------------------------------------
PixTestUT61EVsDacDac::PixTestUT61EVsDacDac( PixSetup *a, std::string name )
: PixTest(a, name), fParDAC1("nada"), fParDAC2("nada"), fParDACStep1(1), fParDACStep2(1), fParDelay(500), fParFactor(1.0)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestUT61EVsDacDac::PixTestUT61EVsDacDac() : PixTest()
{
}

//------------------------------------------------------------------------------
bool PixTestUT61EVsDacDac::setParameter( string parName, string sval )
{
  bool found(false);

  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {
      found = true;
      sval.erase( remove( sval.begin(), sval.end(), ' ' ), sval.end() );

      if( !parName.compare( "dac1" ) ) {
        fParDAC1 = sval;
        LOG(logDEBUG) << "PixTestUT61EVsDacDac setting fParDAC1  ->" << fParDAC1
          << "<- from sval = " << sval;
      }
      if( !parName.compare( "dac2" ) ) {
        fParDAC2 = sval;
        LOG(logDEBUG) << "PixTestUT61EVsDacDac setting fParDAC2  ->" << fParDAC2
          << "<- from sval = " << sval;
      }
      if( !parName.compare( "dacstep1" ) ) {
        fParDACStep1 = atoi(sval.c_str());
        LOG(logDEBUG) << "PixTestUT61EVsDacDac setting fParDACStep1  ->" << fParDACStep1
          << "<- from sval = " << sval;
      }
      if( !parName.compare( "dacstep2" ) ) {
        fParDACStep2 = atoi(sval.c_str());
        LOG(logDEBUG) << "PixTestUT61EVsDacDac setting fParDACStep2  ->" << fParDACStep2
          << "<- from sval = " << sval;
      }
      if( !parName.compare( "factor" ) ) {
        fParFactor = atof(sval.c_str());
        LOG(logDEBUG) << "PixTestUT61EVsDacDac setting fParFactor  ->" << fParFactor
          << "<- from sval = " << sval;
      }
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestUT61EVsDacDac::init()
{
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestUT61EVsDacDac::setToolTips()
{
  fTestTip = string( "measure voltages/currents vs DACDAC");
  fSummaryTip = string("summary plot to be implemented");
}


//------------------------------------------------------------------------------
PixTestUT61EVsDacDac::~PixTestUT61EVsDacDac()
{
  LOG(logDEBUG) << "PixTestUT61EVsDacDac dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logDEBUG) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestUT61EVsDacDac::doTest()
{
  fDirectory->cd();
  PixTest::update();
  cacheDacs();

  LOG(logINFO) << "PixTestUT61EVsDacDac::doTest() DACs = " << fParDAC1 << "/" << fParDAC2;
  LOG(logINFO) << "This is only intended to work on single ROCs";

  char logmsg[1024];

  int range1 = fApi->getDACRange(fParDAC1)+1;
  int range2 = fApi->getDACRange(fParDAC2)+1;

  int nBinsX = int(range1/fParDACStep1);
  int nBinsY = int(range2/fParDACStep2);
  TH2D* h2 = bookTH2D(Form("%s_%s_", fParDAC1.c_str(), fParDAC2.c_str()), 
        Form("%s(x)_%s(y)_", fParDAC1.c_str(), fParDAC2.c_str()), 
        nBinsX, 0., static_cast<double>(range1), nBinsY, 0., static_cast<double>(range2)); 

  fParDelay = 500;
  //for (int i=0;i<range1;i+=fParDACStep1) {
    //for (int j=0;j<range2;j+=fParDACStep2) {
  LOG(logINFO) << "reading " << nBinsX*nBinsY << " values, expected time: " << (float)(nBinsX*nBinsY / 60.0) << " min";
  for (int dacBin1=0;dacBin1<nBinsX;dacBin1++) {
    for (int dacBin2=0;dacBin2<nBinsY;dacBin2++) {

      int dac1 = dacBin1*fParDACStep1;
      int dac2 = dacBin2*fParDACStep2;

      LOG(logDEBUG) << fParDAC1 << "=" << dac1 << ", " << fParDAC2 << "=" << dac2;
      fApi->setDAC(fParDAC1, dac1, 0);
      fApi->setDAC(fParDAC2, dac2, 0);
      timer t;
      while (static_cast<int>(t.get()) < fParDelay) {
        gSystem->ProcessEvents();
      }
      FILE *fp;
      int status;
      fp = popen("he2325u_pyusb.py -n 1", "r");
      if (fp == NULL)
        LOG(logWARNING) << "can't run he2325u_pyusb.py";

      while (fgets(logmsg, 1023, fp) != NULL) {
          string logString = logmsg;
          int pos1 = logString.find(';');
          if (pos1 != string::npos) {
            LOG(logINFO) << logString.substr(0, pos1);
            try {
              // units depend on volt meter settings
              double val = fParFactor * atoi(logString.substr(0, pos1).c_str());
              h2->SetBinContent(1+dacBin1, 1+dacBin2, val);
            } catch (...) {
              LOG(logWARNING) << "can't convert UT61E measurement result '" << logString << "' to integer value.";
            }
          } else {
            LOG(logDEBUG) << logString;
          }
      }

      status = pclose(fp);
    }
  }

  fHistList.push_back(h2);
  fHistOptions.insert(make_pair(h2, "colz")); 

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  if (h2) h2->Draw(getHistOption(h2).c_str());
  PixTest::update(); 

  restoreDacs();
}
