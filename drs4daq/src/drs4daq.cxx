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
#include "MonitorFrame.h"

#include "TApplication.h"
#include "TGClient.h"


int main(int argc, char* argv[]) {

  /*** ROOT TApplication ***/
  TApplication *app = new TApplication("App", &argc, argv);


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
  if (app->Argc() > iarg) datfilename = app->Argv()[iarg]; iarg++;
  unsigned nEvtMax = -1;
  if (app->Argc() > iarg) nEvtMax = atoi(app->Argv()[iarg]); iarg++;


  /*
   * We allow more than one board with synchronized triggers.
   * For simplicity, we assume that 4 channels are acquired from each board.
   */

  /* use first board with highest serial number as the master board */
  drs->SortBoards();
  DRSBoard *mb = drs->GetBoard(0);


  /* common configuration for all boards */
  for (int iboard=0 ; iboard<drs->GetNumberOfBoards() ; iboard++) {

    std::cout << "Configuring board #" << iboard << std::endl;

    DRSBoard *b = drs->GetBoard(iboard);

    /* initialize board */
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
    std::cout << "Setting frequency to 5 GHz." << std::endl;
    b->SetFrequency(5, true);

    /* set input range to -0.5V ... +0.5V */
    std::cout << "Setting input range to (-0.5V -> +0.5V)." << std::endl;
    b->SetInputRange(0);

    /* enable hardware trigger
     * (1, 0) = "fast trigger", "analog trigger"
     * Board types 8, 9 always need (1, 0)
     * other board types take (1, 0) for external (LEMO/FP/TRBUS)
     * and (0, 1) for internal trigger (analog threshold).
     */
    b->EnableTrigger(1, 0);


    if (iboard == 0) {
      /* master board: enable hardware trigger on CH1 at -50 mV negative edge */
      std::cout << "Configuring master board." << std::endl;
      b->SetTranspMode(1);
      b->SetTriggerSource(1<<0);        // set CH1 as source
      b->SetTriggerLevel(-0.05);        // -50 mV
      b->SetTriggerPolarity(true);      // negative edge
      b->SetTriggerDelayNs(150);        // Trigger delay shifts waveform left
    } else {
      /* slave boards: enable hardware trigger on Trigger IN */
      std::cout << "Configuring slave board." << std::endl;
      b->SetTriggerSource(1<<4);        // set Trigger IN as source
      b->SetTriggerPolarity(false);     // positive edge
    }
  } // End loop for common configuration


  // Start DAQ
  mb->EnableTcal(5);
  mb->SelectClockSource(0);
//  writer.setAutoTrigger();

  /*** Main frame ***/

  const config* options;

  std::cout << "Constructing frame." << std::endl;
  MonitorFrame frame(gClient->GetRoot(), options, drs);

  app->Run();
  std::cout << "Finished! Press enter to close.\n";
  std::cin.ignore();
  return 0;

  return 0;
}
