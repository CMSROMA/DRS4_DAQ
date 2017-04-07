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


class DRS4_reader{

public:
  DRS4_reader(DRS4_fifo *const, const char *filename);
  ~DRS4_reader(); // close file

  void run();
  void stop() { f_stop = true; }
  // Stop as soon as the fifo queue is empty
  void stopWhenEmpty() { f_stopWhenEmpty = true; }

private:

  DRS4_fifo *const fifo;
  DRS4_data::Event *event;

  std::ofstream *file;

  bool f_stop;
  bool f_stopWhenEmpty;

};




#endif /* DRS4DATA_INCLUDE_DRS4_READER_H_ */
