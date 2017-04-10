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



  /*******************************
   * Time data
   *******************************/

  struct ChannelTime {

    CHEADER ch;
    float tbins[1024];
  };




  /*******************************
   * Amplitude data
   *******************************/

  struct ChannelData {

    CHEADER ch;
    uint32_t scaler;
    uint16_t data[1024];
  };


  class Event {

  public:
    Event(const unsigned iEvt, const unsigned range);
    ~Event();

    void AddBoard(DRSBoard *);

    // Pointer to data to be stored
    ChannelData *const getChData(int iboard, int ichan) const;
//    void *Data() const { return dynamic_cast<void*>(&header); }
    // Size in bytes
  //  unsigned size() const ;
   // unsigned nChans() const { return nchans; }
   // unsigned nBoards() const { return nboards; }

    int write(std::ofstream *) const ;

  private:

    Event();

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
