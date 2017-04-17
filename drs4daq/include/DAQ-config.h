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
	config();
	~config();

	static const short stacklen=1024;

	int ParseOptions(std::ifstream *input);
	int DumpOptions() const;
// Options defining the behaviour of the display
	unsigned int _w, _h;
	short histolo[DRS4_data::nObservables], histohi[DRS4_data::nObservables];
	int _tRed, _xRed, _yRed;

// Options to setup the CC-USB
	unsigned char trigDelay;  // Trigger delay in ns

};


#endif /* DAQ_CONFIG_HH_ */
