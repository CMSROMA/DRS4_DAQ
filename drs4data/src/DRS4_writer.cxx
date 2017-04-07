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

DRS4_writer::DRS4_writer(DRS *const _drs, DRS4_fifo *const _fifo, const unsigned _nEvtMax) :
drs(_drs), board(NULL), fifo(_fifo), event(NULL),
nEvents(0), nEvtMax(_nEvtMax), f_stop(false)
{
}

void DRS4_writer::run() {

}
