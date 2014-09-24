#include "IRelectra.h"
#include <stdint.h>

#define UNIT 1000
#define NUM_BITS 34

IRelectra::IRelectra(IRsend* remote) : _remote(remote)
{}

// Add bit b to array p at index i
// 
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
            *(p + *i) += UNIT;
            (*i)++;
            *(p + *i) = UNIT;
        }
        if ((b & 1) == 0)
        {
            // we need a MARK unit, then SPACE unit
            (*i)++;
            *(p + *i) = UNIT;
            (*i)++;
            *(p + *i) = UNIT;
        }
    }
    else if (((*i) & 1) == 0)
    {
        // current index is MARK
        if ((b & 1) == 1)
        {
            (*i)++;
            *(p + *i) = UNIT;
            (*i)++;
            *(p + *i) = UNIT;
        }
        if ((b & 1) == 0)
        {
            *(p + *i) += UNIT;
            (*i)++;
            *(p + *i) = UNIT;
        }
    }

}

bool IRelectra::SendElectra(int power, int mode, int fan, int temperature, int swing, int sleep)
{
    unsigned int data[200];
    int i = 0;

    // get the data representing the IR packet
    uint64_t code = EncodeElectra(power, mode, fan, temperature, swing, sleep);

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

uint64_t IRelectra::EncodeElectra(int power, int mode, int fan, int temperature, int swing, int sleep)
{
    uint64_t num = 0;
    uint64_t power64 = power;
    uint64_t mode64 = mode;

    num |= ((power64 & 1) << 33);

    num |= ((mode64 & 7) << 30);

    num |= ((fan & 3) << 28);

    temperature -= 15;
    num |= ((temperature & 15) << 19);

    num |= ((swing & 1) << 25);

    num |= ((sleep & 1) << 18);

    num |= 2;

    return num;
}
