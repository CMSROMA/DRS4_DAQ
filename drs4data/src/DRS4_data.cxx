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
#include <iostream>

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

  Event::Event(const unsigned iEvt, const unsigned range)
  {
    header.setEvtNumber(iEvt);
    header.setTimeStamp();
    header.setRange(range);
  }


  void Event::AddBoard(DRSBoard * newboard) {

    BHEADER *bhdr = new BHEADER;
    bhdr->bn[0] = 'B';
    bhdr->bn[1] = '#';
    bhdr->board_serial_number = newboard->GetBoardSerialNumber();
    bheaders.push_back(bhdr);

    TCHEADER *tchdr = new TCHEADER;
    tchdr->tc[0] = 'T';
    tchdr->tc[1] = '#';
    tchdr->trigger_cell = newboard->GetTriggerCell(0); // FIXME

  }


  ChannelData *const Event::getChData(int iboard, int ichan) const {

    if ( iboard < 0 || iboard > chData.size() ) {
      std::cout << "Event::getChData() WARNING: Requesting invalid board number "
          << iboard << "." << std::endl;
      return NULL;
    }

    if (ichan < 0 || ichan > chData.at(iboard).size()) {
      std::cout << "Event::getChData() WARNING: Requesting invalid channel number "
          << ichan << "." << std::endl;
      return NULL;
    }

    return chData.at(iboard).at(ichan);
  }


  int Event::write(std::ofstream *file) const {

    if(!file) return -1;
    if(!file->good()) return -2;

    header.write(file);

    for(unsigned iboard=0; iboard<bheaders.size(); iboard++) {

      file->write( reinterpret_cast<char*>(bheaders.at(iboard)), sizeof(BHEADER) );
      file->write( reinterpret_cast<const char*>(tcells.at(iboard)), sizeof(TCHEADER) );

      for (unsigned ichan=0; ichan<chData.at(iboard).size(); ichan++) {
        file->write( reinterpret_cast<const char*>(&(chData[iboard][ichan])), sizeof(ChannelData) );
      } // loop over channels

    } // loop over boards

    return 0;
  }


}
