#include "WaveProcessor.h"
#include "TFitResult.h"
#include "TVirtualFFT.h"
#include "DRS.h"

using namespace DRS4_data;

WaveProcessor::WaveProcessor() :
    triggerHeight(0), delay(0), eventID(0), baseLineAVG(BASELINE), baseLineCNT(0),
    alocated(false), aligned(false), No_of_Ch(0),
    TempShapeCh1(NULL),
    TempShapeCh2(NULL),
    TempShapeCh3(NULL),
    TempShapeCh4(NULL),
    RawTempShape(NULL)
{}

WaveProcessor::~WaveProcessor(){
/*	
	free_matrix(TimeBinWidth,1,No_of_Ch,0,1023);
	free_matrix(BinVoltage,1,No_of_Ch,0,1023);
	free_matrix(TimeBinVoltage,1,No_of_Ch,0,1023);
*/
/*
	delete TimeShapeCh1;
	if (No_of_Ch>1) delete TimeShapeCh2; 
	if (No_of_Ch>2) delete TimeShapeCh3;
	if (TimeShapeCh4) delete TimeShapeCh4; // should work if TimeShapeCh4 is defined i.e. is not a null pointer any more
*/
}




////////////////////////// set and get of arrays of DRS4 /////////////////////////////////////////////////////////////////////

void WaveProcessor::set_time_calibration(int ch, int bin, float value){
	aligned = false; // any change of time induce that the alignment is no more valid
    TimeBinWidth[ch][bin]=value;
}
float WaveProcessor::get_time_calibration(int ch, int bin ) const {return TimeBinWidth[ch][bin];}

void WaveProcessor::set_bin_time_n_voltage(int ch, int bin, USHORT voltage, USHORT range, USHORT trigger_Cell){
	float aux_f=0; 
	aligned=false; // any change of time induce that the alignment is no more valid
	
	BinVoltage[ch][bin]=(0.5 - ((float)voltage)/65536. + ((float)range)/1000.); // this way the amplitude is inverted (positive in our case)
	for (int k=0; k<bin ; k++)	aux_f += TimeBinWidth[ch][((k+trigger_Cell) % 1024)];
	TimeBinVoltage[ch][bin]=aux_f;
}

//void Data::set_voltage_bin_total(int ch, int bin, float value) {BinVoltage[ch][bin]=value;}

float WaveProcessor::get_voltage_bin_total(int ch, int bin) const { return BinVoltage[ch][bin];}

/*
void Data::set_time_of_bin_total(int ch, int bin, float value){
	aligned=false; // any change of time induce that the alignment is no more valid
    TimeBinVoltage[ch][bin]=value;
}
*/
float WaveProcessor::get_time_of_bin_total(int ch, int bin) const { return TimeBinVoltage[ch][bin]; }


/////////////////////////////////// allignCells0 //////////////////////////////////////////////////////////////////////////

void WaveProcessor::allignCells0(USHORT trigger_cell){ // align cell #0 of all channels
	int trigCell, t1, t2, dt, chn, i;
	
	trigCell = (int) trigger_cell;
	if(aligned) { 					
		cout<<"Already aligned !!! \n Exiting..."<<endl;
		exit(1);
		}
	
	aligned=true;

	t1 = TimeBinVoltage[1][(1024-trigCell) % 1024]; // ch1 is a referent chanel

	for (chn=2 ; chn<=No_of_Ch ; chn++) {
		t2 = TimeBinVoltage[chn][(1024-trigCell) % 1024];
		dt = t1 - t2;
		for (i=0 ; i<1024 ; i++) TimeBinVoltage[chn][i] += dt;
	}
}

//////////////////////// CreateHistograms() ///////////////////////////////////////////////////////////////////////////////////////

void WaveProcessor::CreateTempHistograms(){
	//Float_t time_aux[1024];
	Int_t pomi;
	if (DEBUG2) cout<<"CreateHistograms"<<endl;
	if((!aligned)&&(No_of_Ch>1)) { 					
		cout<<" Chanels not aligned !!! \n Exiting..."<<endl;
		exit(1);
		}
	for (int j=1; j<=No_of_Ch; j++){
		for (int i=0; i<1024; i++) {
			//time_aux[i] = TimeBinVoltage [j][i];
			if (DEBUG3) cout<<"Allocation time_aux["<<i<<"]="<<TimeBinVoltage [j][i]<<endl;
		}
		/*
		switch (j) {
			case 1: TimeShapeCh1 = new TH1F("TimeShapeCh1", "Time Shape of channel 1", 1023, time_aux); break;// 1024 bins (1023 in definition), width in ns
			case 2: TimeShapeCh2 = new TH1F("TimeShapeCh2", "Time Shape of channel 2", 1023, time_aux); break;
			case 3: TimeShapeCh3 = new TH1F("TimeShapeCh3", "Time Shape of channel 3", 1023, time_aux); break;
			case 4: TimeShapeCh4 = new TH1F("TimeShapeCh4", "Time Shape of channel 4", 1023, time_aux); break;
		}
		*/
		
		// temporary histograms for time shape, must be deleted after each event because they are rebinned each time
		switch (j) {
			case 1: TempShapeCh1 = new TH1F("TempShapeCh1", "Time Shape of channel 1", 1023, TimeBinVoltage [j]); break;// 1024 bins (1023 in definition), width in ns
			case 2: TempShapeCh2 = new TH1F("TempShapeCh2", "Time Shape of channel 2", 1023, TimeBinVoltage [j]); break;
			case 3: TempShapeCh3 = new TH1F("TempShapeCh3", "Time Shape of channel 3", 1023, TimeBinVoltage [j]); break;
			case 4: TempShapeCh4 = new TH1F("TempShapeCh4", "Time Shape of channel 4", 1023, TimeBinVoltage [j]); break;
		}
		
		if (DEBUG3) cout<<"AllHist, bin 512 at: "<<	TempShapeCh1->GetXaxis()->GetBinCenter(512)<<endl;
		
	}
	
	if (DEBUG3) cout<<"FillHistograms, No of Channels: "<<No_of_Ch<<endl;
	
	for (int j=1; j<=No_of_Ch; j++){
	//if(DEBUG) cout <<"j in FillHistograms: "<<j<<endl;
	for (int i=0; i<1024; i++){
		//if(DEBUG) cout <<"i in FillHistograms: "<<i<<endl;
		if (DEBUG3) cout<<"BinVoltage["<<j<<"]["<<i<<"]="<<BinVoltage[j][i]<<endl;
		if (DEBUG3) cout<<"TimeBinVoltage["<<j<<"]["<<i<<"]="<<TimeBinVoltage[j][i]<<endl;
		
		/*
		switch (j) {
			case 1: TimeShapeCh1->Fill(TimeBinVoltage[j][i], BinVoltage[j][i]); break;
			case 2: TimeShapeCh2->Fill(TimeBinVoltage[j][i], BinVoltage[j][i]); break;
			case 3: TimeShapeCh3->Fill(TimeBinVoltage[j][i], BinVoltage[j][i]); break;
			case 4: TimeShapeCh4->Fill(TimeBinVoltage[j][i], BinVoltage[j][i]); break;
		}
		*/
		switch (j) {
			case 1: TempShapeCh1->Fill(TimeBinVoltage[j][i], BinVoltage[j][i]); break;
			case 2: TempShapeCh2->Fill(TimeBinVoltage[j][i], BinVoltage[j][i]); break;
			case 3: TempShapeCh3->Fill(TimeBinVoltage[j][i], BinVoltage[j][i]); break;
			case 4: TempShapeCh4->Fill(TimeBinVoltage[j][i], BinVoltage[j][i]); break;
		}
		
		if (DEBUG3) pomi = TempShapeCh1->FindBin(TimeBinVoltage[j][i]);
		if (DEBUG3) cout<<"Filling the bin No: "<< pomi <<endl;
		if (DEBUG3) cout<<"Left bin edge:"<<TempShapeCh1->GetXaxis()->GetBinLowEdge(pomi)<<", Right Edge: "<<(TempShapeCh1->GetXaxis()->GetBinLowEdge(pomi)+TempShapeCh1->GetXaxis()->GetBinWidth(pomi))<<endl;
		if (DEBUG3) cout<<"TempShapeCh1(TimeBinVoltage)="<<TempShapeCh1->GetBinContent(TempShapeCh1->FindBin(TimeBinVoltage[j][i]))<<flush<<endl<<endl;
		
		}// end loop i	
	}// end loop j
	
	
	
}

///////////////////////////////////// DeleteHistograms /////////////////////////////////////////////////////////////////

void WaveProcessor::DeleteTempHistograms(){
	TempShapeCh1->Delete();
	if (TempShapeCh2) TempShapeCh2->Delete(); 
	if (No_of_Ch>2) delete TempShapeCh3;
	if (No_of_Ch>3) delete TempShapeCh4; // should work if TimeShapeCh4 is defined i.e. is not a null pointer any more

}
	

/////////////////////////////////////  GetHistogram ///////////////////////////////////////////////////////////////////////////////////
/*
TH1F* Data::GetHistogram(int Ch) const {
	if ((Ch=1)&&(No_of_Ch>0)) return TimeShapeCh1;
	if ((Ch=2)&&(No_of_Ch>1)) return TimeShapeCh2;
	if ((Ch=3)&&(No_of_Ch>2)) return TimeShapeCh3;
	if ((Ch=4)&&(No_of_Ch>3)) return TimeShapeCh4;
	
	cout<<"Channel "<<Ch<<" is not defined GetHistogram. Exiting..."<<endl;
	exit(1);
}
*/
TH1F* WaveProcessor::GetTempHist(int Ch) const {
	if ((Ch=1)&&(No_of_Ch>0)) return TempShapeCh1;
	if ((Ch=2)&&(No_of_Ch>1)) return TempShapeCh2;
	if ((Ch=3)&&(No_of_Ch>2)) return TempShapeCh3;
	if ((Ch=4)&&(No_of_Ch>3)) return TempShapeCh4;
	
	cout<<"Channel "<<Ch<<" is not defined GetTempHist. Exiting..."<<endl;
	exit(1);
}

void WaveProcessor::PrintCurrentHist(int ch) const {
	TH1F* tmp;
	char histfilename[60];
	switch (ch) {
		case 1: tmp = TempShapeCh1; break;
		case 2: tmp = TempShapeCh2; break;
		case 3: tmp = TempShapeCh3; break;
		case 4: tmp = TempShapeCh4; break;
	}
	if (DEBUG) cout<<" PRINTCURRENTHIST"<<endl;
	

	if (DEBUG) cout<<"tmp(10ns)inPrint="<<tmp->GetBinContent(tmp->FindBin(10.))<<endl;
	
	sprintf(histfilename, "TempShapeCh%i_event%i_d%im%iy%i_%i:%i:%i::%i.pdf", ch,  eventID, dateStamp.day, dateStamp.month, dateStamp.year, dateStamp.hour, dateStamp.minute, dateStamp.second, dateStamp.milisecond);
	TCanvas *canvTempShape = new TCanvas("TempShape", histfilename,1);
	if (DEBUG) cout<<"Canvas defined"<<endl<<flush;
	

	tmp->Draw("hist");
	
	//TSpectrum *s = new TSpectrum(); 
	//s->Search(tmp,5,"",0.10);

	
	if (DEBUG) cout<<"Histogram Draw"<<endl<<flush;
	canvTempShape->SaveAs(histfilename);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// ANALYSIS //////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Float_t WaveProcessor::GetFWHM(int Ch, Float_t BL) {return WaveProcessor::GetFWHM(Ch, BL, 50.);} // FWHM is at 50 % 

Float_t WaveProcessor::GetFWHM(int Ch, Float_t BL, Float_t height) { // height [%] is the height at which the width is measured FWHM, height = 50 

	TH1F* AnalysisHist;
	int leftFWHM(0), rightFWHM(0), maxBin(0); 
	int i(0);
	Float_t FWHM;
	
	switch (Ch) {
		case 1: AnalysisHist = TempShapeCh1; break;
		case 2: AnalysisHist = TempShapeCh2; break;
		case 3: AnalysisHist = TempShapeCh3; break;
		case 4: AnalysisHist = TempShapeCh4; break;
	}

	
	maxBin = AnalysisHist->GetMaximumBin();
	
	//cout<<"maxVal="<<AnalysisHist->GetBinContent(maxBin)<<", BL="<<BL<<endl;
	
	height = ((AnalysisHist->GetBinContent(maxBin))-BL)/(100./height); // height in [%]
	
	//if ((AnalysisHist->GetBinCenter(maxBin)>70)||(AnalysisHist->GetBinCenter(maxBin)<20)) return 200. ; // this means that something is wrong - probably a spike
	
	while ((leftFWHM==0)||(rightFWHM==0)){
		i++;
		if ((maxBin-i<0)||(maxBin+i>1024)) break; // couldn't find FWHM, will return 0 
		if (leftFWHM==0) leftFWHM = ((AnalysisHist->GetBinContent(maxBin-i))<(height+BL))?(maxBin-i):leftFWHM ;
		if (rightFWHM==0) rightFWHM= ((AnalysisHist->GetBinContent(maxBin+i))<(height+BL))?(maxBin+i):rightFWHM ;
	}	
	FWHM = AnalysisHist->GetBinCenter(rightFWHM) - AnalysisHist->GetBinCenter(leftFWHM);
	
	return FWHM;

}


///////////// give_time_n_amplitude /////////////////////

WaveformParam WaveProcessor::give_waveform_parameters(int Ch) {

WaveformParam output; //how many parameters to be returned

	TH1F* AnalysisHist;
	char histfilename[60];
	int i;
	
	switch (Ch) {
		case 1: AnalysisHist = TempShapeCh1; break;
		case 2: AnalysisHist = TempShapeCh2; break;
		case 3: AnalysisHist = TempShapeCh3; break;
		case 4: AnalysisHist = TempShapeCh4; break;
	}
	
	if (DEBUG) cout<<"TempShapeCh1:"<<TempShapeCh1<<", AnalysisHist:"<<AnalysisHist<<endl;
	


	int ArrivalTimeBin = AnalysisHist->FindFirstBinAbove(triggerHeight); // bin should be transformed to ns according to axis
	
	if (DEBUG) cout<<"ArrivalTimeBin="<<ArrivalTimeBin<<endl;
	
	if (ArrivalTimeBin==-1) { cout<<"In give_waveform_parameters, couldn't find the signal. Empty histogram or the threshold is too high! Exiting..."; exit(1);}
	output.arrivalTime = AnalysisHist->GetXaxis()->GetBinCenter(ArrivalTimeBin);
	
	// Get Amplitude from the fit

	
	/// OPREZ !!! hist->Integral(a,b) - a i b su binovi, ne vrednost x ose
	/// zato ovako mora hist->Integral(hist->FindBin(1) ,hist->FindBin(12)), za vrednosti od 1 ns do 12 ns
	
	/// check Etot subtraction of the baseLine
	output.Etot = AnalysisHist->Integral(ArrivalTimeBin, AnalysisHist->GetXaxis()->GetNbins(), "width"); 
	

	
	// DRS4 writes everything what happened 40 ns before the trigger, when delay=0, otherwise the delay is substracted from 40 ns
	// I took 35 ns, just to have a safe distance.
	output.baseLine = AnalysisHist->Integral(0, AnalysisHist->FindBin(35.-delay), "width") / (35.-delay) ; 
	/*
	 * RMS of the baseLine will be used as a rejection condition
	output->hist->GetXaxis()->SetRange(0, output->hist->FindBin(35.-delay));
	output->Value(baseLineRMS) = output->hist->GetRMS();
	output->hist->GetXaxis()->UnZoom();
	*/
	
	//TSpectrum *s = new TSpectrum(); 
		output.peakMultiplicity = 0; // to do later by other means // s->Search(AnalysisHist,5,"",0.10); // search AnalysisHist histogram for all peaks with 8 % of amplitude of maximum peak
														// Int_t TSpectrum::Search(const TH1 *hin,Double_t sigma = 2, Option_t* option = "", Double_t threshold = 0.05) 	
														// sigma assumes the width of peaks to be found
	
	// this part will say to ignore baseLine if one of the peaks is within its interval of [0, (35.-delay)]
	/// But for now it is commente until Strahinja resolve the problem of the response callibration rotation
	/*
	Double_t *xpeaks = s->GetPositionX();
	for (int i=0; i<output.peakMultiplicity ; i++) {
		if (xpeaks[i]<(35.-delay)) output.baseLine = -100;
		cout<<xpeaks[i]<<endl;
	}
	
	if (output.baseLine != -100) {
		if (baseLineCNT==0) baseLineAVG=0; // baseLineAVG is set to BASELINE (4 mV), just in case if the first event is baseLine = -100
											// if the first event is OK, then no need to use this arbitrary value
		baseLineAVG = (baseLineAVG*((float)baseLineCNT) + output.baseLine)/((float)(baseLineCNT+1));
		baseLineCNT++;
	}
	else output.baseLine=baseLineAVG;
	*/
	
	output.Etot -= output.baseLine*(AnalysisHist->GetXaxis()->GetBinUpEdge(1023) - output.arrivalTime);//(AnalysisHist->GetBinCenter(1023)-(35.-delay));
	
	output.maxVal  = AnalysisHist->GetMaximum() - output.baseLine;	

	TH1F *tmpHist = (TH1F*) AnalysisHist->Clone(); // baseLine should be subtracted in order to get time of 90% energy deposition of real signal (baseLine excluded)
	for (i=0; i<1024; i++){
		tmpHist->SetBinContent(i, (tmpHist->GetBinContent(i)-output.baseLine)); 
	}
	TH1* hcumul = tmpHist->GetCumulative();
	hcumul->Scale(1/tmpHist->Integral()); // normalize cumulative histogram to 1
	output.T90 = AnalysisHist->GetXaxis()->GetBinCenter(hcumul->FindFirstBinAbove(0.9))-output.arrivalTime; 
	output.T70 = AnalysisHist->GetXaxis()->GetBinCenter(hcumul->FindFirstBinAbove(0.7))-output.arrivalTime; 
	output.T50 = AnalysisHist->GetXaxis()->GetBinCenter(hcumul->FindFirstBinAbove(0.5))-output.arrivalTime;	
	
	output.Eof10ns = AnalysisHist->Integral(ArrivalTimeBin, AnalysisHist->FindBin(output.arrivalTime + 10.), "width") 
	- output.baseLine*10. ; // Integral of first 10 ns of signal

	tmpHist->Delete();


/*
	TCanvas *C = new TCanvas("C", histfilename,1);
	if (eventID<10){
		sprintf(histfilename, "tmpHist_event%i.pdf",eventID);
		tmpHist->Draw("hist");
		C->SaveAs(histfilename);
		if (eventID=1) cout<<i<<": BinContent="<<tmpHist->GetBinContent(i)<<", BinWidth="<<tmpHist->GetBinWidth(i)<<", baseLine="<<output.baseLine<<endl;
	}
	*/

/*
		
	func1->SetParameters(output.baseLine, output.maxVal, (AnalysisHist->GetBinCenter(AnalysisHist->GetMaximumBin()) - output.baseLine), 5 );    // da li ovde treba activity*bin????
	func1->SetParNames("Baseline height","Amplitude","Peak position", "Sigma");
	AnalysisHist->Fit("fit1","NR","",(output.arrivalTime - 2.), (output.arrivalTime+10.)) ; // fit of first 10 ns of signal, "" is graphic options like in Draw()
	
	Double_t chi2=func1->GetChisquare(); /// this could be used to reject fit and 
										///  also if p0 is very different from baseLine, it could be indication that something is wrong

	Double_t p0=func1->GetParameter(0);
	Double_t p1=func1->GetParameter(1);
	Double_t p2=func1->GetParameter(2);
	Double_t p3=func1->GetParameter(3);
	Double_t e0=func1->GetParError(0);
	Double_t e1=func1->GetParError(1);
	Double_t e2=func1->GetParError(2);
*/	
	output.fittedAmplitude = 0;// (float)p1; 
	
	return output;
}

Observables *WaveProcessor::ProcessOnline( Float_t* RawTimeArr,
    Float_t* RawVoltArr,
    Int_t RawArrLength)
{
  return ProcessOnline( RawTimeArr, RawVoltArr, RawArrLength, triggerHeight, delay);
}

Observables* WaveProcessor::ProcessOnline( Float_t* RawTimeArr,
                                           Float_t* RawVoltArr,
                                           Int_t RawArrLength,
                                           float threshold,
                                           float baselineWidth)
{
  Observables *output = new Observables;
	int i;

  output->hist = new TH1F("RawTempShape", "RawTempShape", RawArrLength - 1, RawTimeArr); // nonequidistand histogram

	for (i=1; i<=RawArrLength; i++) output->hist -> SetBinContent(i, -RawVoltArr[i]);
	
	// baseLine must be calculated first.
	const int startBin = 3; // Avoid spike at the beginning
  int endBin = output->hist->FindBin(180.); // Avoid ripples at the end
	int blEndBin = output->hist->FindBin(baselineWidth);

	output->Value(baseLine) = output->hist->Integral(startBin, blEndBin, "width") / baselineWidth ;
  output->Value(baseLineRMS) = CalcHistRMS(output->hist, startBin, blEndBin);
//	output->Value(maxVal) = output->hist->GetBinContent(output->hist->GetMaximumBin()) - output->Value(baseLine);

	output->Value(arrivalTime) = ArrivalTime(output->hist, output->Value(maxVal),
	                                         threshold, output->Value(baseLine),
	                                         startBin, endBin, 3, 0.25);
	int ArrivalTimeBin = output->hist->FindBin(output->Value(arrivalTime));


	// baseline subtraction
	TH1F *tmpHist = static_cast<TH1F*>(output->hist->Clone()); // baseLine should be subtracted in order to get time of 90% energy deposition of real signal (baseLine excluded)
	for (i=0; i<RawArrLength; i++) tmpHist->SetBinContent(i, (tmpHist->GetBinContent(i)-output->Value(baseLine)));

	int firstIntegrationBin = max(startBin, ArrivalTimeBin - 5);
  int lastPromptIntegBin = output->hist->FindBin(output->Value(arrivalTime) + 20.);

	output->Value(eTot) = tmpHist->Integral(firstIntegrationBin, endBin, "width");
	
	output->Value(ePrompt) = tmpHist->Integral(firstIntegrationBin, lastPromptIntegBin, "width");

	delete tmpHist;

	return output;
}


///////////// reset_temporary_histograms() //////////////

void WaveProcessor::reset_temporary_histograms(){
	TempShapeCh1->Reset();
	if (No_of_Ch>1) TempShapeCh2->Reset(); 
	if (No_of_Ch>2) TempShapeCh3->Reset();
	if (TempShapeCh4) TempShapeCh4->Reset(); // should work if TimeShapeCh4 is defined i.e. is not a null pointer any more
	
	}

float WaveProcessor::CalcHistRMS(const TH1F *hist, int first, int last){
  TH1F *RMShist;
  Float_t width;

  Double_t max = hist->GetMaximum();
  Double_t min = hist->GetMaximum();

  RMShist = new TH1F("RMShist", "RMShist", 100, min, max);

  for(int i=first; i<=last; i++){
    width = hist->GetXaxis()->GetBinWidth(i);
    RMShist->Fill(hist->GetBinContent(i)); // longer integral measurement carries less of information about the time variation
  }

  float rms = RMShist->GetRMS();
  delete RMShist;
  return rms;

}


float WaveProcessor::MeanAndRMS(const TH1F *hist, int first, int last, float &mean, float &rms){

  if ( !hist ) return -1.;
  if ( first < 1 || first > hist->GetNbinsX() || last < 1 || last > hist->GetNbinsX() ) {
    return -1;
  }
  if ( last < first ) {
    int temp = first;
    first = last;
    last = temp;
  }

  float sum = 0, sumSq = 0;

  for(int i=first; i<=last; i++){
    float c = hist->GetBinContent(i);
    sum += c;
    sumSq += c*c;
  }

  int nBins = (last - first + 1);
  mean = sum / nBins;
  float meanSq = sumSq / nBins;
  rms = sqrt( meanSq - mean*mean );
  return mean;

}


float WaveProcessor::ArrivalTime(TH1F* hist, float &maxVal, float threshold, float baseline,
                                 int firstBin, int lastBin, float risetime, float fraction)
{

  if ( !hist ) return -1.;

  // First-order correction of maxVal:
  // Parabolic shape of the pulse peak is assumed
  // and maxVal is extracted by solving a system of
  // 3 eqs. with 3 unknowns, with parameters v1-3 and t1-3.
  int maxBin = firstBin;
  double maxSample = hist->GetBinContent(maxBin) - baseline;

  for (int ibin=firstBin; ibin<lastBin; ibin++) {
    double sample = hist->GetBinContent(ibin) - baseline;
    if (sample > maxSample) {
      maxSample = sample;
      maxBin = ibin;
    }
  }
  assert(maxBin<925);
  double t1 = hist->GetBinLowEdge(maxBin-1);
  double t2 = hist->GetBinLowEdge(maxBin);
  double t3 = hist->GetBinLowEdge(maxBin+1);
  double deltat = (t3-t1)/2;
  double v1 = hist->GetBinContent(maxBin-1) - baseline;
  double v2 = hist->GetBinContent(maxBin) - baseline;
  double v3 = hist->GetBinContent(maxBin+1) - baseline;
  double a = (2*v2 - v3 - v1) / 2 / (deltat*deltat);
  //double tmax = t2 + (v3 - v1) / (4*a*deltat);
  double dmax = pow((v1-v3)/deltat, 2) / 2 / a;
  maxVal = v2 + dmax;
  /**/
  //double maxVal = maxSample;

  if (maxSample < threshold) return -1.;

  float tt1 = 0;
  int nBint1 = 0;

  for (int ibin=firstBin; ibin<=maxBin; ibin++) {

    if (hist->GetBinContent(ibin) - baseline > threshold) {
      tt1 = hist->GetBinLowEdge(ibin);
      nBint1 = ibin;
      break;
    }
  }

  // Return simple threshold crossing point (uncomment for debugging)
 // return tt1;

  assert(tt1<180);

  if (0.9*maxSample< threshold) {
    return tt1 - risetime;
  }

  if (maxSample*fraction <= threshold) {

    for (int ibin=nBint1; ibin<=maxBin; ibin++) {

      if (hist->GetBinContent(ibin) - baseline > 0.9*maxSample) {
        return hist->GetBinLowEdge(ibin) - risetime;
      }
    }

  } // maxSample*fraction <= threshold

  /*** Constant fraction for tall signals ***/

  int midFitBin = 0;
  int endFitBin = 0;
  int startFitBin = 0;

  // Fraction how far to integrate in both directions from the fraction crossing point
  float fspan = 0.5;
  // timeWing is the time for the pulse to rise by fspan*fraction*maxVal
  float timeWing = fspan*fraction*risetime/(0.9-fraction);
  int nBinsTimeWing = static_cast<int>(timeWing/200*kNumberOfBins + 0.5);


  for (int ibin=nBint1; ibin<=maxBin; ibin++) {
    if (hist->GetBinContent(ibin) - baseline > fraction*maxVal) {
      midFitBin = ibin;
      break;
    }
  }
  startFitBin = midFitBin - nBinsTimeWing;
  endFitBin = midFitBin + nBinsTimeWing;

  assert (startFitBin <= endFitBin);
  if (startFitBin == endFitBin) {
    startFitBin--;
    endFitBin++;
  }

  if (startFitBin+1 == endFitBin) {
    endFitBin++;
  }

  assert(endFitBin<925);

  float sumt=0, sumw=0;

  for (int ibin=startFitBin; ibin<endFitBin; ibin++) {
    // weighting
    float w = 0.;
    float v = hist->GetBinContent(ibin) - baseline;
    float relDistance = fabs(v/maxVal/fraction-1.)/fspan;
    if (relDistance < 1.) {
      float linear = 1. - relDistance;
    // w = pow(linear, 2); // cusp
     w = linear; // triangle
    //  w = sqrt(linear); // bulged triangle
    //   w = 1.; // Constant
    //  w = exp(-2*relDistance*relDistance); // Gaussian (2 sigma)
    //  w = exp(-relDistance); // Exponential
    }
    assert(w<=1.&&w>=0);
    sumt += hist->GetBinLowEdge(ibin) * w;
    sumw += w;
  }

  float tarr = 0;

    if (sumw==0) {
      tarr = hist->GetBinLowEdge(midFitBin);
    }
    else {
      // Proper result:
      tarr = sumt / sumw;
    } //  t0 = v0/maxVal; // Replacing time by actual fraction (for debugging)

  return tarr ;
}


TH1* WaveProcessor::FilterFFTofCurrentHist(int ch){ // to finish it if needed
	TH1F* tmp;
	char histfilename[60];
	int n(1024), i ;
	switch (ch) {
		case 1: tmp = TempShapeCh1; break;
		case 2: tmp = TempShapeCh2; break;
		case 3: tmp = TempShapeCh3; break;
		case 4: tmp = TempShapeCh4; break;
	}
	sprintf(histfilename, "FFTCh%i_event%i_d%im%iy%i_%i:%i:%i::%i.root", ch,  eventID,
	    dateStamp.day, dateStamp.month, dateStamp.year, dateStamp.hour,
	    dateStamp.minute, dateStamp.second, dateStamp.milisecond);
	TCanvas *canvFFT = new TCanvas("FFT", histfilename,1);
	
	TH1 *histMagnitude =0;
	TVirtualFFT::SetTransform(0);
	histMagnitude = tmp->FFT(histMagnitude, "MAG");

   histMagnitude->SetTitle("Magnitude of the 1st transform");
   histMagnitude->Draw();
	// canvFFT->SaveAs(histfilename); // to check the graph
	
	TH1 *histPhase = 0;
   histPhase = tmp->FFT(histPhase, "PH");
	
	TVirtualFFT *fft = TVirtualFFT::GetCurrentTransform();
	   
	Double_t *re_full = new Double_t[n];
   Double_t *im_full = new Double_t[n];
   fft->GetPointsComplex(re_full,im_full);
   
   // Frequency filter (the noise should be checked on the histograms and then apply the filter)
   
   // this should be re-made, but if found to be necessary
   for (i=90;i<130;i++){ // there was a bump at this position, apparently coming from the noise of 10 ns period seen on the waveform
	   re_full[i]=25;
	   im_full[i]=25;
	
   }
	

 //inverse transform:
   TVirtualFFT *fft_inverse = TVirtualFFT::FFT(1, &n, "C2R M K");
   fft_inverse->SetPointsComplex(re_full,im_full);
   fft_inverse->Transform();
   TH1 *hFiltered = 0;

   hFiltered = TH1::TransformHisto(fft_inverse,hFiltered,"Re");
//   hFiltered->SetTitle("inverse transform filtered");
//   hFiltered->Draw();
   
//	canvFFT->SaveAs("inverse.root");

return hFiltered;
	
	}
