/*
 * IRelectra
 * Copyrights 2016 Barak Weiss
 *
 * Many thanks to Chris from AnalysIR
 */

#include "IRelectra.h"
#include "application.h"

#include <initializer_list>
#include <map>

// A sequence of marks (high) and spaces (low) in microseconds. The first element is mark.
class MarkSpaceArray
{
public:

	// Empty array.
	MarkSpaceArray()
	{}

	// Array with values.
	MarkSpaceArray(std::initializer_list<unsigned int> iList) : _data(iList)
	{}

	MarkSpaceArray& operator=(std::initializer_list<unsigned int> iList)
	{	
		_data.assign(iList);
		return (*this);
	}

	// Add a number of usecs of mark to the end of the array.
	void addMark(uint32_t usec)
	{
		if (currentState())
		{
			addTimeToCurrentState(usec);
		}
		else
		{
			addTimeToNextState(usec);
		}
	}

	// Add a number of usecs of space to the end of the array.
	void addSpace(uint32_t usec)
	{
		if (!currentState())
		{
			addTimeToCurrentState(usec);
		}
		else
		{
			addTimeToNextState(usec);
		}
	}

	// Adds the data of the entire given array to the end of the array.
	void addArray(const MarkSpaceArray& array)
	{
		const auto& data = array.data();
		uint32_t i = 0;
		// Special case when the first mark is zero.
		if (data.size() > 0 && data[0] == 0)
		{
			++i;
		}
		for (; i < data.size(); ++i)
		{
			if (i % 2 == 0)
			{
				addMark(data[i]);
			}
			else
			{
				addSpace(data[i]);
			}
		}
	}

	// Array containing timing for marks and spaces, starts with marks.
	const std::vector<unsigned int> data() const
	{
		return _data;
	}

	unsigned int* rawData()
	{
		return _data.data();
	}

	unsigned int size() const
	{
		return _data.size();
	}

private:

	std::vector<unsigned int> _data;

	// Add more usec to the current state. For example, if the array
	// looks like this: { 1000, 1000 } (equal to calling addMark(1000)
	// followed by addSpace(1000), the current state is SPACE (currentState()==0).
	// then calling this function with (1000) will change the array to { 1000, 2000 }.
	void addTimeToCurrentState(uint32_t usec)
	{
		if (size() == 0)
		{ 
			_data.emplace_back(0);
			_data.emplace_back(usec);
		}
		else
		{
		_data.back() += usec;
		}
	}

	// Add more time to the other state. For example, if the array
	// looks like this: { 1000, 1000 } (equal to calling addMark(1)
	// followed by addSpace(1), the current state is SPACE (currentState()==0).
	// Then calling this function with (1000) will change the array to { 1000, 1000, 1000 }
	void addTimeToNextState(uint32_t usec)
	{
		_data.emplace_back(usec);
	}

	// Returns 1 for mark, 0 for space.
	uint8_t currentState()
	{
		return _data.size() % 2;
	}
};

// Base class for encoding values (numbers or buffers), any class that derives from this class
// should implement addZero() and addOne() and by adding values to the _data array. 
class IREncoder
{
public:
	// Array containing the entire encoded value.
	const MarkSpaceArray data()
	{
		return _data;
	}

	// Encode zero and add to the end.
	virtual void addZero() = 0;

	// Encode one and add to the end.
	virtual void addOne() = 0;

	// Encode bit and add to the end. 
	virtual void addBit(uint8_t bit)
	{
		bit ? addOne() : addZero();
	}

	// Encode a given number of bits from the given number to the end (MSB first).
	virtual void addNumber(uint64_t n, uint8_t numberOfBits)
	{
		for (int j = numberOfBits - 1; j >= 0; j--)
		{
			addBit((n >> j) & 1);
		}
	}

	// Encode a given number of bits from the given buffer to the end.
	virtual void addBuffer(uint8_t* buffer, uint8_t numberOfBits)
	{
		for (int j = 0; j < numberOfBits; ++j)
		{
			uint8_t bit = buffer[j / 8];
			bit >>= 7 - (j % 8);
			bit &= 1;
			addBit(bit);
		}
	}

protected:
	MarkSpaceArray _data;
};

// IEEE 802.3 Manchester encoder.
// 0 is encoded as one unit MARK and one unit SPACE.
// 1 is encoded as one unit SPACE and one unit MARK.
class ManchesterIREncoder : public IREncoder
{
public:
	// Initialize the encoder with a specific unit length. This is the clock used
	// in the Manchester encoding. Each bit will be encoder as two units.
	ManchesterIREncoder(uint16_t unitLengthInUsec) : _unitLength(unitLengthInUsec)
	{}

	virtual void addZero()
	{
		_data.addMark(_unitLength);
		_data.addSpace(_unitLength);
	}

	virtual void addOne()
	{
		_data.addSpace(_unitLength);
		_data.addMark(_unitLength);
	}

private:
	uint16_t _unitLength;
};

// Pulse distance encoder.
// 0 is encoded as one unit mark and one unit space.
// 1 is encoded as one unit mark and three units space.
class PulseDistanceIREncoder : public IREncoder
{
public:
	// Initialize the array with a specific unit length. 
	PulseDistanceIREncoder(uint16_t unitLengthInUsec) : _unitLength(unitLengthInUsec)
	{}

	virtual void addZero()
	{
		_data.addMark(_unitLength);
		_data.addSpace(_unitLength);
	}

	virtual void addOne()
	{
		_data.addMark(_unitLength);
		_data.addSpace(3 * _unitLength);
	}
	
private:
	uint16_t _unitLength;
};

// Base A/C IR remote.
class ElectraRemote
{
protected:
	virtual MarkSpaceArray packetHeader() = 0;
	virtual MarkSpaceArray packetTail() = 0;
	virtual MarkSpaceArray codeHeader() = 0;
	virtual MarkSpaceArray codeTail() = 0;
	virtual uint8_t codeRepetitions() = 0;
	virtual MarkSpaceArray code(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep) = 0;
public:
	// In Khz
	virtual uint8_t modulationFrequency() = 0;
	MarkSpaceArray fullPacket(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep)
	{
		MarkSpaceArray packet;
		MarkSpaceArray codeArr = code(power, mode, fan, temperature, swing, sleep);
		packet.addArray(packetHeader());
		for (int i = 0; i < codeRepetitions(); ++i)
		{
			packet.addArray(codeHeader());
			packet.addArray(codeArr);
			packet.addArray(codeTail());
		}
		packet.addArray(packetTail());
		return packet;
	}
};

class OrangeElectraRemote : public ElectraRemote
{
	static const unsigned int baseUnitTime = 992;
	static const uint8_t numBits = 34;

	std::map<IRElectraMode, uint8_t> _remoteModes = {
		{ IRElectraModeCool, 0b001 },
		{ IRElectraModeHeat, 0b010 },
		{ IRElectraModeFan, 0b101 },
		{ IRElectraModeDry, 0b100 },
		{ IRElectraModeAuto, 0b011 }
	};

	std::map<IRElectraFan, uint8_t> _remoteFan = {
		{ IRElectraFanLow, 0b00 },
		{ IRElectraFanMedium, 0b01 },
		{ IRElectraFanHigh, 0b10 },
		{ IRElectraFanAuto, 0b11 }
	};
	
#pragma pack(1)
	typedef union OrangeElectraCode {
		uint64_t num;
		struct {
			uint64_t zeros1 : 1;
			uint64_t ones1 : 1;
			uint64_t zeros2 : 16;
			uint64_t sleep : 1;
			uint64_t temperature : 4;
			uint64_t zeros3 : 2;
			uint64_t swing : 1;
			uint64_t zeros4 : 2;
			uint64_t fan : 2;
			uint64_t mode : 3;
			uint64_t power : 1;
		};
	} ElectraUnion;
#pragma pack()

	MarkSpaceArray packetHeader()
	{
		return{};
	}

	MarkSpaceArray packetTail()
	{
		return{ baseUnitTime * 4 };
	}

	MarkSpaceArray codeHeader()
	{
		return{ baseUnitTime * 3, baseUnitTime * 3 };
	}

	MarkSpaceArray codeTail()
	{
		return{};
	}

	uint8_t codeRepetitions()
	{
		return 3;
	}

public:
	uint8_t modulationFrequency()
	{
		return 33;
	}

	MarkSpaceArray code(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep)
	{
		temperature -= 15; 
		OrangeElectraCode code = { 0 };
		code.ones1 = 1;
		code.sleep = sleep ? 1 : 0;
		code.temperature = temperature;
		code.swing = swing ? 1 : 0;
		code.fan = _remoteFan[fan];
		code.mode = _remoteModes[mode];
		code.power = power ? 1 : 0;

		ManchesterIREncoder encoder(baseUnitTime);
		encoder.addNumber(code.num, numBits);
		return encoder.data();
	}
};

class GreenElectraRemote : public ElectraRemote
{
	static const unsigned int baseUnitTime = 560;
	static const uint8_t numBits = 104;

	std::map<IRElectraMode, uint8_t> _remoteModes = {
		{ IRElectraModeCool, 0b001 },
		{ IRElectraModeHeat, 0b100 },
		{ IRElectraModeFan, 0b110 },
		{ IRElectraModeDry, 0b010 },
		{ IRElectraModeAuto, 0b000 }
	};

	std::map<IRElectraFan, uint8_t> _remoteFan = {
		{ IRElectraFanLow, 0b011 },
		{ IRElectraFanMedium, 0b010 },
		{ IRElectraFanHigh, 0b001 },
		{ IRElectraFanAuto, 0b101 },
	};

#pragma pack(1)
	typedef union GreenElectraCode {
		uint8_t buffer[13];
		struct {
			uint8_t c3;
			union {
				uint8_t temperatureByte;
				struct {
					uint8_t notVerticalSwing : 3;
					uint8_t temperatureMod8 : 3;
					uint8_t temperatureDiv8Minus1 : 2;
				};
			};
			union {
				uint8_t hourByte;
				struct {
					uint8_t hour : 5;
					uint8_t notHorizontalSwing : 3;
				};
			};
			uint8_t minute;
			union {
				uint8_t fanByte;
				struct {
					uint8_t fanZeros : 5;
					uint8_t fan : 3;
				};
			};
			uint8_t zeros1;
			union {
				uint8_t modeByte;
				struct {
					uint8_t modeZeros1 : 2;
					uint8_t sleep : 1;
					uint8_t modeZeros2 : 2;
					uint8_t mode : 3;
				};
			};
			uint8_t zeros2;
			uint8_t zeros3;
			uint8_t power;
			uint8_t zeros4;
			uint8_t button;
			uint8_t sum;
		};
	} ElectraUnion;
#pragma pack()

	MarkSpaceArray packetHeader()
	{
		return{9000, 4500};
	}

	MarkSpaceArray packetTail()
	{
		return{ baseUnitTime };
	}

	MarkSpaceArray codeHeader()
	{
		return{};
	}

	MarkSpaceArray codeTail()
	{
		return{};
	}

	uint8_t codeRepetitions()
	{
		return 1;
	}

public:
	uint8_t modulationFrequency()
	{
		return 38;
	}

	uint8_t reverseByte(uint8_t b)
	{
		uint8_t newByte = 0;
		for (int i = 0; i < 8; ++i)
		{
			newByte = newByte << 1;
			newByte |= (b >> i) & 1;
		}
		return newByte;
	}

	MarkSpaceArray code(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep)
	{
		GreenElectraCode code = { 0 };
		code.c3 = 0xC3;
		code.notVerticalSwing = !swing ? 0b111 : 0;
		code.temperatureDiv8Minus1 = (temperature / 8) - 1;
		code.temperatureMod8 = temperature & 8;
		code.notHorizontalSwing = 0b111;
		code.minute = 0x0;
		code.fan = _remoteFan[fan];
		code.mode = _remoteModes[mode];
		code.sleep = sleep;
		code.power = power ? 0x20 : 0;
		code.button = 0b101;

		int sum = 0;
		for (unsigned int i = 0; i < sizeof(code.buffer); ++i)
		{
			sum += code.buffer[i];
			code.buffer[i] = reverseByte(code.buffer[i]);
		}
		code.sum = reverseByte(sum & 0xFF);
		PulseDistanceIREncoder encoder(baseUnitTime);
		encoder.addBuffer(code.buffer, numBits);
		return encoder.data();
	}
};

IRelectra::IRelectra(IRsend* remote) : _remote(remote)
{}

void IRelectra::sendUsingRemote(ElectraRemote & remote, bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep)
{
	MarkSpaceArray packet = remote.fullPacket(power, mode, fan, temperature, swing, sleep);
	_remote->sendRaw(packet.rawData(), packet.size(), remote.modulationFrequency());
}

void IRelectra::send(bool power, IRElectraMode mode, IRElectraFan fan, int temperature, bool swing, bool sleep)
{
	OrangeElectraRemote orange;
	GreenElectraRemote green;
	sendUsingRemote(orange, power, mode, fan, temperature, swing, sleep);
	delay(2000);
	sendUsingRemote(green, power, mode, fan, temperature, swing, sleep);
}
