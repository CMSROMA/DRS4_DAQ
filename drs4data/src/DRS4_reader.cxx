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

DRS4_reader::DRS4_reader(DRS4_fifo *const _fifo, const char *filename) :
fifo(_fifo), event(NULL), file(NULL),
f_stop(false), f_stopWhenEmpty(false)
{

}

DRS4_reader::~DRS4_reader() {
  file->close();
  delete file;
}

void DRS4_reader::run() {

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
}

void DRS4_reader::processEvent() {

}
