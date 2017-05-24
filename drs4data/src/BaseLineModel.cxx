/*
 * BaseLineModel.cxx
 *
 * Class BaseLineModel is a chi2 functor for fitting
 * the baseline residual difference.
 *
 *  Created on: May 2017
 *      Author: S. Lukic
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
    region1start(0.5/data->GetBinWidth(1)+1), region1end(20./data->GetBinWidth(1)+1),
    region2start(140./data->GetBinWidth(1)+1), region2end(180./data->GetBinWidth(1)+1)
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

  std::cout << "Factor = " << factor << "; Chi2/ndf = " << chisq/(DataSize()-nPars) << "\r";
  std::cout.flush();
  return chisq;
}



