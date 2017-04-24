/*
 * DAQ-config.cxx
 *
 *  Created on: Jun 28, 2015
 *      Author: straja
 */

#include "Riostream.h"
#include "TString.h"
#include "TRegexp.h"

#include "DAQ-config.h"

using namespace DRS4_data;

config::config(Observables *_obs) :
  nEvtMax(10),
  _w(600), _h(350),
  _tRed(10), _tRed2D(50), _xRed(2), _yRed(2),
  sampleRate(5), inputRange(0), triggerLevel(-0.05),
  triggerNegative(true), trigDelay(150), triggerSource(1),
  obs(_obs)
{
  for(int i=0; i<nObservables; i++) {
    histolo[i] = 0; histohi[i] = 400;
  }
}

config::~config() { }


int config::DumpOptions() const {

  std::cout << "\nDump of the run parameters:\n\n";
  std::cout << "Maximum number of events: " << nEvtMax << std::endl;

  std::cout << "\nDump of the display configuration:\n\n";
  std::cout << "Frame dimensions: " << _w << " x " << _h << std::endl;
  std::cout << "Histogram ranges: \n";
  for (int i=0; i<nObservables; i++) {
    std::cout << Form("%s: ", obs->Name(static_cast<kObservables>(i))) << histolo[i] << " - " << histohi[i] << std::endl;
  }
  std::cout << "Reduction: x(" << _xRed << "X), y(" << _yRed << "X), t(" << _tRed << "X), t2D(" << _tRed2D << "X).\n";

  std::cout << "\nDump of the DAQ configuration:\n\n";
  std::cout << "Sample rate: " << sampleRate <<  std::endl;
  std::cout << "Input range center: " << inputRange <<  std::endl;
  std::cout << "Trigger level: " << triggerLevel <<  std::endl;
  std::cout << "Trigger slope: " << (triggerNegative ? "negative" : "positive") <<  std::endl;
  std::cout << "Trigger delay: " << (int)trigDelay <<  std::endl;
  std::cout << "Trigger source: ";
  switch(triggerSource) {
  case 1:
    std::cout << "Channel 1." << std::endl;
    break;
  case 2:
    std::cout << "Channel 2." << std::endl;
    break;
  case 4:
    std::cout << "Channel 3." << std::endl;
    break;
  case 8:
    std::cout << "Channel 4." << std::endl;
    break;
  case 16:
    std::cout << "External." << std::endl;
    break;
  case 0:
    std::cout << "Auto." << std::endl;
    break;
  default:
    std::cout << "Unknown." << std::endl;
    break;
  }

  return 0;
}

int config::ParseOptions(std::ifstream *input)
{
  const TRegexp number_patt("[-+ ][0-9]*[.]?[0-9]+");
  TString line;

  while(!input->eof()) {

    line.ReadLine(*input);
//    std::cout << line.Data() << "\n";

    if(line.BeginsWith("#") || line.BeginsWith("!")) continue;


    /**** Run control ****/

    if( (line.Contains("event", TString::kIgnoreCase)
        || line.Contains("evt", TString::kIgnoreCase))
        && line.Contains("max", TString::kIgnoreCase) ) {
      TString nevtm = line(number_patt);
      nEvtMax = nevtm.Atoi();
      if (nEvtMax < 0) { nEvtMax = 0; }
      continue;
    }


    /**** Display appearance and behaviour ****/

    if(line.Contains("window", TString::kIgnoreCase)) {
      TSubString wid = line(number_patt);
      TString hgt = line(number_patt, wid.Start()+wid.Length());
      _w = TString(wid).Atoi();
      if (_w <= 0) {
        std::cout << "Error parsing options: Window width " << _w << " not allowed.\n";
        return -1;
      }
      _h = hgt.Atoi();
      if (_h <= 0) {
        std::cout << "Error parsing options: Window height " << _h << " not allowed.\n";
        return -1;
      }
      continue;
    }


    for (int i=0; i<nObservables; i++) {
      TString achan = line(TRegexp(Form("%s: ", obs->Name(static_cast<kObservables>(i)))));
      if(achan.Length() > 1) {
        TSubString lo = line(number_patt);
        TSubString hi = line(number_patt, lo.Start()+lo.Length());
        TString nchan = line(number_patt, hi.Start()+hi.Length());
        histolo[i] = TString(lo).Atoi();
        histohi[i] = TString(hi).Atoi();
        histoNbins[i] = TString(nchan).Atoi();
        if(histolo[i] == histohi[i]) {
            std::cout << "Error parsing options: " << achan.Data() << " limits equal ("
                << histolo[i] << ", " << histohi[i] << ")\n";
            return -1;
          }
        if(histolo[i] > histohi[i]) {
          short tmp = histolo[i]; histolo[i] = histohi[i];
          histohi[i] = tmp;
        }
        if(histoNbins[i] == 0) { histoNbins[i] = 100 ; }
      } // (achan.Length() > 1)
    } // loop over observables



    if(line.Contains("reduction", TString::kIgnoreCase)) {
      TSubString t = line(number_patt);
      TSubString t2D = line(number_patt, t.Start()+t.Length());
      TSubString x = line(number_patt, t2D.Start()+t2D.Length());
      TSubString y = line(number_patt, x.Start()+x.Length());
      _tRed = TString(t).Atoi();
      if (_tRed <= 0) {
        std::cout << "Time reduction value " << _tRed << " is not positive.\n";
        return -1;
      }
      _tRed2D = TString(t2D).Atoi();
      if (_tRed2D <= 0) {
        std::cout << "Time reduction value " << _tRed2D << " is not positive.\n";
        return -1;
      }
      _xRed = TString(x).Atoi();
      if (_xRed <= 0) {
        std::cout << "X reduction value " << _xRed << " is not positive.\n";
        return -1;
      }
      _yRed = TString(y).Atoi();
      if (_yRed <= 0) {
        std::cout << "Y reduction value " << _yRed << " is not positive.\n";
        return -1;
      }
      continue;
    }


    /**** Evaluation board parameters ****/

    if(line.Contains("sample", TString::kIgnoreCase) &&
       line.Contains("rate", TString::kIgnoreCase) )    {
      TString par = line(number_patt);
      sampleRate = par.Atof();
      continue;
    }

    if(line.Contains("input", TString::kIgnoreCase) &&
       line.Contains("range", TString::kIgnoreCase) )    {
      TString par = line(number_patt);
      inputRange = par.Atof();
      continue;
    }

    if(line.Contains("trig", TString::kIgnoreCase) &&
       line.Contains("level", TString::kIgnoreCase) )    {
      TString par = line(number_patt);
      triggerLevel = par.Atof();
      continue;
    }

    if(line.Contains("trig", TString::kIgnoreCase) &&
       line.Contains("positive", TString::kIgnoreCase) )  {
      triggerNegative = false;
      continue;
    }

    if(line.Contains("trig", TString::kIgnoreCase) &&
       line.Contains("delay", TString::kIgnoreCase) )    {
      TString par = line(number_patt);
      trigDelay = par.Atoi();
      continue;
    }

    if(line.Contains("trig", TString::kIgnoreCase) &&
       line.Contains("source", TString::kIgnoreCase) )    {
      TString par = line(number_patt);
      triggerSource = par.Atoi();
      continue;
    }

  }
  return 0;
}



