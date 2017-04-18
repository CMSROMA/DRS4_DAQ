/*
 * observables.h
 *
 * Declaration of struct observables
 *
 *  Created on: Apr 16, 2017
 *      Author: S. Lukic
 */

#ifndef DRS4DATA_INCLUDE_OBSERVABLES_H_
#define DRS4DATA_INCLUDE_OBSERVABLES_H_

#include <map>

#include "TString.h"

class TH1F;

namespace DRS4_data {

  struct Observable {
    Observable(const char *_name, const char *_title, const char *_unit, float _value) :
      name(_name), title(_title), unit(_unit), value(_value)
    { }
    const TString name;
    const TString title;
    const TString unit;
    float value;
  };


  enum kObservables { eTot,             // Total integral of signal
                      ePrompt,          // Prompt integral (first 10 ns)
                      arrivalTime,      // Time of the leading edge
                      dt50,             // Time of collection of 50% of energy after leading edge
                      dt70,             // Time of collection of 70% of energy after leading edge
                      dt90,             // Time of collection of 90% of energy after leading edge
                      baseLine,         // baseline
                      maxVal,           // Peak value
                      nObservables      // Keep this because config needs this number at compile time
                    };


  class Observables {

  public:
    Observables() : hist(NULL) {
      observables.insert(std::pair<kObservables, Observable>(eTot, Observable("eTot", "E_{tot}", "mV*ns", 0)));
      observables.insert(std::pair<kObservables, Observable>(ePrompt, Observable("ePrompt", "E_{prompt}", "mV*ns", 0)));
      observables.insert(std::pair<kObservables, Observable>(arrivalTime, Observable("arrivalTime", "t_{arrival}", "ns", -1000)));
      observables.insert(std::pair<kObservables, Observable>(dt50, Observable("dt50", "#Delta t_{50%}", "ns", -100)));
      observables.insert(std::pair<kObservables, Observable>(dt70, Observable("dt70", "#Delta t_{70%}", "ns", -100)));
      observables.insert(std::pair<kObservables, Observable>(dt90, Observable("dt90", "#Delta t_{90%}", "ns", -100)));
      observables.insert(std::pair<kObservables, Observable>(baseLine, Observable("baseLine", "Baseline", "mV", 0)));
      observables.insert(std::pair<kObservables, Observable>(maxVal, Observable("maxVal", "Peak value", "mV", 0)));
    }

    float& Value(kObservables obs) { return observables.at(obs).value; }
    const char * Name(kObservables obs) const { return observables.at(obs).name.Data(); }
    const char * Title(kObservables obs) const { return observables.at(obs).title.Data(); }
    const char * Unit(kObservables obs) const { return observables.at(obs).unit.Data(); }

    unsigned NObservables() const { return observables.size(); }

    TH1F* hist;

  protected:
    std::map<kObservables, Observable> observables;

  };

}



#endif /* DRS4DATA_INCLUDE_OBSERVABLES_H_ */
