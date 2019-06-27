#ifndef BYTE_MANAGER_H
#define BYTE_MANAGER_H

#include <iostream>
#include <vector>
#include <bitset>
#include <string>
#include <memory>

constexpr long N_BIT = 0x10;;
using Bit = std::bitset<0x10>;


// An interface for flexible bit data operations
class BitStream
{
public:
	BitStream() {}

	virtual size_t size() = 0;

	////////////////////////////////////////
	// Encode data to bits container
	////////////////////////////////////////

	// Push data to container
	// How bits are formatted is defined here
	virtual void Add(const std::string& bits) = 0;
	// Write data to stream
	// How bits are written to stream is defined here
	virtual void Write(std::ostream& out) = 0;

	////////////////////////////////////////
	// Decode data from bits container
	////////////////////////////////////////

	// Pop data from container
	// How bits are interpreted and trimmed is defined here
	virtual std::string Pop() = 0;
	// Read stream
	// How bits are read from stream is defined here
	virtual void Read(std::istream& in) = 0;

private:
};


// For rapid-dev and debug
class StringBitStream : public BitStream
{
public:
	StringBitStream();

	virtual size_t size();

	virtual void Add(const std::string& bits);
	virtual void Write(std::ostream& out);

	virtual std::string Pop();
	virtual void Read(std::istream& in);

private:
	std::string m_bits;
};


// For real binary file format
class BinaryBitStream : public BitStream
{
public:
	BinaryBitStream();

	virtual size_t size();

	virtual void Add(const std::string& bits);
	virtual void Write(std::ostream& out);

	virtual std::string Pop();
	virtual void Read(std::istream& in);

private:
};

#endif // !BYTE_MANAGER_H
