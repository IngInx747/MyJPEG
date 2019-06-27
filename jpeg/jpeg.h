#ifndef JPEG_H
#define JPEG_H

#ifdef UNIT_TEST_FLAG
#include "jpeg.cpp"
#endif

#include <vector>
#include <string>
#include <memory>

#include "BitStream.h"

namespace jpeg
{
namespace util // 
{
void RGB2YCC(std::vector<float>& block, size_t block_id);
void YCC2RGB(std::vector<float>& block, size_t block_id);
void UnionChannels(std::vector<float>& block, size_t block_id);
void ScatterChannels(std::vector<float>& block, size_t block_id);

void Quantize(std::vector<float>& block, size_t block_id, size_t channel, float quality);
void Dequantize(std::vector<float>& block, size_t block_id, size_t channel, float quality);

void Zigzag(std::vector<float>& block, size_t block_id, size_t channel);
void Unzigzag(std::vector<float>& block, size_t block_id, size_t channel);
}
}



namespace jpeg
{
namespace dct // decrete cosine transform
{
void ForwardTransform8x8(std::vector<float>& block, size_t block_id, size_t channel);
void InverseTransform8x8(std::vector<float>& block, size_t block_id, size_t channel);
}
}



namespace jpeg
{
namespace huffman_coding
{
template <typename T>
struct node
{
	node(
		int id_,
		T freq_) : id(id_), freq(freq_), left(-1), right(-1)
	{}

	node(
		int id_,
		T freq_,
		int left_,
		int right_) : id(id_), freq(freq_), left(left_), right(right_)
	{}

	int id; // color
	T freq; // requence
	int left, right;
};

template <typename T>
void Encode(
	const std::vector<T>& frequencies,
	std::vector<std::string>& codes);

template <typename T>
bool Decode(
	const std::vector<bool>& data,
	const std::vector<T>& signals_collection,
	const std::vector<std::string>& codes,
	std::vector<T>& signals_output);
}
}



namespace jpeg
{
namespace huffman_coding
{
void Encode_DC(int val, BitStream*);
void Encode_AC(int run, int val, BitStream*);
void EncodeBlock(const std::vector<float>&, size_t, size_t, int&, BitStream*);

int Decode_DC(BitStream*);
std::pair<int, int> Decode_AC(BitStream*);
void DecodeBlock(std::vector<float>&, size_t, size_t, int&, BitStream*);
}
}
#endif // !JPEG_H
