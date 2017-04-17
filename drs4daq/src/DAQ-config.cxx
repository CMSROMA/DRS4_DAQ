/*
 * DAQ-config.cxx
 *
 *  Created on: Jun 28, 2015
 *      Author: straja
 */

#include "Riostream.h"
#include "TString.h"
#include "TRegexp.h"

#include "DAQ-config.h"

config::config() :
	_w(600), _h(350),
	_tRed(10), _xRed(2), _yRed(2),
	trigDelay(150)
{
	for(int i=0; i<DRS4_data::nObservables; i++) {
		histolo[i] = 0; histohi[i] = 400;
	}
}

config::~config() { }


int config::DumpOptions() const {
	std::cout << "\nDump of the display configuration:\n\n";
	std::cout << "Frame dimensions: " << _w << " x " << _h << std::endl;
	std::cout << "Histogram ranges: \n";
	for (int i=0; i<4; i++) {
		std::cout << Form("ADC%i", i+1) << " " << adclo[i] << " - " << adchi[i] << std::endl;
		std::cout << Form("TDC%i", i+1) << " " << tdclo[i] << " - " << tdchi[i] << std::endl;
	}

	std::cout << "\nDump of the DAQ configuration:\n\n";
	std::cout << "Stack:\n";
	std::cout << std::hex << stack[0] << std::endl;
	for (int i=1; i<=stack[0]; i++) {
		std::cout << "0x" << std::hex << stack[i] << std::endl;
	}
	std::cout << "Buffer len: " << std::dec << bufLen <<  std::endl;
	std::cout << "Trigger delay: " << (short)trigDelay <<  std::endl;
	std::cout << "LAM timeout: " << (short)timeoutLAM <<  std::endl;
	std::cout << "LAM mask: 0x" << std::hex << maskLAM << std::endl;
	std::cout << "Bulk transfer n buffers: " << std::dec << (int)nBufferBulk << std::endl;
	std::cout << "Bulk transfer timeout: " << std::dec << (int)timeoutBulk << std::endl;

	return 0;
}

int config::ParseOptions(std::ifstream *input)
{
	const TRegexp number_patt("[' ''-''+'][0-9]+");
	TString line;
	while(!input->eof()) {
		line.ReadLine(*input);
//		std::cout << line.Data() << "\n";

		if(line.BeginsWith("#") || line.BeginsWith("!")) continue;


		if(line.Contains("simulation", TString::kIgnoreCase)) {
			_simulation = true;
		}


		/**** Display appearance and behaviour ****/

		if(line.Contains("window", TString::kIgnoreCase)) {
			TSubString wid = line(number_patt);
			TString hgt = line(number_patt, wid.Start()+wid.Length());
			_w = TString(wid).Atoi();
			if (_w <= 0) {
				std::cout << "Error parsing options: Window width " << _w << " not allowed.\n";
				return -1;
			}
			_h = hgt.Atoi();
			if (_h <= 0) {
				std::cout << "Error parsing options: Window height " << _h << " not allowed.\n";
				return -1;
			}
			continue;
		}

		TString achan = line(TRegexp("ADC[1-4]"));
		if(achan.Length() > 1) {
			TSubString lo = line(number_patt);
			TString hi = line(number_patt, lo.Start()+lo.Length());
			int idx = TString(achan(TRegexp("[1-4]"))).Atoi() - 1;
			adclo[idx] = TString(lo).Atoi();
			adchi[idx] = TString(hi).Atoi();
			if(adclo[idx] == adchi[idx]) {
				std::cout << "Error parsing options: " << achan.Data() << " limits equal ("
						<< adclo[idx] << ", " << adchi[idx] << ")\n";
				return -1;
			}
			if(adclo[idx] > adchi[idx]) {
				short tmp = adclo[idx]; adclo[idx] = adchi[idx];
				adchi[idx] = tmp;
			}
			continue;
		}


		TString tchan = line(TRegexp("TDC[1-4]"));
		if(tchan.Length() > 1) {
			TSubString lo = line(number_patt);
			TString hi = line(number_patt, lo.Start()+lo.Length());
			int idx = TString(tchan(TRegexp("[1-4]"))).Atoi() - 1;
			tdclo[idx] = TString(lo).Atoi();
			tdchi[idx] = TString(hi).Atoi();
			if(tdclo[idx] == tdchi[idx]) {
				std::cout << "Error parsing options: ADC1 limits equal ("
						<< tdclo[idx] << ", " << tdchi[idx] << ")\n";
				return -1;
			}
			if(tdclo[idx] > tdchi[idx]) {
				short tmp = tdclo[idx]; tdclo[idx] = tdchi[idx];
				tdchi[idx] = tmp;
			}
			continue;
		}


		if(line.Contains("reduction", TString::kIgnoreCase)) {
			TSubString t = line(number_patt);
			TSubString x = line(number_patt, t.Start()+t.Length());
			TSubString y = line(number_patt, x.Start()+x.Length());
			_tRed = TString(t).Atoi();
			if (_tRed <= 0) {
				std::cout << "Time reduction value " << _tRed << " is not positive.\n";
				return -1;
			}
			_xRed = TString(x).Atoi();
			if (_xRed <= 0) {
				std::cout << "X reduction value " << _xRed << " is not positive.\n";
				return -1;
			}
			_yRed = TString(y).Atoi();
			if (_yRed <= 0) {
				std::cout << "Y reduction value " << _yRed << " is not positive.\n";
				return -1;
			}
			continue;
		}


		/**** CC-USB parameters ****/

		if(line.Contains("stack", TString::kIgnoreCase)) {
			TString stackline;
			while(!stackline.IsDec())
				{ stackline.ReadLine(*input); }
			stack[0] = stackline.Atoi();
			for(int i=1; i<=stack[0]; i++) {
				stackline.Clear();
				while(!stackline.Contains("0x"))
					{ stackline.ReadLine(*input); }
				TString parhex = stackline(TRegexp("[0-9A-Fa-f]+"), stackline.Index("0x")+1);
				TString par = TString::BaseConvert(parhex, 16, 10);
				stack[i] = par.Atoi();
			}
			continue;
		}

		if(line.Contains("buffer", TString::kIgnoreCase) &&
		   line.Contains("len", TString::kIgnoreCase) )     {
			if (line.Contains("single", TString::kIgnoreCase)) {
				bufLen = 1;
				continue;
			}
			TString par = line(number_patt);
			bufLen = par.Atoi();
			continue;
		}

		if(line.Contains("trig", TString::kIgnoreCase) &&
		   line.Contains("delay", TString::kIgnoreCase) )    {
			TString par = line(number_patt);
			trigDelay = par.Atoi();
			continue;
		}

		if(line.Contains("LAM", TString::kIgnoreCase) &&
		   line.Contains("timeout", TString::kIgnoreCase) )     {
			TString par = line(number_patt);
			timeoutLAM = par.Atoi();
			continue;
		}

		if(line.Contains("LAM", TString::kIgnoreCase) &&
		   line.Contains("mask", TString::kIgnoreCase) )     {
			TString par = line(number_patt);
			maskLAM = par.Atoi();
			continue;
		}

		if(line.Contains("bulk", TString::kIgnoreCase) &&
		   line.Contains("buff", TString::kIgnoreCase)  )    {
			TString par = line(number_patt);
			nBufferBulk = par.Atoi();
			continue;
		}

		if(line.Contains("bulk", TString::kIgnoreCase) &&
		   line.Contains("timeout", TString::kIgnoreCase) )     {
			TString par = line(number_patt);
			timeoutBulk = par.Atoi();
			continue;
		}
	}
	return 0;
}



