

/***************************
 * Implementation of class DRS4_fifo
 ***************************/

#include <queue>
#include <vector>
#include <iostream>
#include <chrono>
#include "DRS4_fifo.h"

namespace DRS4_data {


  /*
   * Implementation of struct RawEvent
   */

  RawEvent::~RawEvent() {

    while (!eventWaves.empty()) {
      delete eventWaves.back();
      eventWaves.pop_back();
    }
  }


  /*
   * Implementation of class DRS4_fifo
   */

  DRS4_fifo::DRS4_fifo() :
    msLastEvent(0),
    msBeginRun(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
  {

  }

  DRS4_fifo::~DRS4_fifo() {

    Discard();
  }

  void DRS4_fifo::Discard() {

    while(!eventQueue.empty()) {
      delete eventQueue.front();
      eventQueue.pop();
    }
  }

  unsigned DRS4_fifo::timeLastEvent() const {

    return msLastEvent-msBeginRun;
  }

  // returns the pointer. The pointer is popped from the list.
  // The caller is responsible of freeing the memory.
  RawEvent* DRS4_fifo::read() {

    if (eventQueue.empty()) return NULL;
    RawEvent *pt = eventQueue.front();
    eventQueue.pop();
    return pt;
  }

  // Push the pointer. The caller should have already reserved the memory.
  int DRS4_fifo::write(RawEvent * pt) {

    if (eventQueue.size() > maxSize) {
      std::cout << "FIFO buffer full!" << std::endl;
      return -1;
    }
    eventQueue.push(pt);
    msLastEvent = pt->header.getMsTotRun();
    return 0;
  }

} // namespace DRS4_data
