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
#include "math.h"



namespace DRS4_data {


  /*** Class EHEADER ***/
  EHEADER::EHEADER() :
    event_header({'E', 'H', 'D', 'R'}),
    event_serial_number(0),
    year(2017), month(0), day(0), hour(0), minute(0), second(0), millisecond(0),
    range(0), bheader(0), tcheader(-1)
  {

  }


  EHEADER::EHEADER(const EHEADER &eh) :
    event_header({'E', 'H', 'D', 'R'}),
    event_serial_number(eh.getEventNumber()),
    year(eh.getYear()), month(eh.getMonth()), day(eh.getDay()),
    hour(eh.getHour()), minute(eh.getMinute()), second(eh.getSecond()),
    millisecond(eh.getMillisecond()), range(eh.getRange()),
    bheader(eh.getBoardNumber()), tcheader(eh.getTriggerCell())
  {

  }

  void EHEADER::setTimeStamp() {

    using namespace std::chrono;

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

  Event::Event(const unsigned iEvt, DRS *drs)
  {
    header.setEvtNumber(iEvt);
    header.setTimeStamp();
    header.setRange(int(floor( (drs->GetBoard(0)->GetCalibratedInputRange())*1000 + 0.5)));

    for( unsigned iboard=0; iboard<drs->GetNumberOfBoards(); iboard++) {
      AddBoard(drs->GetBoard(iboard));
    }

  }


  Event::Event(const unsigned iEvt, const EHEADER _header, DRS *drs) :
    header(_header)
  {
    for( unsigned iboard=0; iboard<drs->GetNumberOfBoards(); iboard++) {
      AddBoard(drs->GetBoard(iboard));
    }
  }


  Event::~Event() {

    while(!bheaders.empty()) {
      delete bheaders.back();
      bheaders.pop_back();
    }

    while(!tcells.empty()) {
      delete tcells.back();
      tcells.pop_back();
    }

    while(!chData.empty()) {
      while(!chData.back().empty()) {
        delete chData.back().back();
        chData.back().pop_back();
      }
      chData.pop_back();
    }

  }


  void Event::AddBoard(DRSBoard * newboard) {

    BHEADER *bhdr = new BHEADER(newboard->GetBoardSerialNumber());
    bheaders.push_back(bhdr);

    TCHEADER *tchdr = new TCHEADER(newboard->GetTriggerCell(0)); // Fixme: Chip index?
    tcells.push_back(tchdr);

    std::vector<ChannelData*> channels;

    for (unsigned ichan=0; ichan < 4; ichan++) {
      ChannelData* cd = new ChannelData(ichan+1);
      channels.push_back(cd);
    }

    chData.push_back(channels);

  }


  ChannelData *const Event::getChData(int iboard, int ichan) const {

    if ( iboard < 0 || iboard >= chData.size() ) {
      std::cout << "Event::getChData() WARNING: Requesting invalid board number "
          << iboard << "." << std::endl;
      return NULL;
    }

    if (ichan < 0 || ichan >= chData.at(iboard).size()) {
      std::cout << "Event::getChData() WARNING: Requesting invalid channel number "
          << ichan << "." << std::endl;
      return NULL;
    }

    return chData.at(iboard).at(ichan);
  }


  int Event::write(std::ofstream *file) const {

    if(!file) return -1;
    if(!file->good()) return -2;

//    std::cout << "Storing event to file." << std::endl;

    header.write(file);
//    std::cout << "Stored event header." << std::endl;

    for(unsigned iboard=0; iboard<bheaders.size(); iboard++) {

 //     std::cout << "Storing board header #" << iboard << "." << std::endl;
      file->write( reinterpret_cast<char*>(bheaders.at(iboard)), sizeof(BHEADER) );
      file->write( reinterpret_cast<const char*>(tcells.at(iboard)), sizeof(TCHEADER) );

 //     std::cout << "Storing data." << std::endl;
      for (unsigned ichan=0; ichan<chData.at(iboard).size(); ichan++) {
        file->write( reinterpret_cast<const char*>(chData.at(iboard).at(ichan)), kNumberOfBins*sizeof(uint16_t) );
      } // loop over channels

    } // loop over boards

    return 0;
  }



  /*** Class BoardHeaders ***/

  DRSHeaders::DRSHeaders(DRSHeaders& _headers) :
    fheader(_headers.fheader), bheaders(_headers.bheaders), chTimes(_headers.chTimes)
  {
  }

  DRSHeaders::DRSHeaders(const FHEADER _fheader, std::vector<BHEADER*> _bheaders,
      ChannelTimes* _chTimes) :
    fheader(_fheader), bheaders(_bheaders), chTimes(*_chTimes)
  {
  }

  DRSHeaders::~DRSHeaders() {

    while (!bheaders.empty()) {
      delete bheaders.back();
      bheaders.pop_back();
    }

    while (!chTimes.empty()) {
      while (!chTimes.back().empty()) {
        delete chTimes.back().back();
        chTimes.back().pop_back();
      }
      chTimes.pop_back();
    }
  }

  DRSHeaders * DRSHeaders::MakeDRSHeaders(DRS* drs) {
    // Objects for the initialization of the DRS headers
    DRS4_data::FHEADER fheader(drs->GetBoard(0)->GetDRSType());
    std::vector<DRS4_data::BHEADER*> bheaders;
    DRS4_data::ChannelTimes *chTimes = new DRS4_data::ChannelTimes;

    /*** Get board serial numbers and time bins ***/
    for (int iboard=0; iboard<drs->GetNumberOfBoards(); iboard++) {

      std::cout << "DRS4_reader::DRS4_reader() - Reading board #" << iboard << std::endl;

      DRSBoard *b = drs->GetBoard(iboard);

      // Board serial numbers
      DRS4_data::BHEADER *bhdr = new DRS4_data::BHEADER(b->GetBoardSerialNumber());
      bheaders.push_back(bhdr);

      std::vector<DRS4_data::ChannelTime*> chTimeVec;

      for (int ichan=0 ; ichan<4 ; ichan++) {

        DRS4_data::ChannelTime *ct = new DRS4_data::ChannelTime(ichan+1);
        // Get time bins
        b->GetTime(iboard, ichan*2, b->GetTriggerCell(iboard), ct->tbins);

        chTimeVec.push_back(ct);
      } // End loop over channels

      chTimes->push_back(chTimeVec);

    } // End loop over boards

    return new DRS4_data::DRSHeaders(fheader, bheaders, chTimes);

  }

}
