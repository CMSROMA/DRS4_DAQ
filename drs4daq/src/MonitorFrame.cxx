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


#include "MonitorFrame.h"
#include "Riostream.h"
#include "TMath.h"

#include "DAQ-config.h"

#include "DRS4_fifo.h"
#include "DRS4_writer.h"
#include "DRS4_reader.h"
#include "observables.h"

using namespace DRS4_data;


MonitorFrame::MonitorFrame(const TGWindow *p, const config *opt, DRS *_drs) :
  TGMainFrame(p, 200, 200),
  limits(opt->histolo, opt->histohi),
  fCanvas01(new TCanvas("DRS4Canvas02", "DRS4 Monitor 01", opt->_w, opt->_h)),
  frCanvas01(new TRootCanvas(fCanvas01, "DRS4 Monitor 01", 0, 200, opt->_w, opt->_h)),
  fCanvas02(new TCanvas("DRS4Canvas02", "DRS4 Monitor 02", opt->_w, opt->_h)),
  frCanvas02(new TRootCanvas(fCanvas02, "DRS4 Monitor 02", 0, 200, opt->_w, opt->_h)),
  tRed(opt->_tRed),
  basename("default"), filename("default"), timestamped(false),
  obs(new Observables),
  eTot12(NULL), ePrompt12(NULL),
  time12(NULL), time34(NULL),
  timeLastSave(0),
  rate(NULL),
  drs(_drs), fifo(NULL), writer(NULL), reader(NULL),
  nEvents(100000),
  itRed(0),
  log(NULL)
{

  for(kObservables iobs=0; iobs<nObservables; iobs++) {
    histo[iobs] = new TH1F(Form("h%d", iobs), Form("%s; %s (%s)", obs->Name(iobs), obs->Name(iobs), obs->Unit(iobs)), 100, opt->histolo[iobs], opt->histohi[iobs]);
  }

  eTot12(new TH2F("eTot12", Form("eTot2 vs. eTot1; %s_{1} (%s); %s_{2} (%s)", obs->Name(eTot), obs->Unit(eTot), obs->Name(eTot), obs->Unit(eTot)),
      (opt->histohi[eTot]-opt->histolo[eTot])/opt->_xRed, opt->histolo[eTot], opt->histohi[eTot],
      (opt->histohi[eTot]-opt->histolo[eTot])/opt->_xRed, opt->histolo[eTot], opt->histohi[eTot]));
  ePrompt12(new TH2F("ePrompt12", Form("ePrompt2 vs. ePrompt1; %s_{1} (%s); %s_{2} (%s)", obs->Name(ePrompt), obs->Unit(ePrompt), obs->Name(ePrompt), obs->Unit(ePrompt)),
      (opt->histohi[ePrompt]-opt->histolo[ePrompt])/opt->_xRed, opt->histolo[ePrompt], opt->histohi[ePrompt],
      (opt->histohi[ePrompt]-opt->histolo[ePrompt])/opt->_xRed, opt->histolo[ePrompt], opt->histohi[ePrompt]));
  time12(new TH2F("time12", Form("t2 vs. t1; %s_{1} (%s); %s_{2} (%s)", obs->Name(tArrival), obs->Unit(tArrival), obs->Name(tArrival), obs->Unit(tArrival)),
      (opt->histohi[tArrival]-opt->histolo[tArrival])/opt->_xRed, opt->histolo[tArrival], opt->histohi[tArrival],
      (opt->histohi[tArrival]-opt->histolo[tArrival])/opt->_xRed, opt->histolo[tArrival], opt->histohi[tArrival]));
  time34(new TH2F("time34", "t4 vs. t3; t_{3} (ns); t_{4} (ns)", 100, 0., 100., 100, 0., 100.));


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
	for( unsigned ih=0; ih<nObservables; ih++) {
	  fCanvas01->cd(1); histo[ih]->Draw();
	}

	fCanvas02->Divide(2, 2, .002, .002);
  fCanvas02->cd(1); eTot12->Draw();
  fCanvas02->cd(2); ePrompt12->Draw();
  fCanvas02->cd(3); time12->Draw();
  fCanvas02->cd(4); time34->Draw();
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

  for(kObservables iobs=0; iobs<nObservables; iobs++) {
    delete histo[iobs];
    histo[iobs] = NULL;
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

	if(reader) {
	  delete reader;
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

  fifo = new DRS4_data::DRS4_fifo;
  writer(drs, fifo);
  reader(fifo, drs);

  writer->start(nEvents);
  while (!writer->isRunning()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); };

  /*** Clear histograms ***/
  for(kObservables iobs=0; iobs<nObservables; iobs++) {
    histo[iobs]->Reset();
  }
  eTot12->Reset();
  ePrompt12->Reset();
  time12->Reset();
  time34->Reset();



	filename(basename);
	timestamped=false;


	rate = new MonitorFrame::RateEstimator();
	rate->Push(0, 1.e-9); // One false time value to avoid division by zero
	timer.Start();
	timeLastSave = 0;


	// FIXME: The monitor frame should BE the reader
  int readerState = reader->run(filename.Data(), writer);
  if ( readerState < 0 ) {
    writer->stop();
    std::cout << "MonitorFrame::Start() - ERROR Starting reader." << std::endl;
    return;
  }

	Run();
}

void MonitorFrame::Run() {

	while (iEvt < nEvents) {

		int nBulkReadAttempts = 0;

		int nbytes = -1;
		while(nbytes < 0)
		{
			gClient->ProcessEventsFor(this);
			if(!cc->DAQisRunning()) return;

			nbytes = FillBuffer();
			nBulkReadAttempts++;

/*			if (nBulkReadAttempts > nBulkReadAttLimit) {
				std::cout << "No data from buffer in " << std::dec <<
						nBulkReadAttLimit << " attempts. Stopping.\n";
				Stop();
			}*/
		}
		if (nbytes > 0) { // Leaving the check in for future possibilities
			nBulkReadAttempts = 0;
			HandleData();
		}

	}

	Stop();

}


void MonitorFrame::Stop() {
	if(!cc->DAQisRunning()) { return; }

	std::cout << "\n\nStopping.\n";
	timer.Stop();
	DoDraw(true);
	if ( cc->DAQisRunning() ) {
		short nbytes = cc->StopDAQ(buffer, bufferSize);
    std::cout << "Drained " << nbytes << " bytes from buffer.\n";
//    std::cout << "buffer[0] reports " << buffer[0] << " events.\n";
    int nevents = ((nbytes/2)-2)/(nChanADC + nChanTDC + 1);
    buffer[0] = nevents;
    std::cout << "Wrote number of events N=" << nevents << " to buffer header.\n";
    if (nbytes > 0) { // Leaving the check in for future possibilities
      HandleData();
    }
		if (nbytes < 0) std::cout << "Error stopping DAQ.\n";
	}

	delete rate;
	rate = NULL;

	std::cout << "Events processed: " << iEvt << "\n";
	std::cout << Form("Elapsed time: %6.2f s.\n", timer.RealTime());
	std::cout << Form("Counting rate: %6.2f events/s \n", float(iEvt)/timer.RealTime());

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
	short newEvents = buffer[0];
	unsigned int idxEvent = 1;

	for (int i=0; i<newEvents; i++)
	{
		unsigned short lenEvt = buffer[idxEvent];
/*		if( lenEvt < 1 ) {
			std::cout << "Event length " << lenEvt << "; Event " << iEvt+i << " !\n";
			continue;
		}
*/
		for(int i=0; i<nChanADC; i++) { adc[i]=0; }
		for(int i=0; i<nChanTDC; i++) { tdc[i]=0; }

		unsigned short idxData = idxEvent+1;
		unsigned short lastADC = idxEvent + TMath::Min(nChanADC, lenEvt);
		for(int i=0; i<nChanADC && idxData <= lastADC; i++, idxData++) {
			adc[i] = buffer[idxData]%4096;
			histo_adc[i]->Fill(adc[i]);
		}
		unsigned short lastTDC = lastADC + TMath::Min(nChanTDC, lenEvt);
		for(int i=0; i<nChanTDC && idxData <= lastTDC; i++, idxData++) {
			tdc[i] = buffer[idxData];
			histo_tdc[i]->Fill(tdc[i]);
		}

		events->Fill();

		adc12->Fill(adc[0], adc[1]);
		tdc12->Fill(tdc[0], tdc[1]);
		adc34->Fill(adc[2], adc[3]);
		tdc34->Fill(tdc[2], tdc[3]);

		idxEvent += lenEvt+1;
		iEvt++;
	} // End buffer

	timer.Stop();

	rate->Push(iEvt, timer.RealTime());

	itRed++;
	if(itRed >= tRed) { itRed=0; DoDraw(true); }
	else { DoDraw(); }

	bufferFresh = false;

	if (timer.RealTime() - timeLastSave> autosavePeriod) {
		AutoSave();
	}
	timer.Continue();

} // HandleData()




int MonitorFrame::FillBuffer() {
//	std::cout << "Reading CC-USB.\n";
	int nbytes = cc->GetData(buffer, bufferSize);
	bufferFresh=true;
	return nbytes;
}


void MonitorFrame::DoDraw(bool all) {
// Draws function graphics in randomly chosen interval

	if(all) {
		fCanvas01->Paint();
		fScin->Paint();
		std::cout << std::dec << "Events: " << iEvt << "\r" << std::flush;
	}
	else {
		fCanvas01->GetPad(1)->Paint();
		fCanvas01->GetPad(2)->Paint();
		fCanvas01->GetPad(4)->Paint();
		fCanvas01->GetPad(5)->Paint();
		fScin->GetPad(1)->Paint();
		fScin->GetPad(2)->Paint();
		fScin->GetPad(4)->Paint();
		fScin->GetPad(5)->Paint();
	}
	fCanvas01->Update();
	fScin->Update();

	nEvtT->SetText(Form("%-10i", iEvt));
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
	filename(basename);
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

	events->CloneTree()->Write();
	for(int i=0; i<nChanADC; i++) { histo_adc[i]->Clone()->Write(); }
	for(int i=0; i<nChanTDC; i++) { histo_tdc[i]->Clone()->Write(); }

	adc12->Clone()->Write();
	tdc12->Clone()->Write();
	adc34->Clone()->Write();
	tdc34->Clone()->Write();
	output.Close();
	std::cout << "\nSaved data in " << rootfilename.Data() << "\n";
	timeLastSave = timer.RealTime();
	timer.Continue();
}


void MonitorFrame::ExportText() {

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


	for (int i=0; i<nChanADC; i++) { exportfile << Form("  ADC%i  ", i+1); }
	for (int i=0; i<nChanTDC; i++) { exportfile << Form("  TDC%i  ", i+1); }
	exportfile << "\n";
	for (int iEvt=0; iEvt<events->GetEntries(); iEvt++) {
		events->GetEvent(iEvt);
		for (int i=0; i<nChanADC; i++) { exportfile << Form("%8d", adc[i]); }
		for (int i=0; i<nChanTDC; i++) { exportfile << Form("%8d", tdc[i]); }
		exportfile << "\n";
	}
	exportfile.close();

	std::cout << "\nSaved text data in " << textfilename.Data() << "\n";
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
