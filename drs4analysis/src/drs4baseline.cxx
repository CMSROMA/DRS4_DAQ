/*
   Name: drs4baseline.cxx

   Purpose: Calculate average waveforms registered in the four channels of drs4daq
   Store average waveforms as histograms in a root file.

*/

#include <cstring>
#include <math.h>
#include <fstream>
#include <vector>
#include <iostream>

#include "TH1F.h"
#include "TFile.h"
#include "TStyle.h"
#include "TString.h"

#include "WaveProcessor.h"
#include "DRS4_data.h"


typedef struct {
   char           tag[3];
   char           version;
} FHEADER;

typedef struct {
   char           time_header[4];
} THEADER;

typedef struct {
   char           bn[2];
   unsigned short board_serial_number;
} BHEADER;

typedef struct {
   char           event_header[4];
   unsigned int   event_serial_number;
   unsigned short year;
   unsigned short month;
   unsigned short day;
   unsigned short hour;
   unsigned short minute;
   unsigned short second;
   unsigned short millisecond;
   unsigned short range;
} EHEADER;

typedef struct {
   char           tc[2];
   unsigned short trigger_cell;
} TCHEADER;

typedef struct {
   char           c[1];
   char           cn[3];
} CHEADER;


int readDataFileHeader (std::ifstream &, FHEADER  &, THEADER  &, BHEADER  &,
                        EHEADER  &, TCHEADER &, CHEADER  &, float bin_width[16][4][1024],
                        const char *filename);

/*-----------------------------------------------------------------------------*/

int main(int argc, const char * argv[])
{
   FHEADER  fh;
   THEADER  th;
   BHEADER  bh;
   EHEADER  eh;
   TCHEADER tch;
   CHEADER  ch;
   
   unsigned int scaler;
   int16_t voltage[4][1024];
   float waveform[16][4][1024];
   float timebins[16][4][1024];
   float bin_width[16][4][1024];
   int b, n_boards;
   int chn_index[4];
   double t1, t2, dt;
   TString infilename, cmfilename;

   const float baselineWid = 22.;
   
   if (argc > 1)  infilename = argv[1];
   else {
      printf("Usage: drs4analysis <list file name> [<remove spikes (1|0)>] [<common mode file name>]\n");
      return 0;
   }
   TString genname(infilename);
   if (genname.EndsWith(".in")) { genname.Resize(genname.Sizeof()-4); }
   bool removeSpikes = false;
   if (argc > 2) removeSpikes = atoi(argv[2]);
   if (argc > 3) cmfilename = argv[3];
   else cmfilename = "BaseLine_CommonMode.root";
   

   std::vector<TString*>filenames;
   std::ifstream infile(infilename.Data());

   if (infile.fail()) {
     std::cout << "Cannot open list file " << infilename.Data() << std::endl;
     return -1;
   }

   TString line;

   while (!infile.eof()) {

     line.ReadLine(infile);
     if (line.Sizeof() <= 1) continue;

     if (line.Contains("spike", TString::kIgnoreCase)) {
       removeSpikes = true;
       continue;
     }
     filenames.push_back(new TString(line.Strip(TString::kTrailing, ' ')));
   }

   infile.close();
   infile.clear();

   std::cout << "Processing files:\n";
   for (int ifile=0; ifile<filenames.size(); ifile++) {
     std::cout << filenames.at(ifile)->Data() << std::endl;
   }
   std::cout << "Spike removal " << (removeSpikes ? "active.\n" : "inactive.\n");



   /*** Prepare histos ***/

   TString rfname(genname);
   rfname += ".root";
   TFile file(rfname, "RECREATE");

   TH1F *havg[4];
   for (int ich=0; ich<4; ich++) {
     havg[ich] = new TH1F(Form("havg_S%d", ich+1), "Average waveform; t (ns); A (mV)", 500, 0., 200.);
   }

   unsigned nCh = 4;
   DRS4_data::Observables obs[nCh];

   // loop over all events in the data file
   int iEvt[4] = {0, 0, 0, 0};


   for (unsigned ifile=0; ifile<filenames.size(); ifile++) {

      std::cout << "Opening data file \'" << filenames.at(ifile)->Data() << "\'\n";

      infile.open(filenames.at(ifile)->Data(), std::ifstream::binary);
      if (infile.fail()) {
        std::cout << "Cannot open!\n";
        break;
      }

      // read file header
      n_boards = readDataFileHeader (infile, fh, th, bh, eh, tch, ch, &(bin_width[0]), filenames.at(0)->Data());

      eh.event_serial_number = 0;
      while (eh.event_serial_number < 100000) {

        // read event header
        infile.read(reinterpret_cast<char*>(&eh), sizeof(eh));
        if (infile.fail()) break;

        if (eh.event_serial_number%100 == 0) {
          std::cout << Form("Found event #%d at %d:%02d:%02d\n", eh.event_serial_number, eh.hour, eh.minute, eh.second);
        }

        // loop over all boards in data file
        for (b=0 ; b<n_boards ; b++) {

           // read board header
          infile.read(reinterpret_cast<char*>(&bh), sizeof(bh));
           if (std::memcmp(bh.bn, "B#", 2) != 0) {
             std::cout << Form("Invalid board header \'%s\' in event %d in file \'%s\', aborting.\n",
                 bh.bn, eh.event_serial_number, filenames.at(ifile)->Data());
             return 0;
           }

           // read trigger cell
           infile.read(reinterpret_cast<char*>(&tch), sizeof(tch));
           if (std::memcmp(tch.tc, "T#", 2) != 0) {
             std::cout << Form("Invalid trigger cell header \'%s\' in event %d file \'%s\', aborting.\n",
                 tch.tc, eh.event_serial_number, filenames.at(ifile)->Data());
             return 0;
           }

           for (unsigned ichan=0 ; ichan<4 ; ichan++) {
             chn_index[ichan] = 0;
           }

           // read channel data
           for (unsigned ichan=0 ; ichan<4 ; ichan++) {

              // read channel header
             infile.read(reinterpret_cast<char*>(&ch), sizeof(ch));
              if (ch.c[0] != 'C') {
                 // event header found
                infile.seekg(-4, infile.cur);
                 break;
              }
              chn_index[ichan] = ch.cn[2] - '0' - 1;
              infile.read(reinterpret_cast<char*>(&scaler), sizeof(int));
              infile.read(reinterpret_cast<char*>(voltage[ichan]), sizeof(short)*1024);
           }

           // Remove spikes
           // Wrote own function because trigger cell is not always well written in file
           if (removeSpikes) DRS4_data::RemoveSpikes(voltage, 20, 2);

           int nsaturated[4] = {0,0,0,0};

           // Process data in all channels
           for (unsigned ichan=0 ; ichan<4 ; ichan++) {

             int chidx = chn_index[ichan];
              for (int ibin=0 ; ibin<1024 ; ibin++) {

                 float t = 0;
                 timebins[b][chidx][ibin]=0;
                 for (int jbin =0; jbin<ibin ; jbin++)
                    t += bin_width[b][chidx][(jbin+tch.trigger_cell) % 1024];

                 timebins[b][chidx][ibin] = t;

                 // Voltage data is encoded in units of 0.1 mV
                 float v = static_cast<float>(voltage[ichan][ibin]) / 10;
                 // Subtract time-dependent baseline
                 waveform[b][chidx][ibin] = v;

                 if (abs(voltage[ichan][ibin]) >= 4999) {
                   nsaturated[ichan]++;
                 }

              }

              DRS4_data::Observables *tmpObs = WaveProcessor::ProcessOnline(timebins[b][chidx],
                  waveform[b][chidx], 1024, 4., baselineWid);
              obs[ichan] = *tmpObs;
              delete tmpObs; tmpObs = NULL;
           } // Loop over channels

           // Ignore events with saturated signals in S1 or S2
           bool saturated = false;
           for (int ichan=0; ichan<2; ichan++) {
             if (nsaturated[ichan] > 3) {
               std::cout << "Saturated signal in channel #" << ichan+1 << " in event #"
                   <<  eh.event_serial_number << std::endl;
               saturated = true;
             }
           }
           // Ignore events with *heavily* saturated signals in S3 and S4 (possible card trips)
           for (int ichan=2; ichan<4; ichan++) {
             if (nsaturated[ichan] > 100) {
               std::cout << "Saturated signal in channel #" << ichan+1 << " in event #"
                   <<  eh.event_serial_number << std::endl;
               saturated = true;
             }
           }
           if (saturated) continue;


           /*** Average pulse calculation ***/

           // Ignore events that might bias the average pulse calculation
           bool reject = false;
           for (unsigned ichan=0; ichan<4; ichan++) {
             if ( obs[ichan].Value(DRS4_data::baseLineRMS) > 1. )
               reject = true;
           }
           if (reject) continue;

           // Reference time for the alignment
 //          float tref = (obs[2].Value(DRS4_data::arrivalTime) + obs[3].Value(DRS4_data::arrivalTime)) / 2;

           for (unsigned ichan=0 ; ichan<4 ; ichan++) {
             for (unsigned ibin=0 ; ibin<1024 ; ibin++) {
               float t = timebins[b][ichan][ibin];// - tref + baselineWid;
               float v = waveform[b][ichan][ibin] + obs[ichan].Value(DRS4_data::baseLine);
               havg[ichan]->Fill(t, v);
             }
             iEvt[ichan]++;
           }



        } // Loop over the boards
     } // Loop over events

      infile.close();
      infile.clear();
   } // Loop over files
   

   for (unsigned ichan=0; ichan<4; ichan++) {
     havg[ichan]->Scale(1./iEvt[ichan]);
   }
   file.Write();
   file.Close();

   // cleanup filenames vector
   for (int i=0; i<filenames.size(); i++) {
     delete filenames.at(i);
   }
   filenames.clear();


   return 1;
}




int readDataFileHeader (std::ifstream &infile, FHEADER  &fh, THEADER  &th, BHEADER  &bh,
                        EHEADER  &eh, TCHEADER &tch, CHEADER  &ch, float bin_width[16][4][1024],
                        const char *filename) {

  // read file header
  infile.read(reinterpret_cast<char*>(&fh), sizeof(fh));
  if (std::memcmp(fh.tag, "DRS", 3) != 0) {
    std::cout << "Found invalid file header \'" << fh.tag << "\' in file " << filename << ", aborting.\n";
    return -2;
  }
  if (fh.version != '2' && fh.version != '4') {
    std::cout << "Found invalid DRS version " << fh.version << " in file "
        << filename << ", should be \'2\' or \'4\', aborting.\n";
    return -3;
  }

  // read time header
  infile.read(reinterpret_cast<char*>(&th), sizeof(th));
  if (std::memcmp(th.time_header, "TIME", 4) != 0) {
     std::cout << "Invalid time header in file " << filename << ", aborting.\n";
     return -4;
  }

  int b;

  for (b = 0 ; ; b++) {
     // read board header
    infile.read(reinterpret_cast<char*>(&bh), sizeof(bh));
    if (std::memcmp(bh.bn, "B#", 2) != 0) {
       // probably event header found
       infile.seekg(-4, infile.cur);
       break;
    }

    std::cout << "Found time data for board " << bh.board_serial_number << std::endl;

     // read time bin widths
     memset(bin_width[b], sizeof(bin_width[0]), 0);

     for (int ichan=0 ; ichan<5 ; ichan++) {
        infile.read(reinterpret_cast<char*>(&ch), sizeof(ch));
        if (ch.c[0] != 'C') {
           // event header found
           infile.seekg(-4, infile.cur);
           break;
        }
        int i = ch.cn[2] - '0' - 1;
        std::cout << "Found timing calibration for channel " << i+1 << std::endl;
        infile.read(reinterpret_cast<char*>(&bin_width[b][i][0]), sizeof(float)*1024);
     } // Loop over channels
  } // Loop over boards

  return b;
}
