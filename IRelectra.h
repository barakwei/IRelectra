/*
* IRelectra
* Copyrights 2016 Barak Weiss
*
* Many thanks to Chris from AnalysIR
*/

#pragma once

#ifndef IRelectra_h
#define IRelectra_h

#include "IRremote.h"

#include <stdint.h>
#include <vector>

typedef enum IRElectraMode {
	IRElectraModeCool,
	IRElectraModeHeat,
	IRElectraModeFan,
	IRElectraModeDry,
	IRElectraModeAuto,
} IRElectraMode;

typedef enum IRElectraFan {
	IRElectraFanLow,
	IRElectraFanMedium,
	IRElectraFanHigh,
	IRElectraFanAuto,
} IRElectraFan;

class ElectraRemote;

class IRelectra
{
public:
	IRelectra(IRsend* remote);
	void send(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep);
private:
	IRsend* _remote;
	void sendUsingRemote(ElectraRemote& remote, bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep);
};

#endif
