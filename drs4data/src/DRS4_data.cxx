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
    range(0), bheader(0), tcheader(-1), msTotRun(0)
  {

  }


  EHEADER::EHEADER(const EHEADER &eh) :
    event_header({'E', 'H', 'D', 'R'}),
    event_serial_number(eh.getEventNumber()),
    year(eh.getYear()), month(eh.getMonth()), day(eh.getDay()),
    hour(eh.getHour()), minute(eh.getMinute()), second(eh.getSecond()),
    millisecond(eh.getMillisecond()), range(eh.getRange()),
    msTotRun(0),
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
    msTotRun = duration_cast<milliseconds>(dtn).count();

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



  /*** struct ChannelData ***/
  int ChannelData::write(std::ofstream *file) {

    if(!file) return -1;
    if(!file->good()) return -2;

    file->write( reinterpret_cast<const char*>(&ch), sizeof(CHEADER) );
    file->write( reinterpret_cast<const char*>(&scaler), sizeof(uint32_t) );
    file->write( reinterpret_cast<const char*>(data), kNumberOfBins*sizeof(int16_t) );

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
        chData.at(iboard).at(ichan)->write(file);
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
        b->GetTimeCalibration(0, ichan*2, 0, ct->tbins);

        chTimeVec.push_back(ct);
      } // End loop over channels

      chTimes->push_back(chTimeVec);

    } // End loop over boards

    return new DRS4_data::DRSHeaders(fheader, bheaders, chTimes);

  }


  int DRSHeaders::write(std::ofstream *file) const {

    file->write(reinterpret_cast<const char*>(FHeader()), 4);
    file->write(reinterpret_cast<const char*>(&DRS4_data::THEADER), 4);

    for(int iboard=0; iboard<ChTimes()->size(); iboard++) {
      // Write board header
      file->write(BHeaders()->at(iboard)->bn, 2);
      file->write(reinterpret_cast<const char*>(&(BHeaders()->at(iboard)->board_serial_number)), 2);
      // Write time calibration
      for (int ichan=0; ichan<ChTimes()->at(iboard).size(); ichan++) {
        file->write(reinterpret_cast<const char*>(&ChTimes()->at(iboard).at(ichan)->ch), sizeof(CHEADER) );
        file->write(reinterpret_cast<const char*>(ChTimes()->at(iboard).at(ichan)->tbins), kNumberOfBins*sizeof(float) );
      }
    } // End loop over boards
    file->flush();
    std::cout << "Wrote DRS headers." << std::endl;

  }

  // Spike removal adapted from class Osci in DRS software
  void RemoveSpikes(int16_t wf[4][kNumberOfBins], short threshold, short spikeWidth)
  {
     int spikePos[kNumberOfBins];
     memset(spikePos, 0, sizeof(spikePos));

     const unsigned nChan = 4;
     int sp[nChan][100];
     int rsp[100];
     int n_sp[nChan], n_rsp;
     int  nNeighbor, nSymmetric;


     memset(sp, 0, sizeof(sp));
     memset(n_sp, 0, sizeof(n_sp));
     memset(rsp, 0, sizeof(rsp));
     n_rsp = 0;


     /* find spikes with a high-pass filter */
     for (unsigned iChan=0 ; iChan<nChan ; iChan++) {

        for (unsigned ibin=0 ; ibin<kNumberOfBins-1 ; ibin++) {

           float diff = - static_cast<float>( wf[iChan][ibin] ) / 2;
           for (unsigned spikeBin=1; spikeBin<=spikeWidth; spikeBin++) {
             diff += static_cast<float>(wf[iChan][(ibin+spikeBin) % kNumberOfBins]) / spikeWidth;
           }
           diff -= static_cast<float>( wf[iChan][(ibin+spikeWidth+1) % kNumberOfBins] ) / 2;

           float slope = static_cast<float>( wf[iChan][(ibin+spikeWidth+1) % kNumberOfBins] )
                       - static_cast<float>( wf[iChan][ibin] ) ;

           if (diff > threshold && diff > slope) {
             n_sp[iChan]++;
             sp[iChan][n_sp[iChan]] = ibin;
             spikePos[ibin]++;
           }
        } // Loop over bins
     } // Loop over chans

     /* go through all spikes and look for neighbors */
     for (unsigned iChan=0 ; iChan<nChan ; iChan++) {
        for (unsigned ispike =0 ; ispike<n_sp[iChan] ; ispike++) {

           /* check if there is a spike at the same position in other channels */
           nNeighbor=0;
           for (unsigned jChan=0 ; jChan<nChan ; jChan++) {
              if (iChan != jChan) {
                 for (unsigned lspike=0 ; lspike<n_sp[jChan] ; lspike++)
                    if ( sp[iChan][ispike] == sp[jChan][lspike] )
                    {
                       nNeighbor++;
                       break;
                    }
              }
           }


           /* if at least two matching spikes, treat this as a real spike */
           if (nNeighbor >= 2) {
              // Check if this spike is already registered as real
              unsigned jspike;
              bool alreadyKnown = false;
              for (jspike=0 ; jspike<n_rsp ; jspike++) {
                 if (rsp[jspike] == sp[iChan][ispike]) {
                   break;
                   alreadyKnown = true;
                 }
              }
              // If not registered, register
              if (n_rsp < 100 && !alreadyKnown) {
                 rsp[n_rsp++] = sp[iChan][ispike];
              }
           }
        }
     } // End search for neighbors

     if (n_rsp > 10) {
       std::cout << "WARNING: More than 10 spikes in event!\n";
     }


     /* Correct spikes */
     for (unsigned ispike=0 ; ispike<n_rsp ; ispike++) {
        for (unsigned iChan=0 ; iChan<nChan ; iChan++) {
          /* remove single spike */
          short x = wf[iChan][rsp[ispike]];
          short y = wf[iChan][(rsp[ispike]+spikeWidth+1) % kNumberOfBins];

          double slope = static_cast<double>(y-x)/(spikeWidth+1);
          for (unsigned spikeBin=1; spikeBin<=spikeWidth; spikeBin++) {
            wf[iChan][(rsp[ispike]+spikeBin) % kNumberOfBins] = static_cast<short>(x + spikeBin*slope + .5);
          }
        } // Loop over iChan
     } // Loop over ispike


     /* find spikes at cell #0 and #1023 */
     for (unsigned iChan=0 ; iChan<nChan ; iChan++) {

        float diff = 0;
        for (unsigned spikeBin=0; spikeBin<spikeWidth; spikeBin++) {
          diff += static_cast<float>(wf[iChan][spikeBin]) / spikeWidth;
        }
        diff -= static_cast<float>(wf[iChan][spikeWidth]);

        // Correct immediately. False spikes have low impact here.
        if ( fabs(diff) > threshold) {
          for (unsigned spikeBin=0; spikeBin<spikeWidth; spikeBin++) {
             wf[iChan][spikeBin] = wf[iChan][spikeWidth];
          }
        }

        diff = 0;
        for (unsigned spikeBin=kNumberOfBins-spikeWidth; spikeBin<kNumberOfBins; spikeBin++) {
          diff += static_cast<float>(wf[iChan][spikeBin]) / spikeWidth;
        }
        diff -= static_cast<float>(wf[iChan][kNumberOfBins-spikeWidth-1]);

        // Correct immediately. False spikes have low impact here.
        if (fabs(diff) > threshold) {
          for (unsigned spikeBin=kNumberOfBins-spikeWidth; spikeBin<kNumberOfBins; spikeBin++) {
             wf[iChan][spikeBin] = wf[iChan][kNumberOfBins-spikeWidth-1];
          }
        }
     }

  } // RemoveSpikes()


} // namespace DRS4_data
