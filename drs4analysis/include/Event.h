#ifndef Event_hpp
#define Event_hpp

#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <iostream>
#include <math.h>

using namespace std ;

namespace H4DAQ
{
#define WORDSIZE 4
  typedef unsigned long long dataTypeSize_t;
  typedef uint32_t WORD;

#define MAX_ADC_CHANNELS 100000
#define MAX_DIGI_SAMPLES 100000
#define MAX_TDC_CHANNELS 200
#define MAX_SCALER_WORDS 16
#define MAX_PATTERNS 16
#define MAX_PATTERNS_SHODO 16
#define SMALL_HODO_X_NFIBERS 8
#define SMALL_HODO_Y_NFIBERS 8
#define MAX_TRIG_WORDS 32
#define MAX_RO 100



  // data format used as bridge between the high level structures and the root tree
  struct treeStructData
  {
    unsigned int runNumber ;
    unsigned int spillNumber ;
    unsigned int evtNumber ;
    unsigned int evtTimeDist ;
    unsigned int evtTimeStart ;

    unsigned int 	nEvtTimes ;
    ULong64_t 	evtTime [MAX_RO] ;
    unsigned int 	evtTimeBoard [MAX_RO] ;

    //  unsigned int triggerBits ;

    unsigned int nAdcChannels ;

    unsigned int adcBoard[MAX_ADC_CHANNELS] ;
    unsigned int adcChannel[MAX_ADC_CHANNELS] ;
    unsigned int adcData[MAX_ADC_CHANNELS] ;

    unsigned int nDigiSamples ;
    unsigned int digiFrequency[MAX_DIGI_SAMPLES] ;
    unsigned int digiStartIndexCell[MAX_DIGI_SAMPLES] ;
    unsigned int digiGroup[MAX_DIGI_SAMPLES] ;
    unsigned int digiChannel[MAX_DIGI_SAMPLES] ;
    unsigned int digiBoard[MAX_DIGI_SAMPLES] ;
    unsigned int digiSampleIndex[MAX_DIGI_SAMPLES] ;
    uint16_t digiSampleValue[MAX_DIGI_SAMPLES] ;
    float digiSampleCalibValue[MAX_DIGI_SAMPLES] ;
    float digiSampleTime[MAX_DIGI_SAMPLES] ;

    unsigned int nTdcChannels ;
    unsigned int tdcBoard[MAX_TDC_CHANNELS] ;
    unsigned int tdcChannel[MAX_TDC_CHANNELS] ;
    unsigned int tdcData[MAX_TDC_CHANNELS] ;

    unsigned int nScalerWords ;
    WORD scalerWord[MAX_SCALER_WORDS] ;
    unsigned int scalerBoard[MAX_SCALER_WORDS] ;

    unsigned int nPatterns ;
    WORD pattern[MAX_PATTERNS] ;
    WORD patternBoard[MAX_PATTERNS] ;
    WORD patternChannel[MAX_PATTERNS] ;

    unsigned int nTriggerWords;
    unsigned int triggerWords[MAX_TRIG_WORDS] ;
    unsigned int triggerWordsBoard[MAX_TRIG_WORDS] ;

  } ;

  struct adcData
  {
    unsigned int board ;
    unsigned int channel ;
    unsigned int adcReadout ;
  } ;

  struct tdcData
  {
    unsigned int board ;
    unsigned int channel ;
    unsigned int tdcReadout ;
  } ;

  struct digiData
  {
    unsigned int board ;
    unsigned int group ;
    unsigned int frequency ;
    unsigned int startIndexCell ;
    unsigned int channel ;
    unsigned int sampleIndex ; //raw index
    uint16_t sampleRaw; // raw sample
    float sampleTime; //calibrated time
    float sampleValue; //calibrated sample
  } ;

  struct patternData
  {
    unsigned int board ;
    unsigned int channel ;
    unsigned int patternValue;
  };

  struct triggerWordData
  {
    unsigned int board;
    unsigned int triggerWord;
  };

  struct scalerData
  {
    unsigned int board ;
    unsigned int scalerValue;
  };

  struct timeData
  {
    unsigned int board;
    uint64_t time;
  };

  struct eventId
  {
    unsigned int runNumber;
    unsigned int spillNumber;
    unsigned int evtNumber;
  };

  struct Event
  {
    Event (TTree * outTree) :
    outTree_ (outTree) 
    { 
      createOutBranches (outTree_, thisTreeEvent_) ;   
    }

    ~Event () { }

    eventId id;
    std::vector<triggerWordData> 		triggerWords ;
    std::vector<bool> 	triggerBits ;
    std::vector<adcData> 	adcValues ; 
    std::vector<tdcData> 	tdcValues ; 
    std::vector<digiData> digiValues ; 
    std::vector<scalerData> 	scalerWords ; 
    std::vector<patternData> 	patterns ; 

    unsigned int 		evtTimeDist ;
    unsigned int 		evtTimeStart ;

    vector<timeData> 	evtTimes ;

    void clear () ;
    void Fill () ;

  private :
  
    TTree * outTree_ ;
    treeStructData thisTreeEvent_ ;

    // move events from the structures to the variables to be put into the root tree
    void fillTreeData (treeStructData & treeData) ;
    void createOutBranches (TTree* tree,treeStructData& treeData) ;

  } ;

}
#endif
