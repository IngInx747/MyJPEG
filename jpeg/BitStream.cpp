#include "BitStream.h"

#include <streambuf>

using namespace std;


BitStream::BitStream()
{}


BitStream::~BitStream()
{}

/**
size_t BitStream::size()
{
	return size_t();
}


void BitStream::Add()
{}


void BitStream::Write(std::ostream & out)
{}


std::string BitStream::Pop()
{
	return std::string();
}


void BitStream::Read(std::istream & in)
{}
*/




StringBitStream::StringBitStream()
{}


StringBitStream::~StringBitStream()
{}


size_t StringBitStream::size()
{
	return m_bits.size();
}


bool StringBitStream::empty()
{
	return m_bits.empty();
}


void StringBitStream::Add(const std::string& bits)
{
	m_bits.append(bits);
	//m_bits.append(" ");
}


void StringBitStream::Write(std::ostream& out)
{
	out << m_bits;
}


int StringBitStream::Pop()
{
	while (!empty() && m_bits[0] != '0' && m_bits[0] != '1')
		m_bits.erase(m_bits.begin());

	if (empty())
		return -1;

	int bit = m_bits[0] - '0';
	m_bits.erase(m_bits.begin());

	return bit;
}


void StringBitStream::Read(std::istream& in)
{
	m_bits.clear();
	m_bits.assign(istreambuf_iterator<char>(in), istreambuf_iterator<char>());
}





BinaryBitStream::BinaryBitStream()
{}


BinaryBitStream::~BinaryBitStream()
{}


size_t BinaryBitStream::size()
{
	return size_t();
}


bool BinaryBitStream::empty()
{
	return false;
}


void BinaryBitStream::Add(const std::string& bits)
{}


void BinaryBitStream::Write(std::ostream & out)
{}


int BinaryBitStream::Pop()
{
	return -1;
}


void BinaryBitStream::Read(std::istream & in)
{}
