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

#include <fstream>
#include "stdint.h"

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

  typedef struct {
     char           event_header[4];
     unsigned int   event_serial_number;
     unsigned short year;
     unsigned short month;
     unsigned short day;
     unsigned short hour;
     unsigned short minute;
     unsigned short second;
     unsigned short millisecond;
     unsigned short range;
  } EHEADER;

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
    uint32_t tbins[1024];
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
    Event(const unsigned nchans=4, const unsigned nboards=1);
    ~Event();

    // Pointer to data to be stored
//    void *Data() const { return dynamic_cast<void*>(&header); }
    // Size in bytes
  //  unsigned size() const ;
    unsigned nChans() const { return _nchans; }
    unsigned nBoards() const { return _nboards; }

    int write(std::ofstream &file);

  private:

    const unsigned _nchans;
    const unsigned _nboards;

    EHEADER header;
    BHEADER *board;
    TCHEADER *tcell;
    ChannelData **chData;

  };

} // namespace DRS4_data


#endif /* DRS4DATA_INCLUDE_DRS4_DATA_H_ */
