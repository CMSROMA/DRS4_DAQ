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


namespace DRS4_data {

  struct Waveforms {
    Waveforms() :
      waveforms(new (std::nothrow) unsigned char [kNumberOfChipsMax * kNumberOfChannelsMax * 2 * kNumberOfBins])
    { }
/*    Waveforms::Waveforms(unsigned nChips=1, unsigned nChannels=8) :
      waveforms(new (std::nothrow) unsigned char [nChips * nChannels * 2 * kNumberOfBins])
    { }*/
    ~Waveforms() {
      delete [] waveforms;
    }
    unsigned char *waveforms;
  } ;


  struct RawEvent {
    ~RawEvent();
    std::vector<Waveforms*> eventWaves;
    EHEADER header;
  };


  class DRS4_fifo {

  public:
    DRS4_fifo();
    ~DRS4_fifo();

    // Returns the pointer to RawEvent object. The pointer is popped from the queue.
    // The caller is responsible for freeing the memory (deleting the pointer).
    // If the queue is empty, returns null pointer.
    RawEvent* read() ;
    // Push the pointer. The caller should have already reserved the memory.
    int write(RawEvent*) ;
    bool isEmpty() const { return eventWaves.empty(); }

    void Discard();

    static const unsigned maxSize = 10000;

  private:
    std::queue<RawEvent*> eventWaves;

  };

} // namespace DRS4_data

#endif /* DRS4DATA_INCLUDE_DRS4_FIFO_H_ */
