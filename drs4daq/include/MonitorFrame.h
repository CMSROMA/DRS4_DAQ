/*
 * MonitorFrame.h
 *
 *  Created on: Apr 16, 2017
 *      Author: S. Lukic
 */

#ifndef MONITORFRAME_HH_
#define MONITORFRAME_HH_

#include <TGFrame.h>
//#include <TRootEmbeddedCanvas.h>
//#include "TTree.h"
#include "TRandom3.h"
#include "TStopwatch.h"
//#include "TFile.h"


#include "observables.h"

class TGLabel;
class TCanvas;
class TRootCanvas;
class TH1F;
class TH2F;

class DRS4_fifo;
class DRS4_writer;
class DRS4_reader;

class config;


class MonitorFrame : public TGMainFrame {

public:
	  MonitorFrame(const TGWindow*,  const config*,  DRS*);
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

  TGLabel *nEvtT;
  TGLabel *rateT;

  TCanvas *fCanvas01;
  TRootCanvas *frCanvas01;
  TCanvas *fCanvas02;
  TRootCanvas *frCanvas02;

  const short tRed; // Update frequency reduction factor

  TString basename, filename;

  bool timestamped;

  DRS4_data::Observables *obs;

  TH1F *histo[DRS4_data::nObservables];
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
  DRS4_fifo *fifo;
  DRS4_writer *writer;
  DRS4_reader *reader;
#endif /* __CINT__ */

  /*** Run handling ***/
  int nEvents;
  int itRed;
  std::ofstream *log;

  void Run();
  void HandleData();
  void AutoSave();

  void DoDraw(bool all=false);

};


#endif /* MONITORFRAME_HH_ */
