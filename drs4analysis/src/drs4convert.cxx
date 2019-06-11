/*
   Name: drs4analysis.cxx

   Purpose:  Analyse data saved by drs4daq
*/

#include <cstring>
#include <math.h>
#include <fstream>
#include <vector>
#include <time.h>

#include "TGraph.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TSystem.h"
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

#define MAX_PROCESSED_SPILL_PER_FILE 5

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
   unsigned int   spill_number;
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

using namespace H4DAQ;

TFile * outFile = NULL;
TTree * outTree = NULL;
H4DAQ::Event* event_ = NULL;

void createOutputFile(TString outName, int seq)
{
  if (outFile)
    {
      outFile->ls () ;
      outFile->cd () ;
      if (outTree)
	outTree->Write ("",TObject::kOverwrite) ;
      outFile->Close () ;
    }
  outFile = TFile::Open (Form("%s/h4Tree_%d.root",outName.Data(),seq), "RECREATE") ;  

  if (!outFile->IsOpen ()) 
    std::cout << "Cannot open " << outFile->GetName() << std::endl;
   
  outTree = new TTree ("H4tree", "H4 testbeam tree");

  if (event_)
    delete event_;
  event_ = new H4DAQ::Event(outTree) ;
}


int main(int argc, char **argv)
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
   TString inpath,outName,spills;
   
   int prescale=-1;
   bool allFiles=false;

   int c;
   opterr = 0;

   while ((c = getopt (argc, argv, "i:o:s:p:a")) != -1)
     switch (c)
       {
       case 'i':
	 inpath = TString(optarg);
	 break;
       case 'o':
	 outName = TString(optarg);
	 break;
       case 's':
	 spills = TString(optarg);
	 break;
       case 'p':
	 prescale = TString(optarg).Atoi();
	 break;
       case 'a':
	 allFiles=true;
	 break;
       // case '?':
       // 	 if (optopt == 'c')
       // 	   fprintf (stderr, "Option -%c requires an argument.\n", optopt);
       // 	 else if (isprint (optopt))
       // 	   fprintf (stderr, "Unknown option `-%c'.\n", optopt);
       // 	 else
       // 	   fprintf (stderr,
       // 		    "Unknown option character `\\x%x'.\n",
       // 		    optopt);
       // 	 return 1;
       // default:
       // 	 abort ();
       }

   
   string ls_command;
   string path;
   
   std::vector<TString>filenames;
   //---Get file list searching in specified path (eos or locally)
   ls_command = string("ls "+inpath+" | grep '.dat' > /tmp/drs4convert.list");
   system(ls_command.c_str());
   
   ifstream waveList(string("/tmp/drs4convert.list").c_str(), ios::in);
   TString line;
   while (!waveList.eof()) {
     line.ReadLine(waveList);
     if (line.Sizeof() <= 1) continue;
     filenames.push_back(inpath+"/"+TString(line.Strip(TString::kTrailing, ' ')));
     std::cout << "+++ Adding file " << filenames.back() << std::endl;
   }
   
   // ifstream infile(infilename.Data());

   // if (infile.fail()) {
   //   std:cout << "Cannot open list file " << infilename.Data() << std::endl;
   //   return -1;
   // }

   // TString line;
   // const TRegexp number_patt("[-+ ][0-9]*[.]?[0-9]+");
   // while (!infile.eof()) {
   //   line.ReadLine(infile);
   //   if (line.Sizeof() <= 1) continue;
   //   filenames.push_back(new TString(line.Strip(TString::kTrailing, ' ')));
   // }


   // infile.close();
   // infile.clear();

   // std::cout << "Processing files:\n";
   // for (int ifile=0; ifile<filenames.size(); ifile++) {
   //   std::cout << filenames.at(ifile).Data() << std::endl;
   // }
   // std::cout << "Spike removal " << (removeSpikes ? "active.\n" : "inactive.\n");
    
   if (!gSystem->OpenDirectory(outName.Data()))
      gSystem->mkdir(outName.Data());
   
   
   std::set<int> spillNumbers;
   Ssiz_t from = 0;
   TString tok;
   while (spills.Tokenize(tok, from, ",")) {
     spillNumbers.insert(tok.Atoi());
   }

   int startTime=-999;
   int processedSpills=0;
   /*** Prepare tree and output histos ***/
   for (unsigned ifile=0; ifile<filenames.size(); ifile++) {

     if (!allFiles && spillNumbers.size())
       {
	 TString baseName(gSystem->BaseName(filenames.at(ifile).Data()));
	 TString spill;
	 Ssiz_t from = 0;
	 baseName.Tokenize(spill, from, "_");
	 if (spillNumbers.find(spill.Atoi()) == spillNumbers.end() )
	   continue;
       }

     if (processedSpills%MAX_PROCESSED_SPILL_PER_FILE==0)
       createOutputFile(outName,processedSpills/MAX_PROCESSED_SPILL_PER_FILE +1);

     std::cout << "Opening data file \'" << filenames.at(ifile).Data() << "\'\n";

     ifstream infile;
     infile.open(filenames.at(ifile).Data(), std::ifstream::binary);
     if (infile.fail()) {
       std::cout << "Cannot open!\n";
       break;
      }

     processedSpills++;     
      // read file header
     n_boards = readDataFileHeader (infile, fh, th, bh, eh, tch, ch, &(bin_width[0]), filenames.at(0).Data());

     eh.event_serial_number = 0;
     while (eh.event_serial_number < 1000000) {
 
       // read event header
        infile.read(reinterpret_cast<char*>(&eh), sizeof(eh));
        if (infile.fail()) break;

	event_->clear();
	event_->id.runNumber = 1;
	event_->id.spillNumber = eh.spill_number;
	event_->id.evtNumber = eh.event_serial_number;

	//fill event time
	timeData td; td.board=1;
	td.time=eh.hour*3600000+eh.minute*60000+eh.second*1000+eh.millisecond;

	if (startTime==-999)
	  {
	    startTime=td.time;
	    td.time=0;
	  }
	else
	  {
	    td.time= td.time - startTime;
	  }

	event_->evtTimes.push_back(td);
				   
	struct tm * timeinfo;
	time_t ts = time(NULL);
	timeinfo = localtime(&ts);
	timeinfo->tm_year   = eh.year - 1900;
	timeinfo->tm_mon    = eh.month - 1;    //months since January - [0,11]
	timeinfo->tm_mday   = eh.day;          //day of the month - [1,31] 
	timeinfo->tm_hour   = eh.hour;         //hours since midnight - [0,23]
	timeinfo->tm_min    = eh.minute;          //minutes after the hour - [0,59]
	timeinfo->tm_sec    = eh.second;          //seconds after the minute - [0,59]
	
	ts = mktime ( timeinfo );

	timeData td1; td.board=1;
	td1.time=ts;
	event_->evtTimes.push_back(td1);
	
        if (eh.event_serial_number%100 == 0) {
          std::cout << Form("Found event #%d at %d:%02d:%02d ts:%d.%d\n", eh.event_serial_number, eh.hour, eh.minute, eh.second,ts,td.time%1000);
        }

        // loop over all boards in data file
        for (b=0 ; b<n_boards ; b++) {

           // read board header
          infile.read(reinterpret_cast<char*>(&bh), sizeof(bh));
           if (std::memcmp(bh.bn, "B#", 2) != 0) {
             std::cout << Form("Invalid board header \'%s\' in event %d in file \'%s\', aborting.\n",
                 bh.bn, eh.event_serial_number, filenames.at(ifile).Data());
             return 0;
           }

           // read trigger cell
           infile.read(reinterpret_cast<char*>(&tch), sizeof(tch));
           if (std::memcmp(tch.tc, "T#", 2) != 0) {
             std::cout << Form("Invalid trigger cell header \'%s\' in event %d file \'%s\', aborting.\n",
                 tch.tc, eh.event_serial_number, filenames.at(ifile).Data());
             return 0;
           }

	   int freq=round(1./bin_width[b][0][0]);

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
           // if (removeSpikes) DRS4_data::RemoveSpikes(voltage, 20, 2);

           // Process data in all channels
           for (unsigned ichan=0 ; ichan<4 ; ichan++) {
             int chidx = chn_index[ichan];
	     float t = 0;
	     for (int ibin=0 ; ibin<1024 ; ibin++) {
	       //get calibrated times
	       if (ibin>0)
		 t += bin_width[b][chidx][(ibin+tch.trigger_cell-1) % 1024];
	       // Voltage data is encoded in units of 0.1 mV
	       float v = static_cast<float>(voltage[ichan][ibin]) / 10;

	       digiData aDigiSample ;
	       aDigiSample.board = b;
	       aDigiSample.channel = chidx;
	       aDigiSample.group = 0;
	       aDigiSample.frequency = freq;
	       aDigiSample.startIndexCell = tch.trigger_cell;
	       aDigiSample.sampleIndex = ibin;
	       aDigiSample.sampleTime = t;
	       aDigiSample.sampleRaw = 0xFFFF; //put dummy value
	       aDigiSample.sampleValue = v;
	       event_->digiValues.push_back (aDigiSample) ;
	     }
	     
           } // Loop over channels
        } // Loop over the boards

	if ( (prescale < 0) || ( (prescale>0) && eh.event_serial_number%prescale==0))
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
