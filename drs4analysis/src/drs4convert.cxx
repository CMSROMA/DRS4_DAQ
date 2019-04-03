/*
   Name: drs4analysis.cxx

   Purpose:  Analyse data saved by drs4daq
*/

#include <cstring>
#include <math.h>
#include <fstream>
#include <vector>

#include "TGraph.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TString.h"
#include "Fit/Fitter.h"
#include "Fit/FitResult.h"
#include "Math/MinimizerOptions.h"
#include "TRegexp.h"
#include "TLegend.h"
#include "TLegendEntry.h"

#include "WaveProcessor.h"
#include "DRS4_data.h"
#include "Event.h"
#include "BaseLineModel.h"

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
   TString infilename;

   
   if (argc > 1)  infilename = argv[1];
   else {
      printf("Usage: drs4analysis <list file name> [<remove spikes (1|0)>] [<common mode file name>]\n");
      return 0;
   }
   bool removeSpikes = false;
   if (argc > 2) removeSpikes = atoi(argv[2]);
   
   std::vector<TString*>filenames;
   ifstream infile(infilename.Data());

   if (infile.fail()) {
     std:cout << "Cannot open list file " << infilename.Data() << std::endl;
     return -1;
   }

   TString line;
   const TRegexp number_patt("[-+ ][0-9]*[.]?[0-9]+");

   while (!infile.eof()) {
     line.ReadLine(infile);
     if (line.Sizeof() <= 1) continue;
     filenames.push_back(new TString(line.Strip(TString::kTrailing, ' ')));
   }

   infile.close();
   infile.clear();

   std::cout << "Processing files:\n";
   for (int ifile=0; ifile<filenames.size(); ifile++) {
     std::cout << filenames.at(ifile)->Data() << std::endl;
   }
   std::cout << "Spike removal " << (removeSpikes ? "active.\n" : "inactive.\n");

   TString outName("test.root");
   TFile * outFile = TFile::Open (outName, "RECREATE") ;  
   if (!outFile->IsOpen ()) 
     std::cout << "Cannot open " << outName << std::endl;
   
   TTree * outTree = new TTree ("H4tree", "H4 testbeam tree") ;
   Event* event_ = new Event(outTree) ;

   /*** Prepare tree and output histos ***/
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

	event_->clear();
	event_->id.runNumber = 1;
	event_->id.spillNumber = 1;
	event_->id.evtNumber = eh.event_serial_number;

	//fill event time
	timeData td; td.board=1;
	td.time=eh.hour*3600000+eh.minute*60000+eh.second*1000+eh.millisecond;
	event_->evtTimes.push_back(td);
				   
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
                 // if (chidx < 2) {
                 //   int baselineBin = hcm[chidx]->FindBin(t);
                 //   float blbWidth = hcm[chidx]->GetBinWidth(baselineBin);
                 //   float thisbWid = bin_width[b][chidx][(ibin+tch.trigger_cell) % 1024];
                 //   v -= hcm[chidx]->GetBinContent(baselineBin)*thisbWid/blbWidth;
                 // }/**/
                 waveform[b][chidx][ibin] = v;
		 if (ichan==1 && eh.event_serial_number<10)
		     std::cout << "ibin " << ibin << " t " << timebins[b][chidx][ibin] << " v " << waveform[b][chidx][ibin] << std::endl; 

                  //This is a sample! 
                  digiData aDigiSample ;
                  aDigiSample.board = b;
                  aDigiSample.channel = chidx;
                  aDigiSample.group = 0;
                  aDigiSample.frequency = 2 ;
                  aDigiSample.startIndexCell = tch.trigger_cell;
                  aDigiSample.sampleIndex = ibin;
                  aDigiSample.sampleTime = timebins[b][chidx][ibin];
                  aDigiSample.sampleRaw = 0xFFFF; //put dummy value
                  aDigiSample.sampleValue = waveform[b][chidx][ibin];
                  event_->digiValues.push_back (aDigiSample) ;
              }

           } // Loop over channels

	   //           events.Fill();

        } // Loop over the boards
	event_->Fill();
      } // Loop over events

      infile.close();
      infile.clear();
      //      std::cout << "Read " << eh.event_serial_number << " events from this file. Event tree has a total of " << events.GetEntries() << " entries.\n";
   } // Loop over files
   

   outFile->ls () ;
   outFile->cd () ;
   outTree->Write ("",TObject::kOverwrite) ;
   outFile->Close () ;

   delete event_;
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
