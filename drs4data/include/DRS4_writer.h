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

#include <thread>


class DRS4_writer {

public:
  DRS4_writer(DRS *const, DRS4_data::DRS4_fifo *const);
  ~DRS4_writer();

  void start(const unsigned _nEvtMax = -1);
  void stop() ;
  void join() ;

  bool isRunning() { return f_isRunning; }

  void setAutoTrigger(bool at=true) { f_autoTrigger = at; }


private:

  DRS4_writer();

  DRS *drs;
  DRSBoard *board;

  DRS4_data::DRS4_fifo *const fifo;
  DRS4_data::RawEvent *event;

  // Event counter
  unsigned iEvent;

  std::thread *internalThread;
  // This has to be static for the compiler to accept it as argument for the thread
  static void run( DRS4_writer*, const unsigned _nEvtMax = -1);

  bool f_stop;
  bool f_isRunning;
  bool f_autoTrigger;

};



#endif /* DRS4DATA_INCLUDE_DRS4_WRITER_H_ */
