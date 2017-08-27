/************************************
 * Implementation of class DRS4_fifo
 * with the associated structs
 ***********************************/

#include <queue>
#include <vector>
#include <iostream>
#include <chrono>
#include "DRS4_fifo.h"

namespace DRS4_data {


  /*
   * Implementation the struct RawEvent
   */

  RawEvent::~RawEvent() {
    // Delete all "eventWaves" objects one by one and
    // pop their pointers from the vector.
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
  { }


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


  // Read() returns the pointer to the front RawEvent object.
  // The pointer is popped from the list.
  // The caller is responsible for freeing the memory.
  RawEvent* DRS4_fifo::Read() {
    if (eventQueue.empty()) return NULL;
    RawEvent *pt = eventQueue.front();
    eventQueue.pop();
    return pt;
  }


  // Write() pushes the passed pointer. The caller must have already reserved the memory.
  int DRS4_fifo::Write(RawEvent * pt) {
    if (eventQueue.size() > maxSize) {
      std::cout << "FIFO buffer full!" << std::endl;
      return -1;
    }
    eventQueue.push(pt);
    msLastEvent = pt->header.getMsTotRun();
    return 0;
  }

} // namespace DRS4_data
