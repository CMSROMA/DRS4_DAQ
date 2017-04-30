/*
 * DRS4_data.h
 *
 * Structs helping to read DRS4 data format
 *
 *  Created on: Apr 6, 2017
 *      Author: S. Lukic
 */

#ifndef DRS4DATA_INCLUDE_DRS4_DATA_H_
#define DRS4DATA_INCLUDE_DRS4_DATA_H_

#include <vector>
#include <fstream>
#include "stdint.h"

#include "DRS.h"

namespace DRS4_data {

  struct HEADER {
    HEADER(const char* init) { strncpy(header, init, 4); }
    char header[4];
  };

  static const HEADER THEADER("TIME");

  struct FHEADER {
    FHEADER(const int v=4) {
      strncpy(tag, "DRS", 3);
      char btype[10];
      sprintf(btype, "%d", v);
      version = btype[0];
    }
    char           tag[3];
    char           version;
  };


  struct BHEADER {
    BHEADER (const unsigned short bsn) :
      board_serial_number(bsn)
    {
      strncpy(bn, "B#", 2);
    }
    char           bn[2];
    const unsigned short board_serial_number;
  };


  struct TCHEADER {
    TCHEADER(const unsigned short tcell) :
      trigger_cell(tcell)
    {
      strncpy(tc, "T#", 2);
    }
    char           tc[2];
    unsigned short trigger_cell;
  } ;


  class EHEADER{
  public:
    EHEADER();
    EHEADER(const EHEADER &);

    void setEvtNumber(unsigned evtnum) { event_serial_number = evtnum; }
    void setTimeStamp();
    void setRange(unsigned short _range) { range = _range; }

    unsigned short getEventNumber()  const { return event_serial_number; }
    unsigned short getYear()          const { return year; }
    unsigned short getMonth()         const { return month; }
    unsigned short getDay()           const { return day; }
    unsigned short getHour()          const { return hour; }
    unsigned short getMinute()        const { return minute; }
    unsigned short getSecond()        const { return second; }
    unsigned short getMillisecond()   const { return millisecond; }
    unsigned short getRange()         const { return range; }
    unsigned short getBoardNumber()   const { return bheader.board_serial_number; }
    unsigned short getTriggerCell()   const { return tcheader.trigger_cell; }

    void setTriggerCell(unsigned short tc) { tcheader.trigger_cell = tc; }

    int write(std::ofstream*) const;

  private:
     const char     event_header[4];
     unsigned int   event_serial_number;
     unsigned short year;
     unsigned short month;
     unsigned short day;
     unsigned short hour;
     unsigned short minute;
     unsigned short second;
     unsigned short millisecond;
     unsigned short range;
     BHEADER bheader;
     TCHEADER tcheader;
  };


  struct CHEADER {
    CHEADER(const unsigned short cnum) {
      c[0] = 'C';
      char chnum[4];
      snprintf(chnum, 4, "%03d", cnum);
      strncpy(cn, chnum, 3);
    }
     char           c[1];
     char           cn[3];
  } ;


  /*******************************
   * Time data
   *******************************/

  struct ChannelTime {

    ChannelTime(const unsigned short chnum) :
          ch(chnum), tbins(new float[kNumberOfBins]) {}
    ~ChannelTime() {
      delete [] tbins;
    }

    const CHEADER ch;
    float *tbins;
  };




  /*******************************
   * Amplitude data
   *******************************/

  struct ChannelData {

    ChannelData(const unsigned short chnum) :
      ch(chnum), scaler(0), data(new uint16_t[kNumberOfBins]) {}
    ~ChannelData() {
      delete [] data;
    }

    int write(std::ofstream *);

    static size_t byteSize() { return sizeof(CHEADER) + sizeof(uint32_t) + kNumberOfBins*sizeof(uint16_t); }

    const CHEADER ch;
    uint32_t scaler;
    uint16_t *data;
  };


  class Event {

  public:
    Event(const unsigned iEvt, DRS*);
    Event(const unsigned iEvt, const EHEADER, DRS*);
    ~Event();

    // Pointer to data to be stored
    ChannelData *const getChData(int iboard, int ichan) const;
    unsigned getNBoards() const { return chData.size(); }
    unsigned getNChans(unsigned iboard) const {
      if ( iboard < 0 || iboard >= getNBoards() ) return 0;
      return chData.at(iboard).size();
    }

    unsigned getEvtNumber() const { return header.getEventNumber(); }

    int write(std::ofstream *) const ;

  private:

    Event();

    void AddBoard(DRSBoard *);

    EHEADER header;
    std::vector<BHEADER*> bheaders;
    std::vector<TCHEADER*> tcells;
    std::vector<std::vector<ChannelData*>> chData;

  };


  typedef std::vector<std::vector<ChannelTime*>> ChannelTimes;

  class DRSHeaders {
  // Class containing all data about the connected boards
  // that are immutable during one run.
  public:
    DRSHeaders(DRSHeaders&);
    ~DRSHeaders();

    static DRSHeaders* MakeDRSHeaders(DRS*);

    const FHEADER* FHeader() const { return &fheader; }
    const std::vector<BHEADER*>* BHeaders() const { return &bheaders; }
    const ChannelTimes* ChTimes() const { return &chTimes; }

    int write(std::ofstream *) const ;

  private:
    DRSHeaders(const FHEADER, std::vector<BHEADER*>, ChannelTimes*);

    FHEADER fheader;
    std::vector<BHEADER*> bheaders;
    ChannelTimes chTimes;
  };

  void RemoveSpikes(short wf[4][kNumberOfBins], short threshold, short spikeWidth);

} // namespace DRS4_data


#endif /* DRS4DATA_INCLUDE_DRS4_DATA_H_ */
