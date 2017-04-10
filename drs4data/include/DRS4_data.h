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

  static const unsigned nChansDRS4 = 1024;

  typedef struct {
     char           tag[3];
     char           version;
  } FHEADER;


  typedef struct {
     char           time_header[4];
  } THEADER;


  typedef struct {
     char           bn[2];
     unsigned short board_serial_number;
  } BHEADER;


  class EHEADER{
  public:
    EHEADER();

    void setEvtNumber(unsigned evtnum) { event_serial_number = evtnum; }
    void setTimeStamp();
    void setRange(unsigned short _range) { range = _range; }

    unsigned getSerialNumber() const { return event_serial_number; }

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
  };


  typedef struct {
     char           tc[2];
     unsigned short trigger_cell;
  } TCHEADER;


  typedef struct {
     char           c[1];
     char           cn[3];
  } CHEADER;

  void setCHeader(CHEADER &ch, int ichan);


  /*******************************
   * Time data
   *******************************/

  struct ChannelTime {

    CHEADER ch;
    float tbins[nChansDRS4];
  };




  /*******************************
   * Amplitude data
   *******************************/

  struct ChannelData {

    CHEADER ch;
    uint32_t scaler;
    uint16_t data[nChansDRS4];
  };


  class Event {

  public:
    Event(const unsigned iEvt, DRS*);
    ~Event();

    // Pointer to data to be stored
    ChannelData *const getChData(int iboard, int ichan) const;
    unsigned getNBoards() const { return chData.size(); }
    unsigned getNChans(unsigned iboard) const {
      if ( iboard < 0 || iboard >= getNBoards() ) return 0;
      return chData.at(iboard).size();
    }

    unsigned getEvtNumber() const { return header.getSerialNumber(); }

//    void *Data() const { return dynamic_cast<void*>(&header); }
    // Size in bytes
  //  unsigned size() const ;
   // unsigned nChans() const { return nchans; }
   // unsigned nBoards() const { return nboards; }

    int write(std::ofstream *) const ;

  private:

    Event();

    void AddBoard(DRSBoard *);

//    const unsigned nchans;
 //   const unsigned nboards;

    EHEADER header;
    std::vector<BHEADER*> bheaders;
    std::vector<TCHEADER*> tcells;
    std::vector<std::vector<ChannelData*>> chData;

  };


  typedef std::vector<std::vector<ChannelTime*>> ChannelTimes;

} // namespace DRS4_data


#endif /* DRS4DATA_INCLUDE_DRS4_DATA_H_ */
