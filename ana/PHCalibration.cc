#include "PHCalibration.hh"

#include <iostream>
#include <TMath.h>
#include <TH1.h>

using namespace std;

// ----------------------------------------------------------------------
PHCalibration::PHCalibration(int mode): fMode(mode) {

}


// ----------------------------------------------------------------------
PHCalibration::~PHCalibration() {

}

// ----------------------------------------------------------------------
double PHCalibration::vcal(int iroc, int icol, int irow, double ph) {
  if (0 == fMode) {
    return vcalErr(iroc, icol, irow, ph); 
  } else if (1 == fMode) {
    return vcalTanH(iroc, icol, irow, ph); 
  } else if (2 == fMode) {
    return vcalLUT(iroc, icol, irow, ph); 
  } else {
    return -99.;
  }
}

// ----------------------------------------------------------------------
double PHCalibration::ph(int iroc, int icol, int irow, double vcal) {
  if (0 == fMode) {
    return phErr(iroc, icol, irow, vcal);
  } else if (1 == fMode) {
    return phTanH(iroc, icol, irow, vcal);
  } else {
    return -99.;
  }

}

// ----------------------------------------------------------------------
double PHCalibration::vcalLUT(int iroc, int icol, int irow, double ph) {
  int idx = icol*80+irow; 

  cout << "fLUT size:" << fLUT.size() << endl;

  std::vector< std::pair< int, int> > pixLUT = fLUT[idx];

  double vcal_low = 256;
  double vcal_high = -1;
  double ph_low = 1.0;
  double ph_high = 1.0;

  if (ph < 0.1) {
    return -1;
  }

  cout << "LUT:" << icol << "/" << irow << ":" << ph  << endl;

  for (int j=0;j<pixLUT.size()-4;j++) {
    if ((double)pixLUT[j].second > ph && (double)pixLUT[j+1].second > ph && (double)pixLUT[j+2].second > ph) {
      vcal_high = pixLUT[j].first;
      cout << icol << "/" << irow << ":" << ph << " --- > upper bound = " <<  pixLUT[j].first << "/" << pixLUT[j].second << std::endl;
      ph_high = (double)pixLUT[j].second;
      break;
    }
  }

  for (int j=pixLUT.size()-1;j>=3;j--) {
    if ((double)pixLUT[j].second < ph && (double)pixLUT[j-1].second < ph && (double)pixLUT[j-2].second < ph) {
      vcal_low = pixLUT[j].first;
      cout << icol << "/" << irow << ":" << ph << " --- > lower bound = " <<  pixLUT[j].first << "/" << pixLUT[j].second << std::endl;
      ph_low = (double)pixLUT[j].second;
      break;
    }
  }

  double x;
  if (ph_high != ph_low) {
    x = vcal_low + (ph - ph_low) * (vcal_high - vcal_low)/(ph_high - ph_low);
  } else {
    x = ph_low;
  }
   cout << " ----- > " << x << endl;

  return x;
}

// ----------------------------------------------------------------------
double PHCalibration::vcalTanH(int iroc, int icol, int irow, double ph) {
  int idx = icol*80+irow; 
//   cout << "parameters: " << fParameters[iroc][idx].p0 << ", " << fParameters[iroc][idx].p1 
//        << ", " << fParameters[iroc][idx].p2 << ", " << fParameters[iroc][idx].p3 
//        << endl;
  double x = (TMath::ATanH((ph - fParameters[iroc][idx].p3)/fParameters[iroc][idx].p2) + fParameters[iroc][idx].p1)
    / fParameters[iroc][idx].p0;
  return x;
}

// ----------------------------------------------------------------------
double PHCalibration::phTanH(int iroc, int icol, int irow, double vcal) {
  int idx = icol*80+irow; 
//   cout << "parameters: " << fParameters[iroc][idx].p0 << ", " << fParameters[iroc][idx].p1 
//        << ", " << fParameters[iroc][idx].p2 << ", " << fParameters[iroc][idx].p3 
//        << endl;
  double x = fParameters[iroc][idx].p3 + fParameters[iroc][idx].p2 
    * TMath::TanH(fParameters[iroc][idx].p0 * vcal - fParameters[iroc][idx].p1);
  return x;
}

// ----------------------------------------------------------------------
double PHCalibration::vcalErr(int iroc, int icol, int irow, double ph) {
  int idx = icol*80+irow; 
  double arg = ph/fParameters[iroc][idx].p3 - fParameters[iroc][idx].p2;
//   cout << "parameters: " << fParameters[iroc][idx].p0 << ", " << fParameters[iroc][idx].p1 
//        << ", " << fParameters[iroc][idx].p2 << ", " << fParameters[iroc][idx].p3 
//        << endl;
//   cout << "fParameters[iroc][idx].p1 = " << fParameters[iroc][idx].p1 << endl;
//   cout << "arg: " << arg 
//        << " ph =  " << ph 
//        << " fParameters[iroc][idx].p3 = " << fParameters[iroc][idx].p3 
//        << " fParameters[iroc][idx].p2 = " <<  fParameters[iroc][idx].p2
//        << endl;
//   cout << "TMath::ErfInverse(arg) = " << TMath::ErfInverse(arg) << endl;
  double x = fParameters[iroc][idx].p0 + fParameters[iroc][idx].p1 * TMath::ErfInverse(arg);
  return x;
}

// ----------------------------------------------------------------------
double PHCalibration::phErr(int iroc, int icol, int irow, double vcal) {
  int idx = icol*80+irow; 
//   cout << "parameters: " << fParameters[iroc][idx].p0 << ", " << fParameters[iroc][idx].p1 
//        << ", " << fParameters[iroc][idx].p2 << ", " << fParameters[iroc][idx].p3 
//        << endl;
  double x = fParameters[iroc][idx].p3*(TMath::Erf((vcal - fParameters[iroc][idx].p0)/fParameters[iroc][idx].p1)
					+ fParameters[iroc][idx].p2);
  return x;
}

// ----------------------------------------------------------------------
void PHCalibration::setPHParameters(std::vector<std::vector<gainPedestalParameters> >v) {
  fParameters = v; 
} 

// ----------------------------------------------------------------------
void PHCalibration::setPHLUT(std::vector< std::vector< std::pair< int, int> > > v) {
  fLUT = v; 
} 

// ----------------------------------------------------------------------
string PHCalibration::getParameters(int iroc, int icol, int irow) {
  int idx = icol*80+irow; 
  return Form("%2d/%2d/%2d: %e %e %e %e", iroc, icol, irow, 
	      fParameters[iroc][idx].p0, fParameters[iroc][idx].p1, fParameters[iroc][idx].p2, fParameters[iroc][idx].p3);
}
