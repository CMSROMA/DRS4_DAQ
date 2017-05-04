/*
 * BaseLineModel.cxx
 *
 * Class BaseLineModel is a chi2 functor useful for fitting
 * the radial shower profile with parametrisation functions.
 *
 *  Created on: Nov 25, 2016
 *      Author: strahinja
 */

#include <BaseLineModel.h>
#include "TMath.h"
#include "TList.h"

#include <cassert>


/************************************************************
 * Implementation of class BaseLineModel
 *
 ************************************************************/

BaseLineModel::BaseLineModel() :
    _data(NULL), _model(NULL), factor(0),
    region1start(3), region1end(180),
    region2start(700), region2end(900)
{
}


BaseLineModel::BaseLineModel(const double *pars, TH1F *data, TH1F *model) :
    _data(data), _model(model), factor(pars[0]),
    region1start(3), region1end(180),
    region2start(700), region2end(900)
{
}


BaseLineModel::~BaseLineModel() {
  // We do not own the histograms so we do not delete anything.
}



void BaseLineModel::UpdateModel(const double *pars) {
  factor = pars[0];
}


double BaseLineModel::operator()(const double * pars) {

  UpdateModel(pars);

  double chisq = 0.;

  for(int ibin=region1start; ibin<=region1end; ibin++) {
    double edata  =  _data->GetBinContent(ibin);
 //   std::cout << "data(" << ibin << ") = " << edata << "\n";
    double emodel = _model->GetBinContent(ibin);
 //   std::cout << "model(" << ibin << ") = " << emodel << "\n";
    double errorsq  = fabs(0.01*edata);
 //   std::cout << "errorsq = " << errorsq << "\n";
    chisq += pow((edata-emodel*factor), 2)/errorsq;
  }

  for(int ibin=region2start; ibin<=region2end; ibin++) {
    double edata  =  _data->GetBinContent(ibin);
 //   std::cout << "data(" << ibin << ") = " << edata << "\n";
    double emodel = _model->GetBinContent(ibin);
 //   std::cout << "model(" << ibin << ") = " << emodel << "\n";
    double errorsq  = fabs(0.01*edata);
 //   std::cout << "errorsq = " << errorsq << "\n";
    chisq += pow((edata-emodel*factor), 2)/errorsq;
  }

  std::cout << "Factor = " << factor << "; Chi2/ndf = " << chisq/(DataSize()-nPars) << "\n";
  std::cout.flush();
  return chisq;
}



