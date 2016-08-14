/*
 * IRelectra
 * Copyrights 2016 Barak Weiss
 *
 * Many thanks to Chris from AnalysIR
 */

#include "IRelectra.h"
#include <stdint.h>
#include <vector>

using std::vector;

#define UNIT 992
#define NUM_BITS 34

IRelectra::IRelectra(IRsend* remote) : _remote(remote)
{}

// Sends the specified configuration to the IR led
bool IRelectra::sendElectra(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep)
{
    // get the data representing the configuration
    uint64_t code = encodeElectra(power, mode, fan, temperature, swing, sleep);
    
    // get the raw data itself with headers, repetition, etc.
    std::vector<unsigned int> data = generateSignal(code);
    
    // send using HW.
    _remote->sendRaw(data.data(), data.size(), 33);
    
    return true;
}

std::vector<unsigned int> IRelectra::generateSignal(uint64_t code)
{
    MarkSpaceArray markspace(UNIT);

    // The whole packet looks this:
    //  3 Times: 
    //    3000 usec MARK
    //    3000 used SPACE
    //    Maxchester encoding of the data, clock is ~1000usec
    // 4000 usec MARK
    for (int k =0; k<3; k++)
    {
        markspace.addMark(3); //mark
        markspace.addSpace(3); //space
        markspace.addNumberWithManchesterCode(code, NUM_BITS);
    }
    markspace.addMark(4);
    return markspace.data();
}

#pragma pack(1)

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
typedef union ElectraCode {
    uint64_t num;
    struct {
        uint8_t zeros1 : 1;
        uint8_t ones1 : 1;
        uint16_t zeros2 : 16;
        uint8_t sleep : 1;
        uint8_t temperature : 4;
        uint8_t zeros3 : 2;
        uint8_t swing : 1;
        uint8_t zeros4 : 2;
        uint8_t fan : 2;
        uint8_t mode : 3;
        uint8_t power : 1;
    };
} ElectraUnion;

#pragma pack()


uint64_t IRelectra::encodeElectra(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep)
{
    temperature -= 15;
    ElectraCode code = { 0 };
    code.ones1 = 1;
    code.sleep = sleep ? 1 : 0;
    code.temperature = temperature;
    code.swing = swing ? 1 : 0;
    code.fan = fan;
    code.mode = mode;
    code.power = power ? 1 : 0;
    
    return code.num;
}

///
/// Mark Space Array
///
MarkSpaceArray::MarkSpaceArray(uint16_t unitLengthInUsec) : _unitLength(unitLengthInUsec)
{ }
    
void MarkSpaceArray::MarkSpaceArray::addMark(uint16_t units)
{
    if (currentState())
    {
        addUnitsToCurrentState(units);
    }
    else
    {
        addUnitsToNextState(units);
    }
}

void MarkSpaceArray::addSpace(uint16_t units)
{
    if (!currentState())
    {
        addUnitsToCurrentState(units);
    }
    else
    {
        addUnitsToNextState(units);
    }}

void MarkSpaceArray::addBitWithManchesterCode(uint8_t bit)
{
    if (currentState() == (bit & 1))
    {
        addUnitsToNextState(1);
    }
    else
    {
        addUnitsToCurrentState(1);
    }
    addUnitsToNextState(1);
}

void MarkSpaceArray::addNumberWithManchesterCode(uint64_t code, uint8_t numberOfBits)
{
   for (int j = numberOfBits - 1; j>=0; j--)
   {
       addBitWithManchesterCode((code >> j) & 1);
   }
}

void MarkSpaceArray::addUnitsToCurrentState(uint16_t units)
{
    _data.back() += _unitLength * units;
}
    
void MarkSpaceArray::addUnitsToNextState(uint16_t units)
{
    _data.emplace_back(_unitLength * units);
}

const std::vector<unsigned int> MarkSpaceArray::data()
{
    return _data;
}

uint8_t MarkSpaceArray::currentState()
{
    return _data.size() % 2;
}
