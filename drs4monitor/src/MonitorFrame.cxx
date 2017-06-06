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
#include "TLegend.h"
#include "TLegendEntry.h"


#include "MonitorFrame.h"
#include "Riostream.h"
#include "TMath.h"

#include "DAQ-config.h"

#include "DRS4_fifo.h"
#include "DRS4_writer.h"
#include "DRS4_reader.h"
#include "observables.h"

using namespace DRS4_data;


MonitorFrame::MonitorFrame(const TGWindow *p, config * const opt, DRS * const _drs) :
  TGMainFrame(p, 250, 200),
  limits(opt->histolo, opt->histohi),
  options(opt),
  fCanvas01(new TCanvas("DRS4Canvas01", "DRS4 Monitor 01", opt->_w, opt->_h)),
  frCanvas01(new TRootCanvas(fCanvas01, "DRS4 Monitor 01", 0, 200, opt->_w, opt->_h)),
  fCanvas02(new TCanvas("DRS4Canvas02", "DRS4 Monitor 02", opt->_w, opt->_h)),
  frCanvas02(new TRootCanvas(fCanvas02, "DRS4 Monitor 02", 0, 200, opt->_w, opt->_h)),
  fCanvas2D(new TCanvas("DRS4Canvas2D", "DRS4 Monitor 2D", opt->_w*2/3, opt->_h)),
  frCanvas2D(new TRootCanvas(fCanvas2D, "DRS4 Monitor 2D", 0, 200, opt->_w*2/3, opt->_h)),
  fCanvasOsc(new TCanvas("DRS4CanvasOsc", "DRS4 oscillogram", opt->_w/3, opt->_h/2)),
  frCanvasOsc(new TRootCanvas(fCanvasOsc, "DRS4 oscillogram", 0, 200, opt->_w/3, opt->_h/2)),
  tRed(opt->_tRed), tRed2D(opt->_tRed2D),
  baseLineWidth(opt->baseLineWidth),
  basename("default"), filename("default"), timestamped(false),
  eTot12(NULL), ePrompt12(NULL), time12(NULL), time34(NULL),
  timeLastSave(0),  rate(NULL),
  drs(_drs), fifo(new DRS4_fifo), writer(NULL),
  rawWave(NULL), event(NULL),
  headers(NULL), nEvtMax(opt->nEvtMax), iEvtSerial(0), iEvtProcessed(0), file(NULL),
  f_stop(false), f_stopWhenEmpty(false), f_running(false),
  timePoints(NULL), amplitudes(NULL),
  itRed(0), itRed2D(0)
{

  Observables obs;
  for(int iobs=0; iobs<nObservables; iobs++) {
    kObservables kobs = static_cast<kObservables>(iobs);
    histo[0][iobs] = new TH1F(Form("h1%d", iobs), Form("%s - S1; %s_{1} (%s)", obs.Title(kobs), obs.Title(kobs), obs.Unit(kobs)), opt->histoNbins[iobs], opt->histolo[iobs], opt->histohi[iobs]);
    histo[1][iobs] = new TH1F(Form("h2%d", iobs), Form("%s - S2; %s_{2} (%s)", obs.Title(kobs), obs.Title(kobs), obs.Unit(kobs)), opt->histoNbins[iobs], opt->histolo[iobs], opt->histohi[iobs]);
  }

  eTot12 = new TH2F("eTot12", Form("eTot2 vs. eTot1; %s_{1} (%s); %s_{2} (%s)", obs.Title(eTot), obs.Unit(eTot), obs.Title(eTot), obs.Unit(eTot)),
      opt->histoNbins[eTot]/opt->_xRed, opt->histolo[eTot], opt->histohi[eTot],
      opt->histoNbins[eTot]/opt->_xRed, opt->histolo[eTot], opt->histohi[eTot]);
  ePrompt12 = new TH2F("ePrompt12", Form("ePrompt2 vs. ePrompt1; %s_{1} (%s); %s_{2} (%s)", obs.Title(ePrompt), obs.Unit(ePrompt), obs.Title(ePrompt), obs.Unit(ePrompt)),
      opt->histoNbins[ePrompt]/opt->_xRed, opt->histolo[ePrompt], opt->histohi[ePrompt],
      opt->histoNbins[ePrompt]/opt->_xRed, opt->histolo[ePrompt], opt->histohi[ePrompt]);
  time12 = new TH2F("time12", Form("t_{2} vs. t_{1}; %s_{1} (%s); %s_{2} (%s)", obs.Title(arrivalTime), obs.Unit(arrivalTime), obs.Title(arrivalTime), obs.Unit(arrivalTime)),
      opt->histoNbins[arrivalTime]/opt->_xRed, opt->histolo[arrivalTime], opt->histohi[arrivalTime],
      opt->histoNbins[arrivalTime]/opt->_xRed, opt->histolo[arrivalTime], opt->histohi[arrivalTime]);
  time34 = new TH2F("time34", "t_{4} vs. t_{3}; t_{3} (ns); t_{4} (ns)", 100, 0., 100., 100, 0., 100.);


  if( gApplication->Argc() > 1 ) basename = gApplication->Argv()[1];
  std::cout << "basename = " << basename << "\n";

  // Placement of histograms
  unsigned ny = int(sqrt(float(nObservables)));
  unsigned nx = int(ceil(nObservables / ny));
  fCanvas01->Divide(nx, ny, .002, .002);
  fCanvas02->Divide(nx, ny, .002, .002);
  for( unsigned ih=0; ih<nObservables; ih++) {
    fCanvas01->cd(ih+1); histo[0][ih]->Draw();
    fCanvas02->cd(ih+1); histo[1][ih]->Draw();
  }

  fCanvas2D->Divide(2, 2, .002, .002);
  fCanvas2D->cd(1); eTot12->Draw("colz");
  fCanvas2D->cd(2); ePrompt12->Draw("colz");
  fCanvas2D->cd(3); time12->Draw("colz");
  fCanvas2D->cd(4); time34->Draw("colz");
  eTot12->SetStats(0);
  ePrompt12->SetStats(0);
  time12->SetStats(0);
  time34->SetStats(0);

  fCanvasOsc->cd();
  fCanvasOsc->GetListOfPrimitives()->SetOwner();
  fCanvasOsc->SetTickx();
  fCanvasOsc->SetTicky();
  TH1F *oscFrame = new TH1F("OscFrame", "Oscillograms; t (ns); A (mV)", 10, 0., 200.);
  oscFrame->SetBit(kCanDelete, false);
  oscFrame->SetMinimum(-550);
  oscFrame->SetMaximum( 50);
  oscFrame->SetStats(false);
  oscFrame->Draw();
  oscFrame=NULL;
  TLegend * oscLeg = new TLegend(.68, .45, .88, .6);
  oscLeg->SetBorderSize(0);
  oscLeg->SetFillStyle(4001);
  oscLeg->SetTextFont(42);
  oscLeg->SetTextSize(.05);
  TLegendEntry *leS1 = oscLeg->AddEntry("S1", " S1", "l");
  leS1->SetLineColor(kBlue);
  leS1->SetLineWidth(2);
  TLegendEntry *leS2 = oscLeg->AddEntry("S2", " S2", "l");
  leS2->SetLineColor(kRed);
  leS2->SetLineWidth(2);
  oscLeg->Draw();


// Create a horizontal frame widget with buttons
  TGHorizontalFrame *hframe = new TGHorizontalFrame(this,250,60);

  TGFontPool *pool = gClient->GetFontPool();
  // family , size (minus value - in pixels, positive value - in points), weight, slant
  //  kFontWeightNormal,  kFontSlantRoman are defined in TGFont.h
  TGFont *font = pool->GetFont("helvetica", 14,  kFontWeightBold,  kFontSlantOblique);
//  TGFont *font = pool->GetFont("-adobe-helvetica-bold-r-normal-*-15-*-*-*-*-*-iso8859-1");
  font->Print();
  FontStruct_t ft = font->GetFontStruct();

  TGTextButton *start = new TGTextButton(hframe,"&Start");
  start->Connect("Clicked()","MonitorFrame", this, "Start()");
  start->SetFont(ft);
  hframe->AddFrame(start, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

  TGTextButton *stop = new TGTextButton(hframe,"&Stop");
  stop->Connect("Clicked()","MonitorFrame",this,"Stop()");
  stop->SetFont(ft);
  hframe->AddFrame(stop, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

  TGTextButton *hardstop = new TGTextButton(hframe,"&Stop!");
  hardstop->Connect("Clicked()","MonitorFrame",this,"HardStop()");
  hardstop->SetFont(ft);
  hframe->AddFrame(hardstop, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

/*  TGTextButton *save = new TGTextButton(hframe,"&Save");
  save->Connect("Clicked()","MonitorFrame",this,"Save()");
  save->SetFont(ft);
  hframe->AddFrame(save, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

  TGTextButton *exportText = new TGTextButton(hframe,"&Export Text");
  exportText->Connect("Clicked()","MonitorFrame",this,"ExportText()");
  exportText->SetFont(ft);
  hframe->AddFrame(exportText, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
*/
  TGTextButton *exit = new TGTextButton(hframe,"&Exit");
  exit->Connect("Clicked()", "MonitorFrame",this,"Exit()");
  exit->SetFont(ft);
//  exit->Connect("Clicked()", "TApplication", gApplication, "Terminate(0)");
  hframe->AddFrame(exit, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  AddFrame(hframe, new TGLayoutHints(kLHintsCenterX | kLHintsTop | kLHintsExpandY,2,2,2,2));


// Create a horizontal frame widget with text displays
  TGHorizontalFrame *hframeT = new TGHorizontalFrame(this,250,60);

  TGLabel *nEvtAcqL = new TGLabel(hframeT, "N acq.: ");
  nEvtAcqL->SetTextFont(ft);
  hframeT->AddFrame(nEvtAcqL, new TGLayoutHints(kLHintsCenterX,5,1,3,4));

  nEvtAcqT = new TGLabel(hframeT, Form("%-15i", 0));
  nEvtAcqT->SetTextFont(ft);
  hframeT->AddFrame(nEvtAcqT, new TGLayoutHints(kLHintsCenterX,1,5,3,4));

  TGLabel *nEvtProL = new TGLabel(hframeT, "N proc.: ");
  nEvtProL->SetTextFont(ft);
  hframeT->AddFrame(nEvtProL, new TGLayoutHints(kLHintsCenterX,5,1,3,4));

  nEvtProT = new TGLabel(hframeT, Form("%-15i", 0));
  nEvtProT->SetTextFont(ft);
  hframeT->AddFrame(nEvtProT, new TGLayoutHints(kLHintsCenterX,1,5,3,4));

  TGLabel *rateL = new TGLabel(hframeT, "Rate: ");
  rateL->SetTextFont(ft);
  hframeT->AddFrame(rateL, new TGLayoutHints(kLHintsCenterX,5,1,3,4));

  rateT = new TGLabel(hframeT, "0.000 evt/s");
  rateT->SetTextFont(ft);
  hframeT->AddFrame(rateT, new TGLayoutHints(kLHintsCenterX,1,5,3,4));

  AddFrame(hframeT, new TGLayoutHints(kLHintsCenterX | kLHintsTop | kLHintsExpandY,2,2,2,2));


  // Create a horizontal frame widget with displays of board status
  TGHorizontalFrame *hframeB = new TGHorizontalFrame(this,250,60);

  TGLabel *temperatureL = new TGLabel(hframeB, "T = ");
  temperatureL->SetTextFont(ft);
  hframeB->AddFrame(temperatureL, new TGLayoutHints(kLHintsCenterX,5,1,3,4));

  temperatureT = new TGLabel(hframeB, Form("%.1f\xB0", drs->GetBoard(0)->GetTemperature()));
  temperatureT->SetTextFont(ft);
  hframeB->AddFrame(temperatureT, new TGLayoutHints(kLHintsCenterX,1,5,3,4));

  TGTextButton *refresh = new TGTextButton(hframeB,"&Refresh");
  refresh->Connect("Clicked()","MonitorFrame",this,"RefreshT()");
  refresh->SetFont(ft);
  hframeB->AddFrame(refresh, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

  AddFrame(hframeB, new TGLayoutHints(kLHintsCenterX | kLHintsTop | kLHintsExpandY,2,2,2,2));

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

  std::cout << "Cleanup main frame.\n";

  for(int iobs=0; iobs<nObservables; iobs++) {
    delete histo[0][iobs];
    histo[0][iobs] = NULL;
    delete histo[1][iobs];
    histo[1][iobs] = NULL;
  }

  delete eTot12;      eTot12 = NULL;
  delete ePrompt12;   ePrompt12 = NULL;
  delete time12;      time12 = NULL;
  delete time34;      time34 = NULL;

  if (fCanvas01) delete fCanvas01; fCanvas01 = NULL;
  if (frCanvas01) delete frCanvas01;
  if (fCanvas02) delete fCanvas02;
  if (frCanvas02) delete frCanvas02;
  if (fCanvas2D) delete fCanvas2D;
  if (frCanvas2D) delete frCanvas2D;
  if (fCanvasOsc) delete fCanvasOsc;
  if (frCanvasOsc) delete frCanvasOsc;


  if (writer)    { delete writer;    writer    = NULL; }
  if (fifo)      { delete fifo;      fifo      = NULL; }

  if (drs)       {
    // Make sure board(s) are idle before exiting
    for (int iboard = 0; iboard<drs->GetNumberOfBoards(); iboard++) {
      drs->GetBoard(iboard)->Reinit();
    }
    delete drs;       drs       = NULL;
  }

  if (headers)   { delete headers;   headers   = NULL; }
  if (rawWave)   { delete rawWave;   rawWave   = NULL; }
  if (event)     { delete event;     event     = NULL; }
}



void MonitorFrame::Exit() {

  HardStop();

  gApplication->SetReturnFromRun(true);
  gApplication->Terminate(0);
}


void MonitorFrame::Start() {

  if (f_running) { return; }

  std::cout << "Starting Monitor frame.\n";

  if(!drs) {
    std::cout << "MonitorFrame::Start() - ERROR: DRS object empty." << std::endl;
    return;
  }

  if(drs->GetNumberOfBoards() < 1) {
    std::cout << "MonitorFrame::Start() - ERROR: No DRS boards present." << std::endl;
    return;
  }


  if(writer) {
    if (writer->isRunning() || writer->isJoinable()) {
      std::cout << "WARNING: Attempt to start while writer is running.!" << std::endl;
      return;
    }
    delete writer;
  }
  fifo->Discard();
  writer = new DRS4_writer(drs, fifo);
  if(options->triggerSource == 0) {
    writer->setAutoTrigger();
  }
  temperatureT->SetText(Form("%.1f\xB0", writer->Temperature()));

  /*** Clear histograms ***/
  for(int iobs=0; iobs<nObservables; iobs++) {
    histo[0][iobs]->Reset();
    histo[1][iobs]->Reset();
  }
  eTot12->Reset();
  ePrompt12->Reset();
  time12->Reset();
  time34->Reset();



  timer.Start();
  timeLastSave = 0;
  iEvtProcessed=0;


  headers = DRS4_data::DRSHeaders::MakeDRSHeaders(drs);
  if (StartNewFile() != 0) {
    writer->stop();
    delete writer;
    writer = NULL;
    return;
  }

  rate = new MonitorFrame::RateEstimator();
  rate->Push(0, 1.e-9); // One false time value to avoid division by zero
  DoDraw(true);

  /*** Start writer ***/
  writer->start(nEvtMax);
  while (!writer->isRunning()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); };

  /*** Start monitor ***/
  Run();

  /*** Cleanup after the run finishes ***/
  if (file) {
    if (file->is_open()) {
      file->close();
    }
    delete file;
    file = NULL;
  }
  f_running = false;

  if (rate)      { delete rate;      rate      = NULL; }
  if (headers)   { delete headers;   headers   = NULL; }
  if (writer)    { delete writer;    writer    = NULL; }

}


int MonitorFrame::Run() {

  std::cout << "Monitor on CPU " << sched_getcpu() << "\n";

  f_stop = false;
  f_stopWhenEmpty = false;
  f_running = true;

  // Time calibrations
  const bool tCalibrated = true; // Whether time points should come calibrated
  const bool tRotated = true;    // Whether time points should come rotated
  // Voltage calibrations
  const bool applyResponseCalib = true;  // Remove noise and offset variations
  const bool adjustToClock = false;      // Extra rotation of amplitudes in the calibration step - SET TO FALSE !
  const bool adjustToClockForFile = false;
  const bool applyOffsetCalib = false;   // ?

  unsigned iEvtLocal = 0;

  while(!f_stop) {

    rawWave = fifo->read();

    if(rawWave) {
      iEvtSerial = rawWave->header.getEventNumber();
      iEvtLocal++;
      if (iEvtLocal%10000 == 0) {
        StartNewFile();
      }
 //     std::cout << "Read event #" << iEvtSerial << std::endl;
 //     std::cout << "Trigger cell is " << rawWave->header.getTriggerCell() << std::endl;
      event = new DRS4_data::Event(iEvtSerial, rawWave->header, drs);

      Observables *obs[2] = {NULL, NULL};

      int16_t wf[4][kNumberOfBins];

      for(int iboard=0; iboard<headers->ChTimes()->size(); iboard++) {

        DRSBoard *b = drs->GetBoard(iboard);

        /* decode waveform (Y) arrays in mV */
        for (unsigned char ichan=0 ; ichan<4 ; ichan++) {
          b->GetWave(rawWave->eventWaves.at(iboard)->waveforms, 0, ichan*2, static_cast<short*>(wf[ichan]),
                     applyResponseCalib, int(rawWave->header.getTriggerCell()), -1,
                     adjustToClockForFile, 0, applyOffsetCalib);
        }

        RemoveSpikes(wf, 20, 2);

        for (unsigned char ichan=0 ; ichan<4 ; ichan++) {

          float timebins[kNumberOfBins];
          b->GetTime(0, ichan*2, int(rawWave->header.getTriggerCell()), timebins, tCalibrated, tRotated);

          float amplitude[kNumberOfBins];
          for (unsigned ibin=0; ibin<kNumberOfBins; ibin++) {
            amplitude[ibin] = static_cast<float>(wf[ichan][ibin]) / 10;
            event->getChData(iboard, ichan)->data[ibin] = wf[ichan][ibin] ;
          }

          if (ichan<2 && iboard==0) {
            obs[ichan] = WaveProcessor::ProcessOnline(timebins, amplitude, kNumberOfBins, 4., baseLineWidth);
            obs[ichan]->hist->SetName(Form("Oscillogram_ch%d", ichan+1));
          }

        } // Loop over the channels
      } // Loop over the boards

      iEvtProcessed++;


      FillHistos(obs);
      if(obs[0]) {
        delete obs[0]; obs[0] = NULL;
      }
      if(obs[1]) {
        delete obs[1]; obs[1] = NULL;
      }

      event->write(file);
      delete event; event = NULL;
      delete rawWave; rawWave = NULL;
      if(iEvtProcessed%500 == 0) {
        std::cout << "Processed event #" << iEvtProcessed << std::endl;
      }
    } // If rawWave (fifo not empty)
    else {
      if( f_stopWhenEmpty ) {
        f_stop = true;
        break;
      }
      else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      } // if(f_stopWhenEmpty)
    } // if(rawWave)

    gClient->ProcessEventsFor(this);
    if (!writer->isRunning()) {
      f_stopWhenEmpty = true;
    }

  } // !f_stop

  DoDraw(true);

  std::cout << "Events processed: " << iEvtProcessed << "\n";
  std::cout << Form("Elapsed time: %6.2f s.\n", timer.RealTime());
  std::cout << Form("Event rate: %6.2f events/s \n", float(iEvtProcessed)/timer.RealTime());

  f_running = false;
  return 0;

}


void MonitorFrame::Stop() {

  if(!f_running) return;
  if (f_stopWhenEmpty) return;

  std::cout << "\n\nStopping acquisition.\n";
  writer->stop();

  timer.Stop();
  f_stopWhenEmpty = true;
}


void MonitorFrame::HardStop() {

  if(!f_running) return;
  if(f_stop) return;

  std::cout << "\n\nStopping acquisition and monitor.\n";
  writer->stop();
  timer.Stop();
  f_stop = true;
  f_stopWhenEmpty = true;
  fifo->Discard();
}


void MonitorFrame::RefreshT() {
  double t;
  if (writer) {
    t = writer->Temperature();
  }
  else {
    t = drs->GetBoard(0)->GetTemperature();
  }
  temperatureT->SetText(Form("%.1f\xB0", t));
}





void MonitorFrame::FillHistos(Observables *obs[2])
{

  for(unsigned ichan=0; ichan<2; ichan++) {
    for(int iobs=0; iobs<nObservables; iobs++) {
      histo[ichan][iobs]->Fill(obs[ichan]->Value(static_cast<kObservables>(iobs)));
    }
  }

  eTot12->Fill(obs[0]->Value(eTot), obs[1]->Value(eTot));
  ePrompt12->Fill(obs[0]->Value(ePrompt), obs[1]->Value(ePrompt));
  time12->Fill(obs[0]->Value(arrivalTime), obs[1]->Value(arrivalTime));

  // TODO: calculate and process t3 and t4

  itRed++; itRed2D++;
  if (itRed >= tRed) {

    itRed=0;
    bool draw2D = false;
    if(itRed2D >= tRed2D) {
      itRed2D=0;
      itRed=0;
      draw2D = true;
    }
    DoDraw(draw2D);

    fCanvasOsc->cd();
    // Remove previous oscillograms
    TList *listp = gPad->GetListOfPrimitives();
    TObject *last = listp->Last();
    if (last->IsA() == obs[0]->hist->IsA()) {
      listp->RemoveLast();
      delete dynamic_cast<TH1F*>(last);
      last = listp->Last();
      if (last->IsA() == obs[0]->hist->IsA()) {
        listp->RemoveLast();
        delete dynamic_cast<TH1F*>(last);
      }
    }

 //   obs[0]->hist->Rebin(3);
    obs[0]->hist->SetLineColor(kBlue);
    obs[0]->hist->Scale(-1);
    obs[0]->hist->DrawCopy("same hist l")->SetBit(kCanDelete);

 //   obs[1]->hist->Rebin(3);
    obs[1]->hist->SetLineColor(kRed);
    obs[1]->hist->Scale(-1);
    obs[1]->hist->DrawCopy("same hist l")->SetBit(kCanDelete);

    fCanvasOsc->Update();
  }

} // FillHistos()





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


  unsigned nAcq = writer->NEvents();
  unsigned timeLast = fifo->timeLastEvent();
  nEvtAcqT->SetText(Form("%-10i", nAcq));
  nEvtProT->SetText(Form("%-10i", iEvtProcessed));
  rate->Push(nAcq, static_cast<double>(timeLast)/1000);
  rateT->SetText(Form("%5.3g evt/s", rate->Get()));
  temperatureT->SetText(Form("%.1f\xB0", writer->Temperature()));
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
//      gApplication->Terminate(0);
  }

  for(int iobs=0; iobs<nObservables; iobs++) {
    histo[0][iobs]->Clone()->Write();
    histo[1][iobs]->Clone()->Write();
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
  int nEvtRate = rateCounts.back() - rateCounts.front();
  if (nEvtRate) {
    evtRate = double(nEvtRate) / (rateTimes.back() - rateTimes.front());
  }
  else {
    evtRate = 0;
  }
  if(nEvtRate >= rateCountPeriod || rateCounts.size() > maxSize) {
    rateCounts.pop();
    rateTimes.pop();
  }
}



int MonitorFrame::StartNewFile() {

  if (file) {
    if (file->is_open()) {
      file->close();
    }
  }
  else {
    file = new std::ofstream();
  }


  filename = basename;
  TTimeStamp ts(std::time(NULL), 0);
  filename = basename;
  filename += "_";
  filename += ts.GetDate(0);
  filename += "-";
  filename += ts.GetTime(0);
  filename += ".dat";

  file->open(filename, std::ios_base::binary & std::ios_base::trunc) ;

  if( file->fail() ) {
    std::cout << "ERROR: Cannot open file " << filename << " for writing.\n";
    return -1;
  }

  /*** Write file header and time calibration ***/
  std::cout << "Writing headers and time calibration to file." << std::endl;
  if (!headers) {
    headers = DRS4_data::DRSHeaders::MakeDRSHeaders(drs);
  }
  headers->write(file);

  return 0;
}
