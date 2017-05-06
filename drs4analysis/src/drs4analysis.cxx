/*
   Name: drs4analysis
   Based on read_binary.cpp by Stefan Ritt <stefan.ritt@psi.ch>

   Purpose:      analyse data saved by drs4daq
 
   The present version only plots the oscillograms from all 4 channels
   in a pdf file.
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

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
      printf("Usage: drs4analysis <filename> <remove spikes (1|0)> [<common mode file name>]\n");
      return 0;
   }
   bool removeSpikes = false;
   if (argc > 2) removeSpikes = atoi(argv[2]);
   if (argc > 3) cmfilename = argv[3];
   else cmfilename = "BaseLine_CommonMode.root";
   
   // open the binary waveform file
   FILE *f = fopen(infilename.Data(), "rb");
   if (f == NULL) {
      printf("Cannot find file \'%s\'\n", infilename.Data());
      return 0;
   }

   // read file header
   fread(&fh, sizeof(fh), 1, f);
   if (fh.tag[0] != 'D' || fh.tag[1] != 'R' || fh.tag[2] != 'S') {
      printf("Found invalid file header in file \'%s\', aborting.\n", infilename.Data());
      return 0;
   }
   
   if (fh.version != '2' && fh.version != '4') {
      printf("Found invalid file version \'%c\' in file \'%s\', should be \'2\' or \'4\', aborting.\n",
          fh.version, infilename.Data());
      return 0;
   }

   // read time header
   fread(&th, sizeof(th), 1, f);
   if (memcmp(th.time_header, "TIME", 4) != 0) {
      printf("Invalid time header in file \'%s\', aborting.\n", infilename.Data());
      return 0;
   }

   for (b = 0 ; ; b++) {
      // read board header
      fread(&bh, sizeof(bh), 1, f);
      if (memcmp(bh.bn, "B#", 2) != 0) {
         // probably event header found
         fseek(f, -4, SEEK_CUR);
         break;
      }
      
      printf("Found data for board #%d\n", bh.board_serial_number);
      
      // read time bin widths
      memset(bin_width[b], sizeof(bin_width[0]), 0);
      for (int ichan=0 ; ichan<5 ; ichan++) {
         fread(&ch, sizeof(ch), 1, f);
         if (ch.c[0] != 'C') {
            // event header found
            fseek(f, -4, SEEK_CUR);
            break;
         }
         int i = ch.cn[2] - '0' - 1;
         printf("Found timing calibration for channel #%d\n", i+1);
         fread(&bin_width[b][i][0], sizeof(float), 1024, f);
         // fix for 2048 bin mode: double channel
         if (bin_width[b][i][1023] > 10 || bin_width[b][i][1023] < 0.01) {
            for (unsigned jbin=0 ; jbin<512 ; jbin++)
               bin_width[b][i][jbin+512] = bin_width[b][i][jbin];
         }
      }
   }
   n_boards = b;
   
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
   TString pdfname(infilename);
   if (pdfname.EndsWith(".dat")) pdfname.Resize(pdfname.Sizeof() - 5);
   pdfname += ".pdf";
   bool firstpage = true, lastpage = false;

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

   TString rfname(infilename);
   if (rfname.EndsWith(".dat")) rfname.Resize(pdfname.Sizeof() - 5);
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
   eh.event_serial_number = 0;
   while (eh.event_serial_number < 100000) {

      // read event header
      int lRead = (int)fread(&eh, sizeof(eh), 1, f);
      if (lRead < 1)
         break;

      if (eh.event_serial_number%100 == 0) {
        printf("Found event #%d at %d:%02d:%03d\n", eh.event_serial_number, eh.minute, eh.second, eh.millisecond);
      }
      
      // loop over all boards in data file
      for (b=0 ; b<n_boards ; b++) {
         
         // read board header
         fread(&bh, sizeof(bh), 1, f);
         if (memcmp(bh.bn, "B#", 2) != 0) {
            printf("Invalid board header in file \'%s\', aborting.\n", infilename.Data());
            return 0;
         }
         
         // read trigger cell
         fread(&tch, sizeof(tch), 1, f);
         if (memcmp(tch.tc, "T#", 2) != 0) {
            printf("Invalid trigger cell header in file \'%s\', aborting.\n", infilename.Data());
            return 0;
         }

         if (n_boards > 1)
            printf("Found data for board #%d\n", bh.board_serial_number);
         
         for (unsigned ichan=0 ; ichan<4 ; ichan++) {
           chn_index[ichan] = 0;
         }

         // read channel data
         for (unsigned ichan=0 ; ichan<4 ; ichan++) {
            
            // read channel header
            fread(&ch, sizeof(ch), 1, f);
            if (ch.c[0] != 'C') {
               // event header found
               fseek(f, -4, SEEK_CUR);
               break;
            }
            chn_index[ichan] = ch.cn[2] - '0' - 1;
            fread(&scaler, sizeof(int), 1, f);
            fread(voltage[ichan], sizeof(short), 1024, f);
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
         if (eh.event_serial_number < 20)
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
   
   std::cout << "Read " << eh.event_serial_number << " events. Event tree has " << events.GetEntries() << " entries.\n";

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

   return 1;
}

