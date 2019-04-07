/*
 * MonitorFrame.h
 *
 *  Created on: Apr 16, 2017
 *      Author: S. Lukic
 */

#ifndef MONITORFRAME_HH_
#define MONITORFRAME_HH_

//#define ROOT_OUTPUT

#include <queue>

#include <TGFrame.h>
//#include <TRootEmbeddedCanvas.h>
#include "TRandom3.h"
#include "TStopwatch.h"
#include "TSystem.h"
#include "WaveProcessor.h"

#ifdef ROOT_OUTPUT
#include <TTree.h>
#include <TFile.h>
#include "Event.h"
#endif

#include "observables.h"

class TGLabel;
class TGHProgressBar;
class TGTextEntry;
class TCanvas;
class TRootCanvas;
class TH1F;
class TH2F;

using namespace DRS4_data;

class DRS;
class DRS4_writer;
namespace DRS4_data {
  class DRS4_fifo;
  class RawEvent;
  class Event;
  class DRSHeaders;
  class Observables;
}
class WaveProcessor;
class config;


class MonitorFrame : public TGMainFrame {

public:
    MonitorFrame(const TGWindow*,  config* const,  DRS* const);
    virtual ~MonitorFrame();

    /*** DAQ and control ***/
    void Start();      //*SLOT*
    void Stop();       //*SLOT*
    void HardStop();   //*SLOT*
    void Save();        //*SLOT*
    void ExportText(); //*SLOT*
    void Exit();       //*SLOT*
    void RefreshT();   //*SLOT*

    ClassDef(MonitorFrame,0)

//    const static int autosavePeriod = 3600; // Autosave period in s

    /* const struct histo_limits { */
    /*   short histolo[DRS4_data::nObservables]; */
    /*   short histohi[DRS4_data::nObservables]; */
    /*   histo_limits(const short *_lo, const short *_hi) { */
    /*     for (int i=0; i<DRS4_data::nObservables; i++) { histolo[i] = _lo[i]; histolo[i] = _hi[i]; } */
    /*   } */
    /*   ~histo_limits() {}; */
    /* } limits; */


protected:
  /*** Display ***/

  config *options;

  TGLabel *nEvtAcqT;
  TGLabel *nEvtProT;
  TGLabel *rateT;
  TGLabel *temperatureT;
  TGHProgressBar    *fHProg2;
  TGTextEntry* nEvtMaxT;
  TGTextEntry* confT;
  TGTextEntry* runIdT;
  TGTextEntry* outFileT;

  /* TCanvas *fCanvas01; */
  /* TRootCanvas *frCanvas01; */
  /* TCanvas *fCanvas02; */
  /* TRootCanvas *frCanvas02; */
  /* TCanvas *fCanvas2D; */
  /* TRootCanvas *frCanvas2D; */
  /* TCanvas *fCanvasOsc; */
  /* TRootCanvas *frCanvasOsc; */

  /* const short tRed; // Update frequency reduction factor */
  /* const short tRed2D; // Update frequency reduction factor for 2D histograms */

  const float baseLineWidth; // Width of window for the baseline calculation

  TString basename, filename;

  bool timestamped;

  /* TH1F *histo[2][DRS4_data::nObservables]; */
  /* TH2F *eTot12; */
  /* TH2F *ePrompt12; */
  /* TH2F *time12; */
  /* TH2F *time34; */
  //  TTree *events;

  TRandom3 dice;
  TStopwatch timer;
  int timeLastSave;

  int lastSpill;

  // To avoid putting this part into the class Dictionary
#ifndef __CINT__

  class RateEstimator {
  public:
    RateEstimator() : evtRate(0) { rateCounts.push(0); rateTimes.push(0.); };
    ~RateEstimator() {};
    void Push(int, double);
    double Get() const { return evtRate; };
    const static int rateCountPeriod = 50; // Number of events for rate calculation.
    const static int maxSize= 10; // Max number of event/time pairs for rate calc.

  private:
    std::queue<int> rateCounts;
    std::queue<double> rateTimes;
    double evtRate;
  } *rate;

  DRS *drs;
  DRS4_fifo *fifo;
  DRS4_writer *writer;

  DRS4_data::RawEvent *rawWave;
  DRS4_data::Event *event;
  DRS4_data::DRSHeaders *headers;


#endif /* __CINT__ */

  /*** Run handling ***/
  unsigned nEvtMax;
  unsigned iEvtProcessed; // Number of events processed by the reader
  unsigned iEvtSerial;    // Serial number reported by the last processed event

#ifndef ROOT_OUTPUT
  std::ofstream *file;
#else
  TFile *file;
  TTree *outTree;
  H4DAQ::Event *h4daqEvent;
#endif

  bool f_stop;
  bool f_stopWhenEmpty;
  bool f_running;

  /* Monitoring */

  /* float *timePoints; */
  /* float *amplitudes; */
  /* int itRed, itRed2D; */

  int Run();
  void FillHistos(Observables *[2]);
  void AutoSave();
  int StartNewFile();

  void DoDraw(bool all=false);

};


#endif /* MONITORFRAME_HH_ */
