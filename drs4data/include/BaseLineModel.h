/*
 * BaseLineModel.h
 *
 * Class BaseLineModel is a chi2 functor for fitting
 * the baseline residual difference.
 *
 *  Created on: May 2017
 *      Author: S. Lukic
 */

#ifndef BASELINEMODEL_H_
#define BASELINEMODEL_H_

#include "TH1F.h"
#include "TRandom3.h"
#include "TF1.h"
#include "TTree.h"
#include "TGraph.h"
#include <vector>


struct GridPoint {
public:
  GridPoint(double x, double y) :
    _x(x), _y(y) {}
  double _x;
  double _y;
};


class BaseLineModel {
public:
  BaseLineModel(const double *pars, TH1 *data, TH1F *model);
  ~BaseLineModel();

  // Minimization objective
  double operator()(const double * pars);

  void UpdateModel(const double *pars);

  int DataSize() const { return region1end-region1start + region2end-region2start; }

  static const int nPars = 1;

private:
  BaseLineModel();

  // Data
  const TH1 *_data;
  /* Model histogram */
  const TH1F *_model;

  double factor;

  const int region1start;
  const int region1end;
  const int region2start;
  const int region2end;

};



#endif /* BASELINEMODEL_H_ */
