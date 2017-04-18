/*
 * MonitorFrame.h
 *
 *  Created on: Apr 16, 2017
 *      Author: S. Lukic
 */

#ifndef MONITORFRAME_HH_
#define MONITORFRAME_HH_

#include <queue>

#include <TGFrame.h>
//#include <TRootEmbeddedCanvas.h>
#include "TRandom3.h"
#include "TStopwatch.h"


#include "observables.h"

class TGLabel;
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
	  void Save(); 		   //*SLOT*
	  void ExportText(); //*SLOT*
	  void Exit();       //*SLOT*
	  
	  ClassDef(MonitorFrame,0)

//	  const static int autosavePeriod = 3600; // Autosave period in s

	  const struct histo_limits {
	  	short histolo[DRS4_data::nObservables];
	  	short histohi[DRS4_data::nObservables];
	  	histo_limits(const short *_lo, const short *_hi) {
	  		for (int i=0; i<DRS4_data::nObservables; i++) { histolo[i] = _lo[i]; histolo[i] = _hi[i]; }
	  	}
	  	~histo_limits() {};
	  } limits;



protected:
  /*** Display ***/

	config *options;

  TGLabel *nEvtT;
  TGLabel *rateT;

  TCanvas *fCanvas01;
  TRootCanvas *frCanvas01;
  TCanvas *fCanvas02;
  TRootCanvas *frCanvas02;
  TCanvas *fCanvas2D;
  TRootCanvas *frCanvas2D;

  const short tRed; // Update frequency reduction factor

  TString basename, filename;

  bool timestamped;

  DRS4_data::Observables *obs;

  TH1F *histo[2][DRS4_data::nObservables];
  TH2F *eTot12;
  TH2F *ePrompt12;
  TH2F *time12;
  TH2F *time34;
//  TTree *events;
  TRandom3 dice;
  TStopwatch timer;
  int timeLastSave;


  // To avoid putting this part into the class Dictionary
#ifndef __CINT__

  class RateEstimator {
  public:
	  RateEstimator() : evtRate(0) { rateCounts.push(0); rateTimes.push(0.); };
	  ~RateEstimator() {};
	  void Push(int, double);
	  double Get() const { return evtRate; };
	  const static int rateCountPeriod = 20; // event count for rate calculation is reset every this many events

  private:
	  std::queue<int> rateCounts;
	  std::queue<double> rateTimes;
	  double evtRate;
  } *rate;

  DRS *drs;
  DRS4_data::DRS4_fifo *fifo;
  DRS4_writer *writer;
  WaveProcessor *processor;

  DRS4_data::RawEvent *rawWave;
  DRS4_data::Event *event;
  DRS4_data::DRSHeaders *headers;


#endif /* __CINT__ */


  /*** Run handling ***/
  unsigned nEvtMax;
  unsigned iEvtProcessed; // Number of events processed by the reader
  unsigned iEvtSerial;    // Serial number reported by the last processed event

  std::ofstream *file;

  bool f_stop;
  bool f_stopWhenEmpty;

  /* Monitoring */

  float *timePoints;
  float *amplitudes;


  int itRed;
  std::ofstream *log;

  int Run();
  void FillHistos(Observables *[2]);
  void AutoSave();

  void DoDraw(bool all=false);

};


#endif /* MONITORFRAME_HH_ */
