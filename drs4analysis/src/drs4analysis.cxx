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
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TString.h"
#include "Fit/Fitter.h"
#include "Fit/FitResult.h"
#include "Math/MinimizerOptions.h"

#include "WaveProcessor.h"
#include "DRS4_data.h"
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
   TString infilename, cmfilename;

   int ndt;
   double threshold, sumdt, sumdt2;
   
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
   ifstream infile(infilename.Data());

   if (infile.fail()) {
     std:cout << "Cannot open list file " << infilename.Data() << std::endl;
     return -1;
   }

   while (!infile.eof()) {
     char newname[256];
     infile >> newname;
     if (infile.fail()) break;
     filenames.push_back(new TString(newname));
   }

   infile.close();

   // initialize statistics
   ndt = 0;
   sumdt = sumdt2 = 0;
   
   /*** Prepare oscillogram plots ***/
   TCanvas c("can", "cancan", 800, 600);
   gStyle->SetPaperSize(18, 12);
   gStyle->SetLabelSize(0.06, "XY");
   gStyle->SetTitleSize(0.06, "XY");
   gStyle->SetPadLeftMargin(0.12);
   gStyle->SetPadBottomMargin(0.12);
   c.Divide(2, 2, .01, .01);
   c.GetPad(1)->SetGrid(1, 0);
   c.GetPad(2)->SetGrid(1, 0);
   c.GetPad(3)->SetGrid(1, 0);
   c.GetPad(4)->SetGrid(1, 0);
   TH1F frame("frame", "frame; t (ns); A (mV)", 10, 0., 200);
   frame.SetMinimum(-500);
   frame.SetMaximum(50);
   frame.SetLineColor(kGray);
   frame.SetStats(false);
   TString pdfname(genname);
   pdfname += ".pdf";
   bool firstpage = true;

   /*** Prepare average waveforms ***/

   TFile cmHistos(cmfilename.Data());
   if (!cmHistos.IsOpen()) {
     std::cout << "Error: Cannot open file " << cmfilename.Data() << ".\n";
     return -1;
   }
   TH1F *hcm[2] = { NULL, NULL};
   cmHistos.GetObject("havg_S1", hcm[0]);
   hcm[0]->SetName("commonModeCH1");
   cmHistos.GetObject("havg_S2", hcm[1]);
   hcm[1]->SetName("commonModeCH2");
   if (!hcm[0] || !hcm[1]) {
     std::cout << "Error: Cannot read common mode histograms \'havg_S1\' and \'havg_S2\' from file "
         << cmfilename.Data() << ".\n";
     return -2;
   }


   /*** Prepare tree and output histos ***/

   TString rfname(genname);
   rfname += ".root";
   TFile file(rfname, "RECREATE");

   TH1F *havg[4];
   for (int ich=0; ich<4; ich++) {
     havg[ich] = new TH1F(Form("havg_S%d", ich+1), "Average waveform; t (ns); A (mV)", 1000, 0., 200.);
   }

   unsigned nCh = 4;
   DRS4_data::Observables obs[nCh];
   TTree events("events", "events");

   for (unsigned iCh=0; iCh<nCh; iCh++) {

     for (unsigned iObs=0; iObs<DRS4_data::nObservables; iObs++) {
       const char *varname = Form("%s_S%d", obs[iCh].Name(DRS4_data::kObservables(iObs)), iCh+1);
       events.Branch(varname, &obs[iCh].Value(DRS4_data::kObservables(iObs)), varname);
     }
   }

   // loop over all events in the data file
   int iEvt[4] = {0, 0, 0, 0};


   for (unsigned ifile=0; ifile<filenames.size(); ifile++) {

      std::cout << "Opening data file " << filenames.at(ifile)->Data() << std::endl;

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
          std::cout << Form("Found event #%d at %d:%02d:%03d\n", eh.event_serial_number, eh.minute, eh.second, eh.millisecond);
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
                 waveform[b][chidx][ibin] = v;

              }

              float blw = 30.;
              if (chidx < 2) blw = 38;
              DRS4_data::Observables *tmpObs = WaveProcessor::ProcessOnline(timebins[b][chidx], waveform[b][chidx], 1024, 2., blw);
              obs[ichan] = *tmpObs;
              delete tmpObs; tmpObs = NULL;
           } // Loop over channels


           // Fill observables into a tree
           events.Fill();

           // Plot interesting waveforms
           if (eh.event_serial_number < 10)
         /*  if (   obs[0].Value(DRS4_data::baseLineRMS) > 1
               || obs[1].Value(DRS4_data::baseLineRMS) > 1
               || obs[2].Value(DRS4_data::baseLineRMS) > 1
               || obs[3].Value(DRS4_data::baseLineRMS) > 1 )
           if (   obs[0].Value(DRS4_data::arrivalTime) < 42.
               || obs[0].Value(DRS4_data::arrivalTime) > 52.
               || obs[1].Value(DRS4_data::arrivalTime) < 41.
               || obs[1].Value(DRS4_data::arrivalTime) > 51. )*/
           {

             for (unsigned ichan=0; ichan<4; ichan++) {
               TGraph *gr = new TGraph(1024, timebins[b][ichan], waveform[b][ichan]);

               if (gr->IsZombie()) {
                 printf("Zombie.\n");
                 exit(0);
               }
               c.cd(ichan+1);
               frame.SetTitle(Form("Ch. %d, baseline RMS %.1f, evt. %d", ichan+1, obs[ichan].Value(DRS4_data::baseLineRMS), iEvt));
               frame.DrawCopy();
               gr->SetLineColor(kRed);
               gr->SetLineWidth(1);
               gr->Draw("l");
             }

             if (firstpage) {
               c.Print(TString(pdfname + "(").Data());
               firstpage = false;
             }
             else {
               c.Print(pdfname.Data());
             }
             for (int ipad=1; ipad<=4; ipad++) {
               c.GetPad(ipad)->Clear();
             }
           } // if  printing pdf

           /*** Average pulse calculation ***/

           // Reject events that might bias the average pulse calculation
           bool reject = false;
           for (unsigned ichan=0; ichan<4; ichan++) {
             if (   obs[ichan].Value(DRS4_data::baseLineRMS) > 1.
                 || obs[ichan].Value(DRS4_data::maxVal) > 499.9 )
               reject = true;
           }
           if (reject) continue;

           // Reference time for the alignment
           float tref = (obs[2].Value(DRS4_data::arrivalTime) + obs[3].Value(DRS4_data::arrivalTime)) / 2;

           for (unsigned ichan=0 ; ichan<4 ; ichan++) {
              // Selection of amplitudes of at least 10 MIP in S1, S2
              if (ichan==0 && obs[ichan].Value(DRS4_data::maxVal) > 18) continue;
              if (ichan==1 && obs[ichan].Value(DRS4_data::maxVal) > 16) continue;
              for (unsigned ibin=0 ; ibin<1024 ; ibin++) {
                float t = timebins[b][ichan][ibin] - tref + 30.;
                float v = waveform[b][ichan][ibin] + obs[ichan].Value(DRS4_data::baseLine);
                havg[ichan]->Fill(t, v);
              }
              iEvt[ichan]++;
           }



        } // Loop over the boards
     } // Loop over events

      infile.close();
      infile.clear();
      std::cout << "Read " << eh.event_serial_number << " events from this file. Event tree has a total of " << events.GetEntries() << " entries.\n";
   } // Loop over files
   

   c.Print(TString(pdfname + ")").Data());
   for (unsigned ichan=0; ichan<4; ichan++) {
     havg[ichan]->Scale(1./iEvt[ichan]);
     if (ichan<2) {
  /*     double pars[1] = {1.};
       BaseLineModel bl(pars, havg[ichan], hcm[ichan]);
       ROOT::Fit::Fitter fitter;
       fitter.SetFCN(bl.nPars, bl, pars, bl.DataSize(), true);
       cout << "Fit function set.\n";
       bool bresult;
       bresult = fitter.FitFCN();
       if(bresult) cout << "\nFit returns true.\n";
       else cout << "\nFit returns false.\n";
       ROOT::Fit::FitResult res = fitter.Result();
       double factor = res.Parameter(0);
       cout << "Fit result chi2 = " << res.Chi2() << "\n";
       cout << "Fitted factor = " << factor << "\n";
       havg[ichan]->Add(hcm[ichan], -factor);*/
       havg[ichan]->Add(hcm[ichan], -1.);

     }/**/
   }
   file.Write();
   file.Close();

   cmHistos.Close();

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
