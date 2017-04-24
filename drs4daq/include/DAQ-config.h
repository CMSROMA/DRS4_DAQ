/*
 * DAQ-config.h
 *
 *  Created on: Jun 27, 2015
 *      Author: straja
 */

#ifndef DAQ_CONFIG_HH_
#define DAQ_CONFIG_HH_

#include "observables.h"


class config {
public:
	config(DRS4_data::Observables *);
	~config();

	static const short stacklen=1024;

	int ParseOptions(std::ifstream *input);
	int DumpOptions() const;

  /*** Options controlling the run ***/
	unsigned nEvtMax;

  /*** Options defining the behaviour of the display ***/
	unsigned int _w, _h;
	short histolo[DRS4_data::nObservables], histohi[DRS4_data::nObservables];
	int histoNbins[DRS4_data::nObservables];
	// Fixme: Add number of bins
	int _tRed, _tRed2D, _xRed, _yRed;

  /*** Options to setup the DRS4 evaluation board ***/
	double sampleRate;
	double inputRange;
	double triggerLevel;
	bool   triggerNegative;
	unsigned trigDelay;  // Trigger delay in ns
	int triggerSource;

private:
	DRS4_data::Observables *obs;

};


#endif /* DAQ_CONFIG_HH_ */
