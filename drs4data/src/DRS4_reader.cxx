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

DRS4_reader::DRS4_reader(DRS4_fifo *const _fifo, DRS4_data::ChannelTimes *_chTimes) :
fifo(_fifo), event(NULL), chTimes(_chTimes), file(NULL),
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


int DRS4_reader::run(const char *filename, std::vector<DRS4_data::BHEADER*> bheaders) {

  file = new std::ofstream(filename, std::ios_base::binary & std::ios_base::trunc) ;

  if( file->fail() ) {
    std::cout << "ERROR: Cannot open file " << filename << " for writing.\n";
    return -1;
  }

  std::cout << "Starting DRS4_reader.\n";

  /*** Write file header and time calibration ***/

  file->write("DRS4", 4);
  file->write("TIME", 4);

  for(int iboard=0; iboard<chTimes->size(); iboard++) {
    // Write board header
    file->write(bheaders.at(iboard)->bn, 2);
    file->write(reinterpret_cast<const char*>(&(bheaders.at(iboard)->board_serial_number)), 2);
    // Write time calibration
    for (int ichan=0; ichan<chTimes->at(iboard).size(); ichan++) {
      file->write(chTimes->at(iboard).at(ichan)->ch.c, 1);
      file->write(chTimes->at(iboard).at(ichan)->ch.cn, 3);
      file->write(reinterpret_cast<const char*>(chTimes->at(iboard).at(ichan)->tbins), DRS4_data::nChansDRS4*sizeof(float));
    }
  }


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
