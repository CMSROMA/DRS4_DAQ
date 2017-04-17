/*
 * DRS4_reader.h
 *
 * Declaration of class DRS4_reader responsible for reading events from RAM,
 * monitoring and storing to a binary file.
 *
 *  Created on: Apr 6, 2017
 *      Author: S. Lukic
 */

#ifndef DRS4DATA_INCLUDE_DRS4_READER_H_
#define DRS4DATA_INCLUDE_DRS4_READER_H_

#include <fstream>
#include "DRS4_fifo.h"
#include "DRS4_writer.h"


class DRS4_reader{

public:
  DRS4_reader(DRS4_data::DRS4_fifo *const, DRS *);
  ~DRS4_reader(); // close file

  int run(const char *filename, DRS4_writer*);
  void stop() ;
  // Stop as soon as the fifo queue is empty
  void stopWhenEmpty() ;

  unsigned getIEvtSerial() const { return iEvtSerial; }
  unsigned getIEvtProcessed() const { return iEvtProcessed; }

private:

  DRS4_reader();

  DRS *drs;

  DRS4_data::DRS4_fifo *const fifo;
  DRS4_data::RawEvent *rawWave;
  DRS4_data::Event *event;
  DRS4_data::DRSHeaders *headers;

  unsigned iEvtProcessed; // Number of events processed by the reader
  unsigned iEvtSerial;    // Serial number reported by the last processed event

  std::ofstream *file;

  bool f_stop;
  bool f_stopWhenEmpty;

  /* Monitoring */

  std::vector<float> times;
  std::vector<float> amplitudes;

  void processEvent();

};




#endif /* DRS4DATA_INCLUDE_DRS4_READER_H_ */
