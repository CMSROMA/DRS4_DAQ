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

  int rawx[kNumberOfChipsMax * kNumberOfChannelsMax * kNumberOfBins];
  for(int i=0; i<kNumberOfChipsMax * kNumberOfChannelsMax * kNumberOfBins; i++) {
    rawx[i] = i;
  }


  while(!f_stop) {

    rawWave = fifo->read();

    if(rawWave) {
      unsigned iEvt = rawWave->header.getEventNumber();
      std::cout << "Read event #" << iEvt << std::endl;
      std::cout << "Trigger cell is " << rawWave->header.getTriggerCell() << std::endl;
      event = new DRS4_data::Event(iEvt, rawWave->header, drs);

      c.Clear();
      c.Divide(1,2);

          /* Raw waveform */
      int raw[kNumberOfChipsMax * kNumberOfChannelsMax * kNumberOfBins];
      for (int i=0; i<kNumberOfChipsMax * kNumberOfChannelsMax * kNumberOfBins; i++) {
        raw[i] = ((rawWave->eventWaves.at(0)->waveforms[i * 2 + 1 ] & 0xff) << 8)
               +  rawWave->eventWaves.at(0)->waveforms[i * 2 ];
      }
      TGraph gRaw(1 * kNumberOfChannelsMax * kNumberOfBins, rawx, raw);
      gRaw.Draw("AL");
      if(firstpage) {
        c.Print("test.pdf(");
        firstpage = false;
      }
      else c.Print("test.pdf");
      c.Clear();
      c.Divide(2,2);

      for(int iboard=0; iboard<headers->chTimes.size(); iboard++) {
        DRSBoard *b = drs->GetBoard(iboard);
        for (unsigned char ichan=0 ; ichan<4 ; ichan++) {


          /* decode waveform (Y) arrays in mV */
          std::cout << "Decoding waveform in chan #" << int(ichan)+1 << std::endl;
          b->GetWave(rawWave->eventWaves.at(iboard)->waveforms, 0, ichan*2, (short*)event->getChData(iboard, ichan)->data,
              true, int(rawWave->header.getTriggerCell()), -1, true);

          float time[kNumberOfBins];
          b->GetTime(0, ichan*2, int(rawWave->header.getTriggerCell()), time, true, true);
          float amplitude[kNumberOfBins];
          b->GetWave(rawWave->eventWaves.at(iboard)->waveforms, 0, ichan*2, amplitude,
              true, int(rawWave->header.getTriggerCell()), -1, false, 0, true);
//          b->GetWave(rawWave->eventWaves.at(iboard)->waveforms, 0, ichan*2, amplitude,
  //            true, b->GetTriggerCell(rawWave->eventWaves.at(iboard)->waveforms, 0), -1, false, 0, true);
          c.cd(int(ichan)+1);
          TGraph gr(kNumberOfBins, time, amplitude);
          gr.DrawGraph(kNumberOfBins, time, amplitude, "AL");
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
