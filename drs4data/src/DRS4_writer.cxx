/*****************************************
 * DRS4_writer.cxx
 *
 * Contents:
 * Implementation of class DRS4_writer
 *
 * Created on: Apr 7, 2017
 *      Author: S. Lukic
 ****************************************/

#include "math.h"
#include "DRS4_writer.h"
#include <iostream>
#include <cassert>
#include <cstdio>


DRS4_writer::DRS4_writer(DRS *const _drs, DRS4_fifo *const _fifo,
    DRS4_data::ChannelTimes * chTimes, std::vector<DRS4_data::BHEADER*> bheaders) :
  drs(_drs), board(NULL),
  fifo(_fifo), event(NULL), iEvent(0),
  internalThread(NULL), f_stop(false), f_isRunning(false)
{

  /*** Get board serial numbers and time bins ***/
  for (int iboard=0; iboard<drs->GetNumberOfBoards(); iboard++) {

    DRSBoard *b = drs->GetBoard(iboard);

    // Board serial numbers
    bheaders.at(iboard)->bn[0] = 'B';
    bheaders.at(iboard)->bn[1] = '#';
    bheaders.at(iboard)->board_serial_number = b->GetBoardSerialNumber();

    for (int ichan=0 ; ichan<4 ; ichan++) {
      chTimes->at(iboard).at(ichan)->ch.c[0] = 'C';
      // Format channel number in a c-string
      char chnum[4];
      snprintf(chnum, 4, "%03d", ichan);
      // Copy char values
      for (int ichar=0; ichar<3; ichar++)
        chTimes->at(iboard).at(ichan)->ch.cn[ichar] = chnum[ichar];
      // Get time bins
      b->GetTime(0, ichan*2, b->GetTriggerCell(0), chTimes->at(iboard).at(ichan)->tbins);
    }
  }
}


DRS4_writer::~DRS4_writer() {
  if (internalThread) {
    if(internalThread->joinable()) {
      stop();
    }
  }
}


void DRS4_writer::start(const unsigned nEvtMax) {

  f_stop = false;
  internalThread = new std::thread(DRS4_writer::run, this, nEvtMax);
}


void DRS4_writer::stop() {
  std::cout << "Stopping DRS4_writer.\n";
  f_stop = true;
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

    w->event = new DRS4_data::Event(w->iEvent, int(floor( (w->drs->GetBoard(0)->GetCalibratedInputRange())*1000 + 0.5)) );

    /* start boards (activate domino wave), master is last */
    for (int iboard=w->drs->GetNumberOfBoards()-1 ; iboard>=0 ; iboard--)
      w->drs->GetBoard(iboard)->StartDomino();

    std::cout << "DRS4_writer::run() Waiting for trigger." << std::endl;
    while (w->drs->GetBoard(0)->IsBusy());

    /*** Transfer and decode waveforms for all boards and channels ***/

    for (int iboard=0; iboard<w->drs->GetNumberOfBoards(); iboard++) {

      DRSBoard *b = w->drs->GetBoard(iboard);

      b->TransferWaves(0, 8);

      for (int ichan=0 ; ichan<4 ; ichan++) {
        /* decode waveform (Y) arrays in mV */
        b->DecodeWave(0, ichan, w->event->getChData(iboard, ichan)->data);
      } // Loop over the channels

    } // Loop over the boards

    w->iEvent++;
  } // Loop over events

  w->f_isRunning = false;
}
