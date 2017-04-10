/*****************************************
 * DRS4_reader.cxx
 *
 * Contents:
 * Implementation of class DRS4_reader
 *
 * Created on: Apr 7, 2017
 *      Author: S. Lukic
 ****************************************/

#include "DRS4_reader.h"
#include <chrono>
#include <thread>
#include <iostream>

DRS4_reader::DRS4_reader(DRS4_fifo *const _fifo) :
fifo(_fifo), event(NULL), file(NULL),
f_stop(false), f_stopWhenEmpty(false)
{
}

DRS4_reader::~DRS4_reader() {
  file->close();
  delete file;
}


void DRS4_reader::stop() {
  std::cout << "Stopping DRS4_reader.\n";
  f_stop = true;
}

void DRS4_reader::stopWhenEmpty() {
  std::cout << "Reading remaining events before stopping DRS4_reader.\n";
  f_stopWhenEmpty = true;
}


int DRS4_reader::run(const char *filename) {

  file = new std::ofstream(filename, std::ios_base::binary & std::ios_base::trunc) ;

  if( file->fail() ) {
    std::cout << "ERROR: Cannot open file " << filename << " for writing.\n";
    return -1;
  }

  std::cout << "Starting DRS4_reader.\n";

  while(!f_stop) {

    event = fifo->read();

    if(event) {
      event->write(file);
      processEvent();
    }
    else {
      if(f_stopWhenEmpty) {
        f_stop = true;
      }
      else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      } // if(f_stopWhenEmpty)
    } // if(event)
  } // !f_stop

  return 0;
}

void DRS4_reader::processEvent() {

}
