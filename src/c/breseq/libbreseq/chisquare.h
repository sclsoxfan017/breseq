#ifndef _CHISQUARE_H_
#define _CHISQUARE_H_

#include "common.h"

namespace breseq {

  double gamma(double x);
  double pchisq(double df, double x);
  
	double chisquaredistribution(double v, double x);
	double incompletegamma(double a, double x, bool complemented = false);
	double lngamma(double x, double* sgngam);
  
  double nbdtrc(double k, double n, double p);
  double nbdtr(double k, double n, double p);
  double nbdtri(double k, double n, double p);
  
  double incbi(double aa, double bb, double yy0);
  double incbet(double aa, double bb, double xx);
  double incbcf(double a, double b, double x);
  double incbd(double a, double b, double x);
  double pseries(double a, double b, double x);
  
  double ndtri(double y0);
  
  double bdtrc(double k, double n, double p);
  double bdtr(double k, double n, double p);
  double bdtri(double k, double n, double y);

} // namespace breseq

#endif