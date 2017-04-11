/*****************************************
 * DRS4_writer.cxx
 *
 * Contents:
 * Implementation of class DRS4_writer
 *
 * Created on: Apr 7, 2017
 *      Author: S. Lukic
 ****************************************/

#include "DRS4_writer.h"
#include <iostream>
#include <cassert>
#include <cstdio>
#include <cstring>


DRS4_writer::DRS4_writer(DRS *const _drs, DRS4_fifo *const _fifo, DRS4_data::DRSHeaders* &headers) :
  drs(_drs), board(NULL),
  fifo(_fifo), event(NULL), iEvent(0),
  internalThread(NULL), f_stop(false), f_isRunning(false), f_autoTrigger(false)
{

  // Objects for the initialization of the DRS headers
  DRS4_data::FHEADER* fheader = new DRS4_data::FHEADER(drs->GetBoard(0)->GetDRSType());
  std::vector<DRS4_data::BHEADER*> bheaders;
  DRS4_data::ChannelTimes *chTimes = new DRS4_data::ChannelTimes;

  /*** Get board serial numbers and time bins ***/
  for (int iboard=0; iboard<drs->GetNumberOfBoards(); iboard++) {

    std::cout << "DRS4_writer::DRS4_writer() - Reading board #" << iboard << std::endl;

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

  headers = new DRS4_data::DRSHeaders(*fheader, bheaders, chTimes);
}


DRS4_writer::~DRS4_writer() {
  if (internalThread) {
    if(internalThread->joinable()) {
      stop();
    }
  }
}


void DRS4_writer::start(const unsigned nEvtMax) {

  std::cout << "DRS4_writer::start(" << nEvtMax << ")." << std::endl;
  f_stop = false;
  internalThread = new std::thread(DRS4_writer::run, this, nEvtMax);
}


void DRS4_writer::stop() {
  std::cout << "Stopping DRS4_writer.\n";
  f_stop = true;
  join();
}

void DRS4_writer::join() {
  std::cout << "Joining DRS4_writer.\n";
  internalThread->join();
  delete internalThread;
  internalThread = NULL;
}

void DRS4_writer::run( DRS4_writer* w, const unsigned nEvtMax) {

  std::cout << "Starting DRS4_writer.\n";

  assert(w->drs!=NULL);
  if ( w->drs->GetNumberOfBoards() < 1 ) {
    std::cout << "DRS4_writer::run() ERROR: No boards present.\n";
    w->f_isRunning = false;
    return;
  }

  w->f_isRunning = true;

  float dataBuffer[DRS4_data::nChansDRS4];




  while(!w->f_stop && w->iEvent<nEvtMax) {

    w->event = new DRS4_data::Event(w->iEvent+1, w->drs);

    /* start boards (activate domino wave), master is last */
    for (int iboard=w->drs->GetNumberOfBoards()-1 ; iboard>=0 ; iboard--)
      w->drs->GetBoard(iboard)->StartDomino();


    if( w->f_autoTrigger ) {
      w->drs->GetBoard(0)->SoftTrigger();
      std::cout << "DRS4_writer::run() Soft trigger." << std::endl;
    }
    else {
      std::cout << "DRS4_writer::run() Waiting for trigger." << std::endl;
    }
    while (w->drs->GetBoard(0)->IsBusy());

    /*** Transfer and decode waveforms for all boards and channels ***/
    std::cout << "DRS4_writer::run() - Transferring waves." << std::endl;

    std::cout << "Chan data has " << w->event->getNBoards() << " boards." <<  std::endl;

    for (int iboard=0; iboard<w->drs->GetNumberOfBoards(); iboard++) {

      std::cout << "Board #" << iboard << std::endl;
      DRSBoard *b = w->drs->GetBoard(iboard);

      b->TransferWaves(0, 8);

      for (int ichan=0 ; ichan<4 ; ichan++) {
        /* decode waveform (Y) arrays in mV */
        std::cout << "Decoding waveform in chan #" << ichan << std::endl;
        b->DecodeWave(0, ichan, w->event->getChData(iboard, ichan)->data);
      } // Loop over the channels

    } // Loop over the boards

    w->fifo->write(w->event);
    w->iEvent++;
    std::cout << "Done with event #" << w->iEvent << std::endl;
  } // Loop over events

  w->f_isRunning = false;
}
