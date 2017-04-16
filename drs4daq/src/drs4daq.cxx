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
#include "DRS.h"



int main(int argc, char* argv[]) {

  /* do initial scan */
  DRS *drs;
  drs = new DRS();


  /* exit if no board found */
  int nBoards = drs->GetNumberOfBoards();
  if (nBoards == 0) {
     std::cout << "No DRS4 evaluation board found\n";
     return 0;
  }

  /* show any found board(s) */
  for (int i=0 ; i<drs->GetNumberOfBoards() ; i++) {
    DRSBoard *b = drs->GetBoard(i);
    std::cout << "Found DRS4 evaluation board, serial #" << b->GetBoardSerialNumber()
        << ", firmware revision " << b->GetFirmwareVersion() << std::endl;
  }
  std::cout << "End of the list of boards." << std::endl;


  /*** Command line input ***/
  int iarg = 1;
  std::string datfilename("test.dat");
  if(argc > iarg) datfilename = argv[iarg]; iarg++;
  unsigned nEvtMax = -1;
  if(argc > iarg) nEvtMax = atoi(argv[iarg]); iarg++;

  DRS4_data::DRSHeaders *headers = NULL;
  DRS4_data::DRS4_fifo *fifo = new DRS4_data::DRS4_fifo;

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

  /* use first board with highest serial number as the master board */
  DRSBoard *mb = drs->GetBoard(0);

  /* common configuration for all boards */
  for (int iboard=0 ; iboard<drs->GetNumberOfBoards() ; iboard++) {

    std::cout << "Configuring board #" << iboard << std::endl;

    DRSBoard *b = drs->GetBoard(iboard);

    /* initialize board */
    std::cout << "Initializing." << std::endl;
    b->Init();

    /* select external reference clock for slave modules */
    /* NOTE: this only works if the clock chain is connected */
    if (iboard > 0) {
      if (b->GetFirmwareVersion() >= 21260) { // this only works with recent firmware versions
         if (b->GetScaler(5) > 300000)        // check if external clock is connected
            b->SetRefclk(true);               // switch to external reference clock
      }
    }

    /* set sampling frequency */
    std::cout << "Setting frequency" << std::endl;
    b->SetFrequency(5, true);

    /* set input range to -0.5V ... +0.5V */
    std::cout << "Setting input range." << std::endl;
    b->SetInputRange(0);

    /* enable hardware trigger */
    /* First: External LEMO/FP/TRBUS trigger
     * Second: analog threshold (internal) trigger
     */
    std::cout << "Setting trigger mode." << std::endl;
    // (1, 0) = "fast trigger", "analog trigger"
    // Board types 8, 9 always need (1, 0)
    // other board types take (1, 0) for external and (0, 1) for internal trigger.
    b->EnableTrigger(1, 0);

   // b->

    if (iboard == 0) {
      /* master board: enable hardware trigger on CH1 at 50 mV positive edge */
      std::cout << "Configuring master board." << std::endl;
      b->SetTranspMode(1);
      b->SetTriggerSource(1<<0);        // set CH1 as source
      b->SetTriggerLevel(0.05);        // -50 mV
      b->SetTriggerPolarity(false);      // negative edge
      b->SetTriggerDelayNs(150);          // Trigger delay shifts waveform left
    } else {
      /* slave boards: enable hardware trigger on Trigger IN */
      std::cout << "Configuring slave board." << std::endl;
      b->SetTriggerSource(1<<4);        // set Trigger IN as source
      b->SetTriggerPolarity(false);     // positive edge
    }
  } // End loop for common configuration


  /*** Writer and reader ***/

  std::cout << "Constructing writer." << std::endl;
  DRS4_writer writer(drs, fifo);
  // Fixme: Why not design the writer as disposable (lifetime = one run)?
  // Pro: Safer for the internal thread

  std::cout << "Constructing reader." << std::endl;
  DRS4_reader reader(fifo, drs);

  // Start DAQ
  mb->EnableTcal(5);
  mb->SelectClockSource(0);
//  writer.setAutoTrigger();
  writer.start(nEvtMax);
  while (!writer.isRunning()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); };

  int readerState = reader.run(datfilename.c_str(), &writer);
  if ( readerState < 0 ) writer.stop();

  writer.join();

  reader.stopWhenEmpty();

  return 0;
}
