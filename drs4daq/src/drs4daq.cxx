/*****************************************************
 * drs4daq.cxx
 *
 * File with the main() routine for DRS4_DAQ
 *
 *  Created on: Apr 7, 2017
 *      Author: S. Lukic
 ****************************************************/

#include <string>
#include <iostream>

#include "DRS4_fifo.h"
#include "DRS4_writer.h"
#include "DRS4_reader.h"



int main(int argc, char* argv[]) {

  int iarg = 1;
  std::string datfilename("test.dat");
  if(argc > iarg) datfilename = argv[iarg]; iarg++;
  unsigned nEvtMax = -1;
  if(argc > iarg) nEvtMax = atoi(argv[iarg]); iarg++;

  std::vector<DRS4_data::BHEADER*> bheaders; // Board serial numbers
  DRS4_data::ChannelTimes *chTimes = new DRS4_data::ChannelTimes;
  DRS4_fifo *fifo = new DRS4_fifo;

  /* do initial scan */
  DRS *drs = new DRS();

  /* show any found board(s) */
  for (int i=0 ; i<drs->GetNumberOfBoards() ; i++) {
    DRSBoard *b = drs->GetBoard(i);
     printf("Found DRS4 evaluation board, serial #%d, firmware revision %d\n",
        b->GetBoardSerialNumber(), b->GetFirmwareVersion());
  }

  /* Examine command-line input for requested number(s) of channels */
  std::vector<int> nChansVec;
  for (int iboard=0; iboard<drs->GetNumberOfBoards(); iboard++) {
    int nChans = 4; // Fixme: Is 4 the good default? Or are two channels used for one input?
    if(argc > iarg) nChans = atoi(argv[iarg]); iarg++;
    if(nChans>4) {
      std::cout << "WARNING: Requested number of channels for board #"
          << iboard << " is " << nChans << ". Correcting to 4.\n";
      nChans = 4;
    }
  }

  /* exit if no board found */
  int nBoards = drs->GetNumberOfBoards();
  if (nBoards == 0) {
     printf("No DRS4 evaluation board found\n");
     return 0;
  }


  DRS4_writer writer(drs, fifo, chTimes, bheaders);

  DRS4_reader reader(fifo, chTimes);

  writer.start(1000);
  while (!writer.isRunning()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); };

  int readerState = reader.run(datfilename.c_str(), bheaders);
  if ( readerState < 0 ) writer.stop();


  return 0;
}
