

/***************************
 * Implementation of class DRS4_fifo
 ***************************/

#include <queue>
#include <vector>
#include "DRS4_fifo.h"

namespace DRS4_data {


/*
 * Implementation of struct RawEvent
 */

RawEvent::~RawEvent() {

  for (unsigned iwf=0; iwf<eventWaves.size(); iwf++) {
    delete eventWaves.at(iwf);
  }
}


/*
 * Implementation of class DRS4_fifo
 */

DRS4_fifo::DRS4_fifo() {

}

DRS4_fifo::~DRS4_fifo() {

  while(!eventWaves.empty()) {
    delete eventWaves.front();
    eventWaves.pop();
  }
}

  // returns the pointer. The pointer is popped from the list.
  // The caller is responsible of freeing the memory.
  RawEvent* DRS4_fifo::read() {

    if (eventWaves.empty()) return NULL;
    RawEvent *pt = eventWaves.front();
    eventWaves.pop();
    return pt;
  }

  // Push the pointer. The caller should have already reserved the memory.
  int DRS4_fifo::write(RawEvent * pt) {
    eventWaves.push(pt);
    return 0;
  }

} // namespace DRS4_data
