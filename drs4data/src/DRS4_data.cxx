/*
 * DRS4_data.cxx
 *
 * Contents:
 * Implementation of classes declared in DRS4_data.h
 *
 *  Created on: Apr 7, 2017
 *      Author: S. Lukic
 */

#include "DRS4_data.h"
#include <chrono>

namespace DRS4_data {


  /*** Class EHEADER ***/
  EHEADER::EHEADER() :
    event_header({'E', 'H', 'D', 'R'}),
    event_serial_number(0),
    year(2017), month(0), day(0), hour(0), minute(0), second(0), millisecond(0),
    range(0)
  {

  }

  void EHEADER::setTimeStamp() {

    using namespace std::chrono;

 //   using years = duration<int, std::ratio<3600*24*365>>;

    system_clock::time_point tp = system_clock::now();
    system_clock::duration dtn = tp.time_since_epoch();

    time_t tt = system_clock::to_time_t(tp);
    tm local_tm = *localtime(&tt);

    year = local_tm.tm_year + 1900;
    month = local_tm.tm_mon+1;
    day = local_tm.tm_mday;
    hour = local_tm.tm_hour;
    minute = local_tm.tm_min;
    second = local_tm.tm_sec;
    seconds s = duration_cast<seconds>(dtn);
    millisecond = duration_cast<milliseconds>(dtn).count()
                - duration_cast<seconds>(dtn).count()*1000;

  }

  int EHEADER::write(std::ofstream* file) const {

    if(!file) return -1;
    if(!file->good()) return -2;

    file->write(event_header, sizeof(event_header));
    file->write( reinterpret_cast<const char*>(&event_serial_number),
                 sizeof(event_serial_number));
    file->write( reinterpret_cast<const char*>(&year), sizeof(year) );
    file->write( reinterpret_cast<const char*>(&month), sizeof(month) );
    file->write( reinterpret_cast<const char*>(&day), sizeof(day) );
    file->write( reinterpret_cast<const char*>(&hour), sizeof(hour) );
    file->write( reinterpret_cast<const char*>(&minute), sizeof(minute) );
    file->write( reinterpret_cast<const char*>(&second), sizeof(second) );
    file->write( reinterpret_cast<const char*>(&millisecond), sizeof(millisecond) );
    file->write( reinterpret_cast<const char*>(&range), sizeof(range) );

    return 0;
  }


  /*** class Event ***/

  Event::Event(const unsigned _nchans, const unsigned _nboards) :
    nchans(_nchans), nboards(_nboards),
    board(NULL), tcell(NULL),
    chData(NULL)
  {

  }


  int Event::write(std::ofstream *file) const {

    if(!file) return -1;
    if(!file->good()) return -2;

    header.write(file);

    for(unsigned iboard=0; iboard<nboards; iboard++) {

      file->write( reinterpret_cast<char*>(board+iboard), sizeof(BHEADER) );
      file->write( reinterpret_cast<const char*>(tcell+iboard), sizeof(TCHEADER) );

      for (unsigned ichan=0; ichan<nchans; ichan++) {
        file->write( reinterpret_cast<const char*>(&(chData[iboard][ichan])), sizeof(ChannelData) );
      } // loop over channels

    } // loop over boards

    return 0;
  }


}
