

/***************************
 * Implementation of class DRS4_fifo
 ***************************/

#include "DRS4_fifo.h"

DRS4_fifo::DRS4_fifo() {

}

// returns the pointer. The pointer is popped from the list.
// The caller is responsible of freeing the memory.
DRS4_data::Event* DRS4_fifo::read() {

  if (events.empty()) return NULL;
  DRS4_data::Event *p = events.front();
  events.pop();
  return p;
}

// Reserve memory and push the pointer
int DRS4_fifo::write(DRS4_data::Event * evt) {
  events.push(evt);
  return 0;
}

