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

#include "DAQ-config.h"
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

  config options(new DRS4_data::Observables);

  /*** Monitor frame ***/
  std::cout << "Constructing frame." << std::endl;
  MonitorFrame frame(gClient->GetRoot(), &options, drs);

  std::cout << "Start DAQ" << std::endl;
  app->Run();
 // std::cout << "Finished! Press enter to close.\n";
 // std::cin.ignore();
  return 0;
}
