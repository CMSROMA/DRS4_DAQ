/*
 * DRS4_fifo.h
 *
 *  Created on: Apr 6, 2017
 *      Author: S. Lukic
 */

#ifndef DRS4DATA_INCLUDE_DRS4_FIFO_H_
#define DRS4DATA_INCLUDE_DRS4_FIFO_H_

#include "DRS4_data.h"
#include <queue>
#include <vector>
#include <iostream>


namespace DRS4_data {


  // Struct that manages memory allocation and free-ing
  // for one data readout from a single DRS4 board
  struct Waveforms {

    Waveforms() : waveforms(NULL) {
      waveforms = new (std::nothrow) unsigned char [kNumberOfChipsMax * kNumberOfChannelsMax * 2 * kNumberOfBins];
    }

    ~Waveforms() {
      delete [] waveforms;
    }

    unsigned char *waveforms;
  } ;


  // Struct that manages a set of Waveforms objects
  // (one per DRS4 board) for one event. The objects
  // should be accessible by the sequential number of the board
  struct RawEvent {
    ~RawEvent();
    std::vector<Waveforms*> eventWaves;
    EHEADER header;
  };


  // Class that manages a std::queue of pointers to RawEvent objects
  // so that the readout from the board(s) and storing of the data in memory can be
  // done in a thread separate from the online monitoring.
  class DRS4_fifo {

  public:
    DRS4_fifo();
    ~DRS4_fifo();

    // Returns the time of the last event in ms since the beginning of the run.
    unsigned timeLastEvent() const ;

    // Returns the pointer to the front RawEvent object in the queue.
    // The pointer is popped from the queue.
    // The caller is responsible for freeing the memory.
    // If the queue is empty, returns null pointer.
    RawEvent* Read() ;
    // Write() pushes the passed pointer.
    // The validity of the pointer is not checked.
    // The caller must have already reserved the memory.
    int Write(RawEvent*) ;

    bool IsEmpty() const { return eventQueue.empty(); }
    unsigned Size() const {return eventQueue.size(); }

    void SetTimeBeginRun(unsigned tf) { msBeginRun = tf; };

    void Discard();

    static const unsigned maxSize = 1000;

  private:
    std::queue<RawEvent*> eventQueue;
    unsigned msLastEvent;
    unsigned msBeginRun;

  };

} // namespace DRS4_data

#endif /* DRS4DATA_INCLUDE_DRS4_FIFO_H_ */
