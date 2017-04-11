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

DRS4_reader::DRS4_reader(DRS4_fifo *const _fifo, DRS4_data::DRSHeaders* _headers) :
fifo(_fifo), event(NULL), headers(_headers), file(NULL),
f_stop(false), f_stopWhenEmpty(false)
{
  std::cout << "DRS4_reader::DRS4_reader()." << std::endl;
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


int DRS4_reader::run(const char *filename, DRS4_writer *writer) {

  file = new std::ofstream(filename, std::ios_base::binary & std::ios_base::trunc) ;

  if( file->fail() ) {
    std::cout << "ERROR: Cannot open file " << filename << " for writing.\n";
    return -1;
  }

  std::cout << "Starting DRS4_reader.\n";

  /*** Write file header and time calibration ***/

  // Fixme: The version number should come from DRSBoard::GetDRSType()
  file->write(reinterpret_cast<const char*>(&headers->fheader), 4);
  file->write(reinterpret_cast<const char*>(&headers->theader), 4);

  std::cout << "Writing headers." << std::endl;
  for(int iboard=0; iboard<headers->chTimes.size(); iboard++) {
    // Write board header
    file->write(headers->bheaders.at(iboard)->bn, 2);
    file->write(reinterpret_cast<const char*>(&(headers->bheaders.at(iboard)->board_serial_number)), 2);
    // Write time calibration
    for (int ichan=0; ichan<headers->chTimes.at(iboard).size(); ichan++) {
      file->write(reinterpret_cast<const char*>(headers->chTimes.at(iboard).at(ichan)), sizeof(DRS4_data::ChannelTime) );
    }
  } // End loop over boards
  std::cout << "Done writing headers." << std::endl;


  while(!f_stop) {

    event = fifo->read();
    std::cout << "Read event." << std::endl;

    if(event) {
      std::cout << "Read event #" << event->getEvtNumber() << std::endl;
      event->write(file);
      processEvent();
      delete event;
    }
    else {
      if(f_stopWhenEmpty || !writer->isRunning()) {
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
