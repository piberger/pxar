// -- author:Pirmin Berger 
// plot measured value of UT61E multimeter (USB) vs DAC

#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find

#include <TStopwatch.h>

#include "PixTestUT61EVsDac.hh"
#include "log.h"
#include "timer.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestUT61EVsDac)

//------------------------------------------------------------------------------
PixTestUT61EVsDac::PixTestUT61EVsDac( PixSetup *a, std::string name )
: PixTest(a, name), fParDAC("nada"), fParDACStep(1), fParDelay(500), fParFactor(1.0)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestUT61EVsDac::PixTestUT61EVsDac() : PixTest()
{
}

//------------------------------------------------------------------------------
bool PixTestUT61EVsDac::setParameter( string parName, string sval )
{
  bool found(false);

  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {
      found = true;
      sval.erase( remove( sval.begin(), sval.end(), ' ' ), sval.end() );

      if( !parName.compare( "dac" ) ) {
        fParDAC = sval;
        LOG(logDEBUG) << "PixTestUT61EVsDac setting fParDAC  ->" << fParDAC
          << "<- from sval = " << sval;
      }
      if( !parName.compare( "dacstep" ) ) {
        fParDACStep = atoi(sval.c_str());
        LOG(logDEBUG) << "PixTestUT61EVsDac setting fParDACStep  ->" << fParDACStep
          << "<- from sval = " << sval;
      }
      if( !parName.compare( "factor" ) ) {
        fParFactor = atof(sval.c_str());
        LOG(logDEBUG) << "PixTestUT61EVsDac setting fParFactor  ->" << fParFactor
          << "<- from sval = " << sval;
      }
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestUT61EVsDac::init()
{
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestUT61EVsDac::setToolTips()
{
  fTestTip = string( "measure voltages/currents vs DACDAC");
  fSummaryTip = string("summary plot to be implemented");
}


//------------------------------------------------------------------------------
PixTestUT61EVsDac::~PixTestUT61EVsDac()
{
  LOG(logDEBUG) << "PixTestUT61EVsDac dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logDEBUG) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestUT61EVsDac::doTest()
{
  fDirectory->cd();
  PixTest::update();
  cacheDacs();

  LOG(logINFO) << "PixTestUT61EVsDac::doTest() DACs = " << fParDAC;
  LOG(logINFO) << "This is only intended to work on single ROCs";

  char logmsg[1024];

  int range1 = fApi->getDACRange(fParDAC)+1;

  int nBinsX = int(range1/fParDACStep);
  TH1D* h2 = bookTH1D(Form("%s", fParDAC.c_str()), 
        Form("%s(x)_", fParDAC.c_str()), 
        nBinsX, 0., static_cast<double>(range1)); 

  fParDelay = 500;
  //for (int i=0;i<range1;i+=fParDACStep1) {
    //for (int j=0;j<range2;j+=fParDACStep2) {
  LOG(logINFO) << "reading " << nBinsX << " values, expected time: " << (float)(nBinsX / 60.0) << " min";
  for (int dacBin1=0;dacBin1<nBinsX;dacBin1++) {

      int dac1 = dacBin1*fParDACStep;

      LOG(logDEBUG) << fParDAC << "=" << dac1;
      fApi->setDAC(fParDAC, dac1, 0);
      timer t;
      while (static_cast<int>(t.get()) < fParDelay) {
        gSystem->ProcessEvents();
      }
      FILE *fp;
      int status;
      fp = popen("he2325u_pyusb.py -n 1", "r");
      if (fp == NULL) {
        LOG(logWARNING) << "can't run he2325u_pyusb.py";
      }

      while (fgets(logmsg, 1023, fp) != NULL) {
          string logString = logmsg;
          int pos1 = logString.find(';');
          if (pos1 != string::npos) {
            LOG(logINFO) << logString.substr(0, pos1);
            try {
              // units depend on volt meter settings
              double val = fParFactor * atoi(logString.substr(0, pos1).c_str());
              h2->SetBinContent(1+dacBin1, val);
            } catch (...) {
              LOG(logWARNING) << "can't convert UT61E measurement result '" << logString << "' to integer value.";
            }
          } else {
            LOG(logDEBUG) << logString;
          }
      }

      status = pclose(fp);
  }

  fHistList.push_back(h2);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  if (h2) h2->Draw(getHistOption(h2).c_str());
  PixTest::update(); 

  restoreDacs();
}
