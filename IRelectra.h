/*
* IRelectra
* Copyrights 2016 Barak Weiss
*
* Many thanks to Chris from AnalysIR
*/

#ifndef IRelectra_h
#define IRelectra_h

#include "IRremote.h"

#include <stdint.h>
#include <vector>

typedef enum IRElectraMode {
    IRElectraModeCool = 0b001,
    IRElectraModeHeat = 0b010,
    IRElectraModeAuto = 0b011,
    IRElectraModeDry  = 0b100,
    IRElectraModeFan  = 0b101
} IRElectraMode;

typedef enum IRElectraFan {
    IRElectraFanLow    = 0b00,
    IRElectraFanMedium = 0b01,
    IRElectraFanHigh   = 0b10,
    IRElectraFanAuto   = 0b11
} IRElectraFan;

class IRelectra
{
public:
    // Initialize
    IRelectra(IRsend* remote);
    
    // Send an IR packet with the given parameters.
    bool sendElectra(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep);

private:
    IRsend* _remote;
    
    // Encodes specific A/C configuration to a number that describes
    uint64_t encodeElectra(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep);
    
    // Create the entire MARK-SPACE array containing the entire packet to send to the A/C
    std::vector<unsigned int> generateSignal(uint64_t code);
};

// Class to create MARK-SPACE array. An IR code is a digital signal, which
// means it's made out of 0's (space) and 1's (mark). This class helps
// create these kinds of signals. It has the ability to add marks and spaces
// at any time, and to add single bit using Manchester code to the signal.
// Once you added enough data to the array use the data() methods to get 
// the raw data. Make sure that the first think you add to the array is
// at least one mark.
class MarkSpaceArray
{
public:
    // Initialize the array with a specific unit length. This is the clock used
    // in the Manchester code.
    MarkSpaceArray(uint16_t unitLengthInUsec);

    // Add a number of time units with mark.
    void addMark(uint16_t units);

    // Add a number of time units with space.
    void addSpace(uint16_t units);
    
    // Encodes the bit with IEEE 802.3 Manchester coding and adds it to the array
    // A zero bit is one unit MARK and one unit SPACE
    // a one bit is one unit SPACE and one unit MARK
    void addBitWithManchesterCode(uint8_t bit);
    
    // Encodes a given number of bits from the given number bit by bit with 
    // IEEE 802.3 Manchester coding and adds it to the array. MSB first.
    void addNumberWithManchesterCode(uint64_t bit, uint8_t numberOfBits);

    // Array containing timing for marks and spaces, starts with marks.
    const std::vector<unsigned int> data();

private:
    // Add more time units to the current state. For example, if the array
    // looks like this: { 1*UNIT, 1*UNIT } (equal to calling addMark(1)
    // followed by addSpace(1), the current state is SPACE (currentState()==0)
    // calling this function will change the array to { 1*UNIT, 2*UNIT }.
    void addUnitsToCurrentState(uint16_t units);
    
    // Add more time to the other state. For example, if the array
    // looks like this: { 1*UNIT, 1*UNIT } (equal to calling addMark(1)
    // followed by addSpace(1), the current state is SPACE (currentState()==0)
    // calling this function will change the array to { 1*UNIT, 1*UNIT, 1*UNIT }
    void addUnitsToNextState(uint16_t units);
    
    // Returns 1 for mark, 0 for space.
    uint8_t currentState();
    
    uint16_t _unitLength;
    std::vector<unsigned int> _data;
};

#endif