#ifndef WAVEPROCESSOR_H
#define WAVEPROCESSOR_H

#define DEBUG 0
#define DEBUG2 0 // 
#define DEBUG3 0 // CreateHistograms debug (

#define BASELINE 4

// root

#include "TH1F.h"
#include "TF1.h"
#include "TMath.h"
#include "TCanvas.h"
//#include "TSpectrum.h"

#include "observables.h"



using namespace std;

typedef unsigned short USHORT;

struct date {
    USHORT year;
    USHORT month;
    USHORT day;
    USHORT hour;
    USHORT minute;
    USHORT second;
    USHORT milisecond;
    };

struct WaveformParam {
    Float_t arrivalTime;
    Float_t Etot;
    Float_t T90;
    Float_t T70;
    Float_t T50;
    Float_t maxVal;
    Float_t baseLine;
    Int_t peakMultiplicity;
    Float_t fittedAmplitude;
    Float_t Eof10ns;
};

/*
struct WaveformOnline {
    Float_t arrivalTime;
    Float_t Etot;
    Float_t maxVal;
    Float_t baseLine;
    Float_t Eof10ns;
    Float_t T90;
    Float_t T70;
    Float_t T50;
    TH1F* hist;
};
*/


class WaveProcessor {

// TimeBinWidth, BinVoltage are showing to which channel they belong to 
// BinVoltage[1][321]CH1, bin 321 CH STARTS WITH 1 !!!!! to corespond to real channel, mark 0 is ignored
// bin starts as usual - form 0

    public:

    WaveProcessor();   //constructor
    ~WaveProcessor();  //destructor

    // setget DRS4 waveform analysis parameters
    void setTriggerHeight(float value) {triggerHeight=value;}
    float getTriggerHeght() const {return triggerHeight;}
    void setDelay(float value) {delay = value;}
    float getDelay() const {return delay;}
    void setEventID(unsigned int value) {eventID = value;}
    int getEventID() const {return eventID;}
    void setDateStamp (date value) {dateStamp=value;}


    void SetNoOfChannels(int Ch) {No_of_Ch = Ch;}
    int GetNoOfChannels() {return No_of_Ch;}
    //void allocate_memory(int) ; // alocate memery for 1024 words of TimeBinWidth, BinVoltage times given "int" number of channels
    void set_time_calibration (int,int,float); // CH, bin, value
    float get_time_calibration (int,int) const;
    void set_bin_time_n_voltage (int, int, USHORT, USHORT, USHORT);
                    //void set_voltage_bin_total (int,int,float); // CH, bin, value
    float get_voltage_bin_total (int,int) const;
                    //void set_time_of_bin_total (int,int,float); // CH, bin, value
    float get_time_of_bin_total (int,int) const;

    /// ch1 is allways the referent one !!!!!!!
    void allignCells0(unsigned short); // align cell #0 of all channels
    void CreateTempHistograms(); // fill the histograms with the colected waveforms of one event, one histogram per channel
    void DeleteTempHistograms(); // they must be deleted at the end of each event
    //TH1F* GetHistogram(int) const; // return the histogram of given chanel
    TH1F* GetTempHist(int) const;
    void PrintCurrentHist(int) const; // print pdf of temporary histogram (TempShape) of given channel

// analysis methods
    void reset_temporary_histograms();
    WaveformParam give_waveform_parameters(int);  // gives time and amplitude of a given channel
    DRS4_data::Observables* ProcessOnline(Float_t* , Float_t* , Int_t);
    static DRS4_data::Observables* ProcessOnline(Float_t* time, Float_t* amplitude, Int_t length, float threshold, float trigDelay);

    Float_t GetFWHM(int, Float_t);
    Float_t GetFWHM(int, Float_t, Float_t); // third float is in percent, 50% for FWHM, 10 % - width at 10
    
    TH1* FilterFFTofCurrentHist(int);

    private:
    
    static float CalcHistRMS(const TH1F*, int, int );
    static float MeanAndRMS(const TH1F*, int first, int last, float &mean, float &rms);
    static float ArrivalTime(TH1F*, float threshold, float baseline,
                                    float risetime, float fraction = 0.5);
    
    float triggerHeight;
    float delay;
    unsigned int eventID;
    date dateStamp;
    float baseLineAVG;
    int baseLineCNT;
    bool alocated; //flag for allocation of memory
    bool aligned; // flag that the 0 cells of chanels are aligned
    int No_of_Ch;
    
    float TimeBinWidth[5][1024]; // this is the time width of given bin according to the calibration
    float BinVoltage[5][1024]; // this is the voltage of the given bin, for us (-0.5 V, +0.5 V)
    float TimeBinVoltage[5][1024]; // this is the time in ns between the trigger and the given bin

/*   
    // histograms of sums of all signals
    TH1F* TimeShapeCh1;
    TH1F* TimeShapeCh2;
    TH1F* TimeShapeCh3;     
    TH1F* TimeShapeCh4;
*/
   
    // should contain the shape of only one event
    TH1F* TempShapeCh1;
    TH1F* TempShapeCh2;
    TH1F* TempShapeCh3;     
    TH1F* TempShapeCh4;
    
    TH1F* RawTempShape;
  
    
    
};
#endif //WAVEPROCESSOR_H
