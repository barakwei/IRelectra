/*
 * IRelectra
 * Version 0.8 
 * Copyrights 2014 Barak Weiss
 *
 * Many thanks to Chris from AnalysIR
 */

#include "IRelectra.h"
#include <stdint.h>

#define UNIT 1000
#define NUM_BITS 34

IRelectra::IRelectra(IRsend* remote) : _remote(remote)
{}

// Add bit b to array p at index i
// This function is used to convert manchester encoding to MARKs and SPACEs
// p is the pointer to the start of the MARK, SPACE array
// i is the current index
// b is the bit we want to add to the MARK, SPACE array
// A zero bit is one unit MARK and one unit SPACE
// a one bit is one unit SPACE and one unit MARK
void IRelectra::addBit(unsigned int* p, int* i, char b)
{
    if (((*i) & 1) == 1)
    {
        // current index is SPACE
        if ((b & 1) == 1)
        {
            // one is one unit low, then one unit up
            // since we're pointing at SPACE, we should increase it byte a unit
            // then add another MARK unit
            *(p+*i) += UNIT;
            (*i)++;
            *(p+*i) = UNIT;
        }
        if ((b & 1) == 0)
        {
            // we need a MARK unit, then SPACE unit
            (*i)++;
            *(p+*i) = UNIT;
            (*i)++;
            *(p+*i) = UNIT;
        }
    }
    else if (((*i) & 1) == 0)
    {
        // current index is MARK
        if ((b & 1) == 1)
        {
            (*i)++;
            *(p+*i) = UNIT;
            (*i)++;
            *(p+*i) = UNIT;
        }
        if ((b & 1) == 0)
        {
            *(p+*i) += UNIT;
            (*i)++;
            *(p+*i) = UNIT;
        }
    }

}

// Sends the specified configuration to the IR led using IRremote
// 1. Get the numeric value of the configuration
// 2. Convert to IRremote compatible array (MARKS and SPACES)
// 3. Send to IRremote
bool IRelectra::SendElectra(int power, int mode, int fan, int temperature, int swing, int sleep)
{
    unsigned int data[200]; //~maximum size of the IR packet
    int i = 0;
 
    // get the data representing the configuration
    uint64_t code = EncodeElectra(power, mode, fan, temperature, swing, sleep);
    
    // The whole packet looks this:
    //  3 Times: 
    //    3000 usec MARK
    //    3000 used SPACE
    //    Maxchester encoding of the data, clock is ~1000usec
    // 4000 usec MARK
    for (int k = 0; k<3; k++)
    {
        data[i] = 3 * UNIT; //mark
        i++;
        data[i] = 3 * UNIT;
        for (int j = NUM_BITS - 1; j >= 0; j--)
        {
            addBit(data, &i, (code >> j) & 1);
        }
        i++;
    }
    data[i] = 4 * UNIT;

    _remote->sendRaw(data, i + 1, 38);
    return true;
}

// Encodes specific A/C configuration to a number that describes
// That configuration has a total of 34 bits
//    33: Power bit, if this bit is ON, the A/C will toggle it's power.
// 32-30: Mode - Cool, heat etc.
// 29-28: Fan - Low, medium etc.
// 27-26: Zeros
//    25: Swing On/Off
// 24-23: Zeros
// 22-19: Temperature, where 15 is 0000, 30 is 1111
//    18: Sleep mode On/Off
// 17- 2: Zeros
//     1: One
//     0: Zero
uint64_t IRelectra::EncodeElectra(int power, int mode, int fan, int temperature, int swing, int sleep)
{
    uint64_t num = 0;
    uint64_t power64 = power;
    uint64_t mode64 = mode;
    
    num |= ((power64 & 1) << 33);
    
    num |= ((mode64 & 7) << 30);
    
    num |= ((fan & 3) << 28);

    num |= ((swing & 1) << 25);

    temperature -= 15;
    num |= ((temperature & 15) << 19);
    
    num |= ((sleep & 1) << 18);
    
    num |= 2;
    
    return num;
}
