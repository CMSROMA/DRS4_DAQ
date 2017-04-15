/*****************************************
 * DRS4_reader.cxx
 *
 * Contents:
 * Implementation of class DRS4_reader
 *
 * Created on: Apr 7, 2017
 *      Author: S. Lukic
 ****************************************/

#include "DRS4_reader.h"
#include <chrono>
#include <thread>
#include <iostream>

#include "TCanvas.h"
#include "TGraph.h"
#include "TH1F.h"
#include "TF1.h"
#include "TLatex.h"
#include "TString.h"

DRS4_reader::DRS4_reader(DRS4_data::DRS4_fifo *const _fifo, DRS* _drs) :
  drs(_drs), fifo(_fifo), rawWave(NULL), event(NULL),
  headers(NULL), file(NULL),
  f_stop(false), f_stopWhenEmpty(false)
{
  std::cout << "DRS4_reader::DRS4_reader()." << std::endl;

  // Objects for the initialization of the DRS headers
  DRS4_data::FHEADER fheader(drs->GetBoard(0)->GetDRSType());
  std::vector<DRS4_data::BHEADER*> bheaders;
  DRS4_data::ChannelTimes *chTimes = new DRS4_data::ChannelTimes;

  /*** Get board serial numbers and time bins ***/
  for (int iboard=0; iboard<drs->GetNumberOfBoards(); iboard++) {

    std::cout << "DRS4_reader::DRS4_reader() - Reading board #" << iboard << std::endl;

    DRSBoard *b = drs->GetBoard(iboard);

    // Board serial numbers
    DRS4_data::BHEADER *bhdr = new DRS4_data::BHEADER(b->GetBoardSerialNumber());
    bheaders.push_back(bhdr);

    std::vector<DRS4_data::ChannelTime*> chTimeVec;

    for (int ichan=0 ; ichan<4 ; ichan++) {

      DRS4_data::ChannelTime *ct = new DRS4_data::ChannelTime(ichan+1);
      // Get time bins
      b->GetTime(iboard, ichan*2, b->GetTriggerCell(iboard), ct->tbins);

      chTimeVec.push_back(ct);
    } // End loop over channels

    chTimes->push_back(chTimeVec);

  } // End loop over boards

  headers = new DRS4_data::DRSHeaders(fheader, bheaders, chTimes);
}

DRS4_reader::~DRS4_reader() {
  file->close();
  delete file;
}


void DRS4_reader::stop() {
  std::cout << "Stopping DRS4_reader.\n";
  f_stop = true;
}

void DRS4_reader::stopWhenEmpty() {
  std::cout << "Reading remaining events before stopping DRS4_reader.\n";
  f_stopWhenEmpty = true;
}


int DRS4_reader::run(const char *filename, DRS4_writer *writer) {

  file = new std::ofstream(filename, std::ios_base::binary & std::ios_base::trunc) ;

  if( file->fail() ) {
    std::cout << "ERROR: Cannot open file " << filename << " for writing.\n";
    return -1;
  }

  std::cout << "Starting DRS4_reader.\n";

  /*** Write file header and time calibration ***/
  std::cout << "Writing headers and time calibration." << std::endl;

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

  TCanvas c("wf", "Waveforms", 1600, 1200);
  c.Divide(2,2);
  bool firstpage=true;

  TGraph *g;

  // Time calibrations
  const bool tCalibrated = true; // Whether time points should come calibrated
  const bool tRotated = true;    // Whether time points should come rotated
  // Voltage calibrations
  const bool applyResponseCalib = true;  // Remove noise and offset variations
  const bool adjustToClock = false;      // Extra rotation of amplitudes in the calibration step - SET TO FALSE !
  const bool adjustToClockForFile = false;
  const bool applyOffsetCalib = false;   // ?

  while(!f_stop) {

    rawWave = fifo->read();

    if(rawWave) {
      unsigned iEvt = rawWave->header.getEventNumber();
      std::cout << "Read event #" << iEvt << std::endl;
      std::cout << "Trigger cell is " << rawWave->header.getTriggerCell() << std::endl;
      event = new DRS4_data::Event(iEvt, rawWave->header, drs);

      /* Raw waveform */
/*      int raw[kNumberOfChipsMax * kNumberOfChannelsMax * kNumberOfBins];
      for (int i=0; i<kNumberOfChipsMax * kNumberOfChannelsMax * kNumberOfBins; i++) {
        raw[i] = ((rawWave->eventWaves.at(0)->waveforms[i * 2 + 1 ] & 0xff) << 8)
               +  rawWave->eventWaves.at(0)->waveforms[i * 2 ];
      }*/

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
          c.cd(int(ichan)+1);

          TGraph *gr = new TGraph(kNumberOfBins, time, amplitude);
//          gPad->SetGridx(5);
  //        TGraph *gr = new TGraph(51, time, amplitude);
    /*      double x, y;
          gr->GetPoint(0, x, y);
          std::cout << "Time(0) = " << x << ", y(0) = " << y << std::endl;
          gr->GetPoint(50, x, y);
          std::cout << "Time(50) = " << x << ", y(50) = " << y <<  std::endl;*/
   //       frm->Draw("");
          gr->SetMarkerStyle(8);
          gr->GetHistogram()->SetMinimum(-600);
          gr->GetHistogram()->SetMaximum(600);
     /*     gr->GetHistogram()->GetXaxis()->SetNdivisions(20, 2, 0, kFALSE);*/
          gr->GetHistogram()->GetXaxis()->SetRangeUser(0, 200.);
          gr->Draw("AL");
          TF1 *f = new TF1("f", "[0]*sin([1]*x+[2])", 0, 200);
          f->SetParameter(0, 500);
          f->FixParameter(1, 0.1*2*M_PI);
          f->SetParameter(2, 0.);
    //      gr->Fit(f, "", "", 0, 200.);
          f->ReleaseParameter(1);
      //    gr->Fit(f, "", "", 0, 200.);
          //f->Draw("same");
          TLatex *tl = new TLatex(5, 520., Form("#nu = %.3f MHz", f->GetParameter(1)*1000/2/M_PI));
          TLatex *ttc = new TLatex(5, 40., Form("tc_{headers} = %d, tc_{board} = %d",
              rawWave->header.getTriggerCell(), b->GetTriggerCell(rawWave->eventWaves.at(iboard)->waveforms, 0)));
        //  tl->Draw();
        } // Loop over the channels

        if(firstpage) {
          c.Print("test.pdf(");
          firstpage = false;
        }
        else c.Print("test.pdf");

        c.Clear("D");

      }
      event->write(file);
      processEvent();
      delete event;
    }
    else {
      if(f_stopWhenEmpty || !writer->isRunning()) {
        f_stop = true;
      }
      else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      } // if(f_stopWhenEmpty)
    } // if(event)
  } // !f_stop

  c.Print("test.pdf)");

  return 0;
}

void DRS4_reader::processEvent() {

}
