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




DRS4_writer::DRS4_writer(DRS *const _drs, DRS4_data::DRS4_fifo *const _fifo) :
  drs(_drs), board(NULL),
  fifo(_fifo), event(NULL), iEvent(0),
  internalThread(NULL), f_stop(false), f_isRunning(false), f_autoTrigger(false)
{
  std::cout << "DRS4_writer::DRS4_writer()." << std::endl;
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

  double range = w->drs->GetBoard(0)->GetInputRange();

  w->f_isRunning = true;


  while(!w->f_stop && w->iEvent<nEvtMax) {

    w->event = new DRS4_data::RawEvent;
    w->event->header.setEvtNumber(w->iEvent+1);
    w->event->header.setRange(range);

    /* start boards (activate domino wave), master is last */
    for (int iboard=w->drs->GetNumberOfBoards()-1 ; iboard>=0 ; iboard--)
      w->drs->GetBoard(iboard)->StartDomino();

    /* If auto trigger specified, send auto trigger */
    if( w->f_autoTrigger ) {
      w->drs->GetBoard(0)->SoftTrigger();
      std::cout << "DRS4_writer::run() Soft trigger." << std::endl;
    }
    else {
      std::cout << "DRS4_writer::run() Waiting for trigger." << std::endl;
    }
    while (w->drs->GetBoard(0)->IsBusy());

    w->event->header.setTimeStamp();

    /*** Transfer waveforms for all boards ***/
    std::cout << "DRS4_writer::run() - Transferring waves." << std::endl;

    for (int iboard=0; iboard<w->drs->GetNumberOfBoards(); iboard++) {

//      std::cout << "Board #" << iboard << std::endl;
      DRSBoard *b = w->drs->GetBoard(iboard);
      DRS4_data::Waveforms *wf = new DRS4_data::Waveforms;

      b->TransferWaves(wf->waveforms, 0, 8);

      w->event->eventWaves.push_back(wf);

    } // Loop over the boards

    w->fifo->write(w->event);
    w->event = NULL; // Here we promise not to accidentally write in this raw event
    w->iEvent++;
    std::cout << "Done with event #" << w->iEvent << std::endl;
  } // Loop over events

  w->f_isRunning = false;
}
