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
    event_serial_number(0), spill_number(0),
    year(2017), month(0), day(0), hour(0), minute(0), second(0), millisecond(0),
    range(0), bheader(0), tcheader(-1), msTotRun(0)
  {

  }


  EHEADER::EHEADER(const EHEADER &eh) :
    event_header({'E', 'H', 'D', 'R'}),
    event_serial_number(eh.getEventNumber()),
    spill_number(eh.getSpillNumber()),
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
    file->write( reinterpret_cast<const char*>(&spill_number),
                 sizeof(spill_number));
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

  Event::Event(const unsigned iEvt, const unsigned spill, DRS *drs)
  {
    header.setEvtNumber(iEvt);
    header.setSpillNumber(spill);
    header.setTimeStamp();
    header.setRange(int(floor( (drs->GetBoard(0)->GetCalibratedInputRange())*1000 + 0.5)));

    for( unsigned iboard=0; iboard<drs->GetNumberOfBoards(); iboard++) {
      AddBoard(drs->GetBoard(iboard));
    }

  }


  Event::Event(const unsigned iEvt, const unsigned spill, const EHEADER _header, DRS *drs) :
    header(_header)
  {
    header.setSpillNumber(spill);
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

  int fillH4Event(DRSHeaders* headers, Event* event, H4DAQ::Event *h4event) 
  {
    if(!headers ||
       !event  ||
       !h4event ) return -1;

    // std::cout << "Filling event  " << header.getEventNumber() << std::endl;
    //fill h4daq format
    h4event->id.runNumber = 1;
    h4event->id.spillNumber = event->getEventHeader().getSpillNumber();
    h4event->id.evtNumber = event->getEventHeader().getEventNumber();

    //fill event time
    H4DAQ::timeData td; td.board=1;
    td.time=event->getEventHeader().getHour()*3600000+event->getEventHeader().getMinute()*60000+event->getEventHeader().getSecond()*1000+event->getEventHeader().getMillisecond();
    h4event->evtTimes.push_back(td);

    for(unsigned iboard=0; iboard<event->getNBoards(); iboard++) 
      {
    	int tcell=event->getTriggerCells()[iboard]->trigger_cell;

    	for(unsigned ich=0; ich<event->getNChans(iboard); ich++) 
    	  {
    	    const ChannelTimes* chTimes=headers->ChTimes();
    	    const ChannelData* chData=event->getChData(iboard,ich);
    	    float t = 0;
    	    for (int isample=0 ; isample<kNumberOfBins ; isample++) {

    	      //get calibrated time (in ns)
    	      if (isample>0)
    		t += chTimes->at(iboard).at(ich)->tbins[(tcell+isample-1) % kNumberOfBins];

    	      H4DAQ::digiData aDigiSample ;
    	      aDigiSample.board = iboard;
    	      aDigiSample.channel = ich;
    	      aDigiSample.group = 0;
    	      aDigiSample.frequency = round(1./chTimes->at(iboard).at(0)->tbins[0]) ;
    	      aDigiSample.startIndexCell = tcell;
    	      aDigiSample.sampleIndex = isample;
    	      aDigiSample.sampleTime = t;
    	      aDigiSample.sampleRaw = 0xFFFF; //put dummy value
    	      aDigiSample.sampleValue = float(chData->data[isample])/10.;
    	      h4event->digiValues.push_back (aDigiSample) ;
    	    }
    	  }
      }
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

     unsigned nSpikesTot=0;
     unsigned nSpikesRemoved=0;
     const unsigned nChan = 4;

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
             spikePos[ibin]++;
             nSpikesTot++;
           }
        } // Loop over bins
     } // Loop over chans


     // Remove spikes if at least two found at the same position in different channels.
     for (unsigned ibin=0; ibin<kNumberOfBins; ibin++) {

       if (spikePos[ibin] < 2) continue;
       for (unsigned iChan=0 ; iChan<nChan ; iChan++) {
         /* remove single spike */
         short x = wf[iChan][ibin];
         short y = wf[iChan][(ibin+spikeWidth+1) % kNumberOfBins];

         double slope = static_cast<double>(y-x)/(spikeWidth+1);
         for (unsigned spikeBin=1; spikeBin<=spikeWidth; spikeBin++) {
           wf[iChan][(ibin+spikeBin) % kNumberOfBins] = static_cast<short>(x + spikeBin*slope + .5);
         }
       } // Loop over iChan
       nSpikesRemoved++;
     }

 //    std::cout << "Of " << nSpikesTot << " candidates removed " << nSpikesRemoved << "*4 spikes.\n";

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
