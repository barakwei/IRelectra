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

typedef enum IRElectraRemote {
	IRElectraRemoteOrange,
	IRElectraRemoteGreen,
} IRElectraRemote;

// A sequence of marks (high) and spaces (low) in microseconds. The first element is mark.
class MarkSpaceArray
{
public:
	MarkSpaceArray();

	// Array with values.
	MarkSpaceArray(std::initializer_list<unsigned int> iList);

	MarkSpaceArray& operator=(std::initializer_list<unsigned int> iList);

	// Add a number of usecs of mark to the end of the array.
	void addMark(uint32_t usec);

	// Add a number of usecs of space to the end of the array.
	void addSpace(uint32_t usec);

	// Adds the data of the entire given array to the end of the array.
	void addArray(const MarkSpaceArray& array);

	// Array containing timing for marks and spaces, starts with marks.
	const std::vector<unsigned int> data() const;

	unsigned int* rawData();

	unsigned int size() const;

private:

	std::vector<unsigned int> _data;

	// Add more usec to the current state. For example, if the array
	// looks like this: { 1000, 1000 } (equal to calling addMark(1000)
	// followed by addSpace(1000), the current state is SPACE (currentState()==0).
	// then calling this function with (1000) will change the array to { 1000, 2000 }.
	void addTimeToCurrentState(uint32_t usec);

	// Add more time to the other state. For example, if the array
	// looks like this: { 1000, 1000 } (equal to calling addMark(1)
	// followed by addSpace(1), the current state is SPACE (currentState()==0).
	// Then calling this function with (1000) will change the array to { 1000, 1000, 1000 }
	void addTimeToNextState(uint32_t usec);

	// Returns 1 for mark, 0 for space.
	uint8_t currentState() const;
};

// Base A/C IR remote.
class ElectraRemote
{
protected:
	virtual MarkSpaceArray packetHeader() const = 0;
	virtual MarkSpaceArray packetTail() const = 0;
	virtual MarkSpaceArray codeHeader() const = 0;
	virtual MarkSpaceArray codeTail() const = 0;
    virtual uint8_t codeRepetitions() const = 0;
    virtual MarkSpaceArray code(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep) const = 0;
public:
	// In Khz
	virtual uint8_t modulationFrequency() const = 0;
	MarkSpaceArray fullPacket(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep) const;
    virtual const char* name() = 0;
};

class OrangeElectraRemote : public ElectraRemote
{
private:
	static const unsigned int baseUnitTime = 992;
	static const uint8_t numBits = 34;
	MarkSpaceArray packetHeader() const override;
	MarkSpaceArray packetTail() const override;
	MarkSpaceArray codeHeader() const override;
	MarkSpaceArray codeTail() const override;
	uint8_t codeRepetitions() const override;
public:
	uint8_t modulationFrequency() const override;
	MarkSpaceArray code(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep) const override;
    virtual const char* name();
};

class GreenElectraRemote : public ElectraRemote
{
private:
	static const unsigned int baseUnitTime = 560;
	static const uint8_t numBits = 104;
	MarkSpaceArray packetHeader() const override;
	MarkSpaceArray packetTail() const override;
	MarkSpaceArray codeHeader() const override;
	MarkSpaceArray codeTail() const override;
	uint8_t codeRepetitions() const override;
	uint8_t reverseByte(uint8_t b)  const;
public:
	uint8_t modulationFrequency() const override;
	MarkSpaceArray code(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep) const override;
    virtual const char* name();
};

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
