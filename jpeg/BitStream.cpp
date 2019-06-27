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


void StringBitStream::Add(const std::string& bits)
{
	m_bits.append(bits);
	m_bits.append(" ");
}


void StringBitStream::Write(std::ostream& out)
{
	out << m_bits;
}


std::string StringBitStream::Pop()
{
	size_t start_0 = m_bits.find_first_of('0');
	size_t start_1 = m_bits.find_first_of('1');
	size_t start = start_0 < start_1 ? start_0 : start_1;
	if (start == string::npos) return "";
	size_t end = m_bits.find_first_of(' ', start);
	string buffer(m_bits.begin() + start, m_bits.begin() + end);
	m_bits.erase(m_bits.begin(), m_bits.begin() + end);
	return buffer;
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


void BinaryBitStream::Add(const std::string& bits)
{}


void BinaryBitStream::Write(std::ostream & out)
{}


std::string BinaryBitStream::Pop()
{
	return std::string();
}


void BinaryBitStream::Read(std::istream & in)
{}
