#include "Data.h"

Data::Data() {aligned=false; alocated=false; baseLineAVG=BASELINE ; baseLineCNT=0;}

Data::~Data(){
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

void Data::set_time_calibration(int ch, int bin, float value){
	aligned = false; // any change of time induce that the alignment is no more valid
    TimeBinWidth[ch][bin]=value;
}
float Data::get_time_calibration(int ch, int bin ) const {return TimeBinWidth[ch][bin];}

void Data::set_bin_time_n_voltage(int ch, int bin, USHORT voltage, USHORT range, USHORT trigger_Cell){
	float aux_f=0; 
	aligned=false; // any change of time induce that the alignment is no more valid
	
	BinVoltage[ch][bin]=(0.5 - ((float)voltage)/65536. + ((float)range)/1000.); // this way the amplitude is inverted (positive in our case)
	for (int k=0; k<bin ; k++)	aux_f += TimeBinWidth[ch][((k+trigger_Cell) % 1024)];
	TimeBinVoltage[ch][bin]=aux_f;
}

//void Data::set_voltage_bin_total(int ch, int bin, float value) {BinVoltage[ch][bin]=value;}

float Data::get_voltage_bin_total(int ch, int bin) const { return BinVoltage[ch][bin];}

/*
void Data::set_time_of_bin_total(int ch, int bin, float value){
	aligned=false; // any change of time induce that the alignment is no more valid
    TimeBinVoltage[ch][bin]=value;
}
*/
float Data::get_time_of_bin_total(int ch, int bin) const { return TimeBinVoltage[ch][bin]; }


/////////////////////////////////// allignCells0 //////////////////////////////////////////////////////////////////////////

void Data::allignCells0(USHORT trigger_cell){ // align cell #0 of all channels
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

void Data::CreateTempHistograms(){
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

void Data::DeleteTempHistograms(){
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
TH1F* Data::GetTempHist(int Ch) const {
	if ((Ch=1)&&(No_of_Ch>0)) return TempShapeCh1;
	if ((Ch=2)&&(No_of_Ch>1)) return TempShapeCh2;
	if ((Ch=3)&&(No_of_Ch>2)) return TempShapeCh3;
	if ((Ch=4)&&(No_of_Ch>3)) return TempShapeCh4;
	
	cout<<"Channel "<<Ch<<" is not defined GetTempHist. Exiting..."<<endl;
	exit(1);
}

void Data::PrintCurrentHist(int ch) const {
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
	
	TSpectrum *s = new TSpectrum(); 
	s->Search(tmp,5,"",0.10);

	
	if (DEBUG) cout<<"Histogram Draw"<<endl<<flush;
	canvTempShape->SaveAs(histfilename);
	

	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// ANALYSIS //////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////// give_time_n_amplitude /////////////////////

WaveformParam Data::give_waveform_parameters(int Ch) {

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
	
	output.maxVal  = AnalysisHist->GetMaximum();
	
	// DRS4 writes everything what happened 40 ns before the trigger, when delay=0, otherwise the delay is substracted from 40 ns
	// I took 35 ns, just to have a safe distance.
	output.baseLine = AnalysisHist->Integral(0, AnalysisHist->FindBin(35.-delay), "width") / (35.-delay) ; 
	
	
	TSpectrum *s = new TSpectrum(); 
		output.peakMultiplicity = s->Search(AnalysisHist,5,"",0.10); // search AnalysisHist histogram for all peaks with 8 % of amplitude of maximum peak
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

WaveformOnline Data::ProcessOnline(Float_t* RawTimeArr, Float_t* RawVoltArr, Int_t RawArrLength){
	WaveformOnline output;
	int i;
	char histTitle[30];
	
	if (RawTempShape!=0) RawTempShape->Delete();

	RawTempShape = new TH1F("RawTempShape", "RawTempShape", RawArrLength - 1, RawTimeArr); // nonequidistand histogram
	
	//for(i=0; i<RawArrLength; i++) RawTempShape -> SetBinContent((i+RawTrigCell)%RawArrLength, RawVoltArr[i]);
	for (i=0; i<RawArrLength; i++) RawTempShape -> Fill(RawTimeArr[i], RawVoltArr[i]);
	
	int ArrivalTimeBin = RawTempShape->FindFirstBinAbove(triggerHeight); // bin should be transformed to ns according to axis
	if (ArrivalTimeBin==-1) return output; // I guess will be 0 and the event ignored
	
	TH1F *tmpHist = (TH1F*) RawTempShape->Clone(); // baseLine should be subtracted in order to get time of 90% energy deposition of real signal (baseLine excluded)
	for (i=0; i<RawArrLength; i++) tmpHist->SetBinContent(i, (tmpHist->GetBinContent(i)-output.baseLine)); 
	TH1* hcumul = tmpHist->GetCumulative();
	hcumul->Scale(1/tmpHist->Integral()); // normalize cumulative histogram to 1
	
		
	output.arrivalTime = RawTempShape->GetXaxis()->GetBinCenter(ArrivalTimeBin);
	output.baseLine = RawTempShape->Integral(0, RawTempShape->FindBin(35.-delay), "width") / (35.-delay) ;
	output.Etot = RawTempShape->Integral(ArrivalTimeBin, RawTempShape->GetXaxis()->GetNbins(), "width") 
				- output.baseLine*(RawTempShape->GetXaxis()->GetBinUpEdge(1023)- output.arrivalTime);//(35.-delay));
	output.maxVal  = RawTempShape->GetMaximum();
	output.T90 = RawTempShape->GetXaxis()->GetBinCenter(hcumul->FindFirstBinAbove(0.9))-output.arrivalTime; 
	output.T70 = RawTempShape->GetXaxis()->GetBinCenter(hcumul->FindFirstBinAbove(0.7))-output.arrivalTime; 
	output.T50 = RawTempShape->GetXaxis()->GetBinCenter(hcumul->FindFirstBinAbove(0.5))-output.arrivalTime;	
	
	
	
	output.Eof10ns = RawTempShape->Integral(ArrivalTimeBin, RawTempShape->FindBin(output.arrivalTime + 10.), "width") 
	- output.baseLine*10. ; // Integral of first 10 ns of signal
	

	
	output.hist = RawTempShape;
	
	tmpHist->Delete();

	return output;
}


///////////// reset_temporary_histograms() //////////////

void Data::reset_temporary_histograms(){
	TempShapeCh1->Reset();
	if (No_of_Ch>1) TempShapeCh2->Reset(); 
	if (No_of_Ch>2) TempShapeCh3->Reset();
	if (TempShapeCh4) TempShapeCh4->Reset(); // should work if TimeShapeCh4 is defined i.e. is not a null pointer any more
	
	}


