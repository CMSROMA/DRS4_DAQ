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

DRS4_writer::DRS4_writer(DRS *const _drs, DRS4_fifo *const _fifo,
                         std::vector<int> _nChans, const unsigned _nEvtMax) :
  drs(_drs), board(NULL), nChans(_nChans),
  fifo(_fifo), event(NULL), iEvent(0), nEvtMax(_nEvtMax), f_stop(false)
{
}

void DRS4_writer::run() {

  while(!f_stop && iEvent<nEvtMax) {

    event = new DRS4_data::Event(iEvent, int(floor( (drs->GetBoard(0)->GetCalibratedInputRange())*1000 + 0.5)) );

    for (int iboard=0; iboard<drs->GetNumberOfBoards(); iboard++) {


    } // Loop over the boards

    iEvent++;
  } // Loop over events
}
