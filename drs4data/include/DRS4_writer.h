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
#include <stdio.h>

#define LED_SCAN
#define LED_SCAN_PORT "/dev/usbtmc0"
#define LED_SCAN_START 2.875
#define LED_SCAN_STEP  0.015

class DRS4_writer {

public:
#ifdef LED_SCAN
   DRS4_writer(DRS *const, DRS4_data::DRS4_fifo *const,bool ledscan=0);
#else
   DRS4_writer(DRS *const, DRS4_data::DRS4_fifo *const);
#endif
  ~DRS4_writer();

  void start(const unsigned _nEvtMax = -1);
  void stop() ;
  void join() ;

  bool isRunning() const { return f_isRunning; }
  bool isJoinable();

  void setAutoTrigger(bool at=true) { f_autoTrigger = at; }

  unsigned NEvents() const { return iEvent; }

  void setSpillSize(unsigned spill) { 
    if (!f_isRunning)
      spillSize = spill;
    else
      return;
  }

  void setInterSpillTime(unsigned time) { 
    if (!f_isRunning)
      interSpillTime = time;
    else
      return;
  }

  void setLedScan(bool ledscan) { 
    if (!f_isRunning)
      ledScan=ledscan;
    else
      return;
  }

  double Temperature() {
    if (!f_isRunning)  return drs->GetBoard(0)->GetTemperature();
    else               return temperature;
}

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

  double temperature;
  
  unsigned spillSize;
  unsigned interSpillTime;

#ifdef LED_SCAN
  /* FILE* ledPort; */
  bool  ledScan;
#endif        
};



#endif /* DRS4DATA_INCLUDE_DRS4_WRITER_H_ */
