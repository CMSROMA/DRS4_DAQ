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


DRS4_writer::DRS4_writer(DRS *const _drs, DRS4_fifo *const _fifo,
                         std::vector<int> _nChans) :
  drs(_drs), board(NULL), nChans(_nChans),
  fifo(_fifo), event(NULL), iEvent(0),
  internalThread(NULL), f_stop(false), f_isRunning(false)
{
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

  w->f_isRunning = true;
  std::cout << "Starting DRS4_writer.\n";

  while(!w->f_stop && w->iEvent<nEvtMax) {

    w->event = new DRS4_data::Event(w->iEvent, int(floor( (w->drs->GetBoard(0)->GetCalibratedInputRange())*1000 + 0.5)) );

    for (int iboard=0; iboard<w->drs->GetNumberOfBoards(); iboard++) {

      // TODO: Acquire data here into event

    } // Loop over the boards

    w->iEvent++;
  } // Loop over events

  w->f_isRunning = false;
}
