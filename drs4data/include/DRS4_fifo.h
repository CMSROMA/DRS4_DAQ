/*
 * DRS4_fifo.h
 *
 *  Created on: Apr 6, 2017
 *      Author: S. Lukic
 */

#ifndef DRS4DATA_INCLUDE_DRS4_FIFO_H_
#define DRS4DATA_INCLUDE_DRS4_FIFO_H_

#include <queue>

#include "DRS4_data.h"


class DRS4_fifo {

public:
  DRS4_fifo();

  // returns the pointer. The pointer is popped from the queue.
  // The caller is responsible for freeing the memory.
  // If the queue is empty, returns null pointer.
  DRS4_data::Event* read() ;
  // Reserve memory and push the pointer
  int write(DRS4_data::Event *) ;

private:
  std::queue<DRS4_data::Event*> events;
};


#endif /* DRS4DATA_INCLUDE_DRS4_FIFO_H_ */
