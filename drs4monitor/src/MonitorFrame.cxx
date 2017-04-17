/*
 * MonitorFrame.cxx
 *
 *  Created on: Apr 16, 2017
 *      Author: S. Lukic
 */

#include "TApplication.h"
#include <TGClient.h>
#include "TGButton.h"
#include "TGTextView.h"
#include "TGText.h"
#include "TGLabel.h"
#include "TCanvas.h"
#include "TRootCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TTimeStamp.h"
#include "TFile.h"
#include "TString.h"


#include "MonitorFrame.h"
#include "Riostream.h"
#include "TMath.h"

#include "DAQ-config.h"

#include "DRS4_fifo.h"
#include "DRS4_writer.h"
#include "DRS4_reader.h"
#include "observables.h"

using namespace DRS4_data;


MonitorFrame::MonitorFrame(const TGWindow *p, const config *opt, DRS * const _drs) :
  TGMainFrame(p, 200, 200),
  limits(opt->histolo, opt->histohi),
  fCanvas01(new TCanvas("DRS4Canvas02", "DRS4 Monitor 01", opt->_w, opt->_h)),
  frCanvas01(new TRootCanvas(fCanvas01, "DRS4 Monitor 01", 0, 200, opt->_w, opt->_h)),
  fCanvas02(new TCanvas("DRS4Canvas02", "DRS4 Monitor 02", opt->_w, opt->_h)),
  frCanvas02(new TRootCanvas(fCanvas02, "DRS4 Monitor 02", 0, 200, opt->_w, opt->_h)),
  fCanvas2D(new TCanvas("DRS4Canvas2D", "DRS4 Monitor 2D", opt->_w, opt->_h)),
  frCanvas2D(new TRootCanvas(fCanvas2D, "DRS4 Monitor 2D", 0, 200, opt->_w, opt->_h)),
  tRed(opt->_tRed),
  basename("default"), filename("default"), timestamped(false),
  obs(new Observables),
  eTot12(NULL), ePrompt12(NULL),
  time12(NULL), time34(NULL),
  timeLastSave(0),
  rate(NULL),
  drs(_drs), fifo(new DRS4_fifo), writer(NULL), rawWave(NULL), event(NULL),
  headers(NULL), nEvtMax(10), iEvtSerial(0), iEvtProcessed(0), file(NULL),
  f_stop(false), f_stopWhenEmpty(false),
  timePoints(NULL), amplitudes(NULL),
  itRed(0),
  log(NULL)
{

  for(int iobs=0; iobs<nObservables; iobs++) {
    kObservables kobs = static_cast<kObservables>(iobs);
    histo01[iobs] = new TH1F(Form("h%d", iobs), Form("%s_{1}; %s_{1} (%s)", obs->Name(kobs), obs->Name(kobs), obs->Unit(kobs)), 100, opt->histolo[iobs], opt->histohi[iobs]);
    histo02[iobs] = new TH1F(Form("h%d", iobs), Form("%s_{2}; %s_{2} (%s)", obs->Name(kobs), obs->Name(kobs), obs->Unit(kobs)), 100, opt->histolo[iobs], opt->histohi[iobs]);
  }

  eTot12 = new TH2F("eTot12", Form("eTot2 vs. eTot1; %s_{1} (%s); %s_{2} (%s)", obs->Name(eTot), obs->Unit(eTot), obs->Name(eTot), obs->Unit(eTot)),
      (opt->histohi[eTot]-opt->histolo[eTot])/opt->_xRed, opt->histolo[eTot], opt->histohi[eTot],
      (opt->histohi[eTot]-opt->histolo[eTot])/opt->_xRed, opt->histolo[eTot], opt->histohi[eTot]);
  ePrompt12 = new TH2F("ePrompt12", Form("ePrompt2 vs. ePrompt1; %s_{1} (%s); %s_{2} (%s)", obs->Name(ePrompt), obs->Unit(ePrompt), obs->Name(ePrompt), obs->Unit(ePrompt)),
      (opt->histohi[ePrompt]-opt->histolo[ePrompt])/opt->_xRed, opt->histolo[ePrompt], opt->histohi[ePrompt],
      (opt->histohi[ePrompt]-opt->histolo[ePrompt])/opt->_xRed, opt->histolo[ePrompt], opt->histohi[ePrompt]);
  time12 = new TH2F("time12", Form("t2 vs. t1; %s_{1} (%s); %s_{2} (%s)", obs->Name(tArrival), obs->Unit(tArrival), obs->Name(tArrival), obs->Unit(tArrival)),
      (opt->histohi[tArrival]-opt->histolo[tArrival])/opt->_xRed, opt->histolo[tArrival], opt->histohi[tArrival],
      (opt->histohi[tArrival]-opt->histolo[tArrival])/opt->_xRed, opt->histolo[tArrival], opt->histohi[tArrival]);
  time34 = new TH2F("time34", "t4 vs. t3; t_{3} (ns); t_{4} (ns)", 100, 0., 100., 100, 0., 100.);


	if( gApplication->Argc() > 1 ) basename = gApplication->Argv()[1];
	std::cout << "basename = " << basename << "\n";

	TTimeStamp ts(std::time(NULL), 0);
		filename = basename;
		filename += "_";
		filename += ts.GetDate(0);
		filename += "-";
		filename += ts.GetTime(0);
		filename += ".log";
	log = new std::ofstream(filename.Data());
	filename = basename;

	// Placement of histograms
	unsigned ny = int(sqrt(nObservables));
	unsigned nx = int(ceil(nObservables / ny));
  fCanvas01->Divide(nx, ny, .002, .002);
  fCanvas02->Divide(nx, ny, .002, .002);
  for( unsigned ih=0; ih<nObservables; ih++) {
    fCanvas01->cd(1); histo01[ih]->Draw();
    fCanvas02->cd(1); histo02[ih]->Draw();
  }

	fCanvas2D->Divide(2, 2, .002, .002);
	fCanvas2D->cd(1); eTot12->Draw();
	fCanvas2D->cd(2); ePrompt12->Draw();
	fCanvas2D->cd(3); time12->Draw();
	fCanvas2D->cd(4); time34->Draw();
  eTot12->SetStats(0);
  ePrompt12->SetStats(0);
  time12->SetStats(0);
  time34->SetStats(0);


// Create a horizontal frame widget with buttons
	TGHorizontalFrame *hframe = new TGHorizontalFrame(this,200,60);

	TGFontPool *pool = gClient->GetFontPool();
	// family , size (minus value - in pixels, positive value - in points), weight, slant
	//  kFontWeightNormal,  kFontSlantRoman are defined in TGFont.h
	TGFont *font = pool->GetFont("helvetica", 14,  kFontWeightBold,  kFontSlantOblique);
//	TGFont *font = pool->GetFont("-adobe-helvetica-bold-r-normal-*-15-*-*-*-*-*-iso8859-1");
	font->Print();
	FontStruct_t ft = font->GetFontStruct();

	TGTextButton *start = new TGTextButton(hframe,"&Start");
	start->Connect("Clicked()","DAQMainFrame", this, "Start()");
	start->SetFont(ft);
	hframe->AddFrame(start, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

	TGTextButton *stop = new TGTextButton(hframe,"&Stop");
	stop->Connect("Clicked()","DAQMainFrame",this,"Stop()");
	stop->SetFont(ft);
	hframe->AddFrame(stop, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

	TGTextButton *save = new TGTextButton(hframe,"&Save");
	save->Connect("Clicked()","DAQMainFrame",this,"Save()");
	save->SetFont(ft);
	hframe->AddFrame(save, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

	TGTextButton *exportText = new TGTextButton(hframe,"&Export Text");
	exportText->Connect("Clicked()","DAQMainFrame",this,"ExportText()");
	exportText->SetFont(ft);
	hframe->AddFrame(exportText, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

	TGTextButton *exit = new TGTextButton(hframe,"&Exit");
	exit->Connect("Clicked()", "DAQMainFrame",this,"Exit()");
	exit->SetFont(ft);
//	exit->Connect("Clicked()", "TApplication", gApplication, "Terminate(0)");
	hframe->AddFrame(exit, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
	AddFrame(hframe, new TGLayoutHints(kLHintsCenterX | kLHintsTop | kLHintsExpandY,2,2,2,2));


// Create a horizontal frame widget with text displays
	TGHorizontalFrame *hframeT = new TGHorizontalFrame(this,200,60);

	TGLabel *nEvtL = new TGLabel(hframeT, "Event: ");
	nEvtL->SetTextFont(ft);
	hframeT->AddFrame(nEvtL, new TGLayoutHints(kLHintsCenterX,5,1,3,4));

	nEvtT = new TGLabel(hframeT, Form("%-15i", 0));
	nEvtT->SetTextFont(ft);
	hframeT->AddFrame(nEvtT, new TGLayoutHints(kLHintsCenterX,1,5,3,4));

	TGLabel *rateL = new TGLabel(hframeT, "Rate: ");
	rateL->SetTextFont(ft);
	hframeT->AddFrame(rateL, new TGLayoutHints(kLHintsCenterX,5,1,3,4));

	rateT = new TGLabel(hframeT, "0.000 evt/s");
	rateT->SetTextFont(ft);
	hframeT->AddFrame(rateT, new TGLayoutHints(kLHintsCenterX,1,5,3,4));

	AddFrame(hframeT, new TGLayoutHints(kLHintsCenterX | kLHintsTop | kLHintsExpandY,2,2,2,2));

	std::cout << "Added labels.\n";

// Set a name to the main frame
	SetWindowName("Acquisition Control");
// Map all subwindows of main frame
	MapSubwindows();
// Initialize the layout algorithm
	Resize(GetDefaultSize());
// Map main frame
	MapWindow();

	std::cout << "Constructed main window.\n";

}



MonitorFrame::~MonitorFrame() {
	std::cout << "Deleting main frame.\n";

  for(int iobs=0; iobs<nObservables; iobs++) {
    delete histo02[iobs];
    histo02[iobs] = NULL;
  }

  delete eTot12;      eTot12 = NULL;
  delete ePrompt12;   ePrompt12 = NULL;
  delete time12;      time12 = NULL;
  delete time34;      time34 = NULL;
}



void MonitorFrame::Exit() {
	std::cout << "Cleanup main frame.\n";

	if(writer) {
	  delete writer;
	}

	if(fifo) {
	  delete fifo;
	}

	log->close();

	gApplication->Terminate(0);
}


void MonitorFrame::Start() {

  if(!drs) {
    std::cout << "MonitorFrame::Start() - ERROR: DRS object empty." << std::endl;
    return;
  }

  if(drs->GetNumberOfBoards() < 1) {
    std::cout << "MonitorFrame::Start() - ERROR: No DRS boards present." << std::endl;
    return;
  }

  fifo->Discard();
  writer = new DRS4_writer(drs, fifo);

  /*** Clear histograms ***/
  for(int iobs=0; iobs<nObservables; iobs++) {
    histo01[iobs]->Reset();
    histo02[iobs]->Reset();
  }
  eTot12->Reset();
  ePrompt12->Reset();
  time12->Reset();
  time34->Reset();



	filename = basename;
	timestamped=false;


	rate = new MonitorFrame::RateEstimator();
	rate->Push(0, 1.e-9); // One false time value to avoid division by zero
	timer.Start();
	timeLastSave = 0;


	/*** Start writer ***/
  writer->start(nEvtMax);
  while (!writer->isRunning()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); };

  /*** Start monitor ***/
	Run();
}

int MonitorFrame::Run() {

  file = new std::ofstream(filename, std::ios_base::binary & std::ios_base::trunc) ;

  if( file->fail() ) {
    std::cout << "ERROR: Cannot open file " << filename << " for writing.\n";
    writer->stop();
    return -1;
  }

  std::cout << "Starting Monitor frame.\n";

  /*** Write file header and time calibration ***/
  std::cout << "Writing headers and time calibration." << std::endl;

  // Fixme: Need to fill the headers first.
  file->write(reinterpret_cast<const char*>(&headers->fheader), 4);
  file->write(reinterpret_cast<const char*>(&DRS4_data::THEADER), 4);

  for(int iboard=0; iboard<headers->chTimes.size(); iboard++) {
    // Write board header
    file->write(headers->bheaders.at(iboard)->bn, 2);
    file->write(reinterpret_cast<const char*>(&(headers->bheaders.at(iboard)->board_serial_number)), 2);
    // Write time calibration
    for (int ichan=0; ichan<headers->chTimes.at(iboard).size(); ichan++) {
      file->write(reinterpret_cast<const char*>(headers->chTimes.at(iboard).at(ichan)), sizeof(DRS4_data::ChannelTime) );
    }
  } // End loop over boards
  std::cout << "Done writing headers." << std::endl;


  // Time calibrations
  const bool tCalibrated = true; // Whether time points should come calibrated
  const bool tRotated = true;    // Whether time points should come rotated
  // Voltage calibrations
  const bool applyResponseCalib = true;  // Remove noise and offset variations
  const bool adjustToClock = false;      // Extra rotation of amplitudes in the calibration step - SET TO FALSE !
  const bool adjustToClockForFile = false;
  const bool applyOffsetCalib = false;   // ?

  iEvtProcessed=0;

  while(!f_stop) {

    gClient->ProcessEventsFor(this);
    rawWave = fifo->read();

    if(rawWave) {
      iEvtSerial = rawWave->header.getEventNumber();
      std::cout << "Read event #" << iEvtSerial << std::endl;
      std::cout << "Trigger cell is " << rawWave->header.getTriggerCell() << std::endl;
      event = new DRS4_data::Event(iEvtSerial, rawWave->header, drs);

      for(int iboard=0; iboard<headers->chTimes.size(); iboard++) {
        DRSBoard *b = drs->GetBoard(iboard);
        for (unsigned char ichan=0 ; ichan<4 ; ichan++) {

          /* decode waveform (Y) arrays in mV */
          // Do not use DRSBoard::GetTriggerCell()
          // - it shows the CURRENT trigger cell, not that corresponding to the waveform
          std::cout << "Decoding waveform in chan #" << int(ichan)+1 << std::endl;
          b->GetWave(rawWave->eventWaves.at(iboard)->waveforms, 0, ichan*2, (short*)event->getChData(iboard, ichan)->data,
              applyResponseCalib, int(rawWave->header.getTriggerCell()), -1, adjustToClockForFile, 0, applyOffsetCalib);

          float time[kNumberOfBins];
          b->GetTime(0, ichan*2, int(rawWave->header.getTriggerCell()), time, tCalibrated, tRotated);
          float amplitude[kNumberOfBins];
          b->GetWave(rawWave->eventWaves.at(iboard)->waveforms, 0, ichan*2, amplitude, applyResponseCalib,
              int(rawWave->header.getTriggerCell()), -1, adjustToClock, 0, applyOffsetCalib);

        } // Loop over the channels


      }
      event->write(file);
      HandleData();
      delete event;
      iEvtProcessed++;
    } // If rawWave (fifo not empty)
    else {
      if(f_stopWhenEmpty || !writer->isRunning()) {
        f_stop = true;
      }
      else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      } // if(f_stopWhenEmpty)
    } // if(rawWave)
  } // !f_stop

  return 0;

	Stop();

}


void MonitorFrame::Stop() {

	std::cout << "\n\nStopping.\n";
  f_stop = true;
	timer.Stop();
	DoDraw(true);

	delete rate;
	rate = NULL;

	std::cout << "Events processed: " << iEvtProcessed << "\n";
	std::cout << Form("Elapsed time: %6.2f s.\n", timer.RealTime());
	std::cout << Form("Event rate: %6.2f events/s \n", float(iEvtProcessed)/timer.RealTime());

	TTimeStamp ts(std::time(NULL), 0);
	filename = basename;
	filename += "_";
	filename += ts.GetDate(0);
	filename += "-";
	filename += ts.GetTime(0);
	timestamped=true;
}


void MonitorFrame::HandleData()
{


} // HandleData()





void MonitorFrame::DoDraw(bool all) {
// Draws function graphics in randomly chosen interval

  fCanvas01->Paint();
  fCanvas02->Paint();
  fCanvas01->Update();
  fCanvas02->Update();
	if(all) {
    fCanvas2D->Paint();
    fCanvas2D->Update();
	}

	nEvtT->SetText(Form("%-10i", iEvtProcessed));
	rateT->SetText(Form("%5.3g evt/s", rate->Get()));
	timer.Continue();

}

void MonitorFrame::AutoSave() {
	TTimeStamp ts(std::time(NULL), 0);
	filename = basename;
	filename += "_autosave_";
	filename += ts.GetDate(0);
	filename += "-";
	filename += ts.GetTime(0);
	timestamped = true;
	Save();
	filename = basename;
	timestamped=false;
}

void MonitorFrame::Save() {

	if ( !timestamped ) {
		TTimeStamp ts(std::time(NULL), 0);
		filename = basename;
		filename += "_";
		filename += ts.GetDate(0);
		filename += "-";
		filename += ts.GetTime(0);
	}

	TString rootfilename(filename); rootfilename += ".root";

	TFile output(rootfilename.Data(), "RECREATE");
	if(!(output.IsOpen())) {
		std::cout << "\nCannot open output root file " << rootfilename.Data() << ".\n";
//			gApplication->Terminate(0);
	}

  for(int iobs=0; iobs<nObservables; iobs++) {
    histo01[iobs]->Clone()->Write();
    histo02[iobs]->Clone()->Write();
  }

  eTot12->Clone()->Write();
  ePrompt12->Clone()->Write();
  time12->Clone()->Write();
  time34->Clone()->Write();

	output.Close();
	std::cout << "\nSaved data in " << rootfilename.Data() << "\n";
	timeLastSave = timer.RealTime();
	timer.Continue();
}


void MonitorFrame::ExportText() {
/*
	if ( !timestamped ) {
		TTimeStamp ts(std::time(NULL), 0);
		filename = basename;
		filename += "_";
		filename += ts.GetDate(0);
		filename += "-";
		filename += ts.GetTime(0);
	}

	TString textfilename(filename); textfilename += ".dat";

	std::ofstream exportfile(textfilename.Data(), std::ios::trunc);
	if (!exportfile.is_open()) {
		std::cout << "\nCannot open file " << textfilename.Data() << " for writing.\n";
		return;
	}


  for (kObservables i=0; i<nObservables; i++) { exportfile << Form("  %s(1)  ", obs->Name(i)); }
  for (kObservables i=0; i<nObservables; i++) { exportfile << Form("  %s(2)  ", obs->Name(i)); }
	exportfile << "\n";
	for (int iEvt=0; iEvt<events->GetEntries(); iEvt++) {
		events->GetEvent(iEvt);
		for (kObservables i=0; i<nObservables; i++) { exportfile << Form("%8.3f", ???); }
		for (kObservables i=0; i<nObservables; i++) { exportfile << Form("%8.3f", ???); }
		exportfile << "\n";
	}
	exportfile.close();

	std::cout << "\nSaved text data in " << textfilename.Data() << "\n";
	*/
}



void MonitorFrame::RateEstimator::Push(int count, double time) {
	rateCounts.push(count);
	rateTimes.push(time);
	int nEvtRate = rateCounts.front() - rateCounts.back();
	evtRate = double(nEvtRate) / (rateTimes.front() - rateTimes.back());
	if(nEvtRate >= rateCountPeriod) {
		rateCounts.pop();
		rateTimes.pop();
	}
}
