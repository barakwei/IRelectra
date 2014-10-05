/*
* IRelectra
* Version 0.8
* Copyrights 2014 Barak Weiss
*
* Many thanks to Chris from AnalysIR
*/

#ifndef IRelectra_h
#define IRelectra_h

#include <stdint.h>

#include "IRremote.h"

#define POWER_OFF 0
#define POWER_ON  1

#define MODE_COOL 0b001
#define MODE_HEAT 0b010
#define MODE_AUTO 0b011
#define MODE_DRY  0b100
#define MODE_FAN  0b101

#define FAN_LOW   0b00
#define FAN_MED   0b01
#define FAN_HIGH  0b10
#define FAN_AUTO  0b11

#define SWING_OFF 0b0
#define SWING_ON  0b1

#define SLEEP_OFF 0b0
#define SLEEP_ON  0b1

class IRelectra
{
public:
    // Ctor, remote will be used to send the raw IR data
    IRelectra(IRsend* remote);
    
    // Sends the specified configuration to the IR led using IRremote
    bool SendElectra(int power, int mode, int fan, int temperature, int swing, int sleep);

private:
    IRsend* _remote;
    uint64_t EncodeElectra(int power, int mode, int fan, int temperature, int swing, int sleep);
    void addBit(unsigned int* p, int* i, char b);
};

#endif