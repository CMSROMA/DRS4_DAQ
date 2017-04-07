/*****************************************************
 * drs4daq.cxx
 *
 * File with the main() routine for DRS4_DAQ
 *
 *  Created on: Apr 7, 2017
 *      Author: S. Lukic
 ****************************************************/

#include <thread>
#include <string>

#include "DRS4_fifo.h"
#include "DRS4_writer.h"
#include "DRS4_reader.h"


int main(int argc, char* argv[]) {

  int iarg = 1;
  std::string datfilename("test.dat");
  if(argc > iarg) datfilename = argv[iarg]; iarg++;
  unsigned nEvtMax = -1;
  if(argc > iarg) nEvtMax = atoi(argv[iarg]); iarg++;

  DRS4_fifo *fifo = new DRS4_fifo;

  /* do initial scan */
  DRS *drs = new DRS();

  /* show any found board(s) */
  for (int i=0 ; i<drs->GetNumberOfBoards() ; i++) {
    DRSBoard *b = drs->GetBoard(i);
     printf("Found DRS4 evaluation board, serial #%d, firmware revision %d\n",
        b->GetBoardSerialNumber(), b->GetFirmwareVersion());
  }

  /* exit if no board found */
  int nBoards = drs->GetNumberOfBoards();
  if (nBoards == 0) {
     printf("No DRS4 evaluation board found\n");
     return 0;
  }

  DRS4_writer writer(drs, fifo, nEvtMax);

  DRS4_reader reader(fifo, datfilename.c_str());

  return 0;
}
