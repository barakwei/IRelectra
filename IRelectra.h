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