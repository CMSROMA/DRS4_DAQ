/*
 * DRS4_writer.h
 *
 * Declaration of class DRS4_writer responsible for the communication with
 * the DRS4 board and transfer of the data (writing) to RAM.
 *
 *  Created on: Apr 6, 2017
 *      Author: S. Lukic
 */

#ifndef DRS4DATA_INCLUDE_DRS4_WRITER_H_
#define DRS4DATA_INCLUDE_DRS4_WRITER_H_

#include "DRS.h"
#include "DRS4_fifo.h"


class DRS4_writer {

public:
  DRS4_writer(DRS *const, DRS4_fifo *const, const unsigned _nEvtMax = -1);

  void run();
  void stop() { f_stop = true; }


private:

  DRS4_writer();

  DRS *drs;
  DRSBoard *board;

  DRS4_fifo *const fifo;
  DRS4_data::Event *event;

  // Event counter
  unsigned nEvents;
  const unsigned nEvtMax;

  bool f_stop;

};



#endif /* DRS4DATA_INCLUDE_DRS4_WRITER_H_ */
