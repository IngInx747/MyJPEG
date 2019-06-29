#include "jpeg.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <functional>

#include <queue>
#include <map>
#include <unordered_map>

namespace jpeg
{
namespace dct // decrete cosine transform
{
using std::vector;
using std::string;
using std::cos;

constexpr float pi() { return  3.14159265358979323846; }
constexpr float sqrt1_2() { return 0.707106781186547524401; }


// Discrete Cosine Transform Coefficiencies
// r(x,y,u,v) = r(x,u) * r(y,v)
//            = cos((2x+1)u * pi/16) * cos((2y+1)v * pi/16)
// where r(x,u) = cos((2x+1)u * pi/16)
const vector<float> dct_mat8x8([]() {
	const size_t n = 64;
	const size_t w = 8, h = 8;
	vector<float> ret(n);
	for (size_t x = 0; x < h; ++x)
		for (size_t u = 0; u < w; ++u)
			ret[u * w + x] = .5f * cos((2 * x + 1) * u * pi() / 16.f) * (u == 0 ? sqrt1_2() : 1);
	return ret;
}());


//
//
void ForwardTransform8x8(vector<float>& block, size_t block_id, size_t channel)
{
	size_t offset = block_id * 256 + channel * 64;
	auto iter = block.begin() + offset;
	vector<float> copy(iter, iter + 64), temp(64, 0.f);

	for (size_t e = 0; e < 64; ++e)
		copy[e] -= 128.f;

	for (size_t i = 0; i < 8; ++i)
		for (size_t j = 0; j < 8; ++j)
			for (size_t k = 0; k < 8; ++k)
				temp[i * 8 + j] += dct_mat8x8[i * 8 + k] * copy[k * 8 + j];

	for (size_t e = 0; e < 64; ++e)
		copy[e] = 0.f;

	for (size_t i = 0; i < 8; ++i)
		for (size_t j = 0; j < 8; ++j)
			for (size_t k = 0; k < 8; ++k)
				copy[i * 8 + j] += temp[i * 8 + k] * dct_mat8x8[j * 8 + k];

	for (size_t e = 0; e < 64; ++e)
		block[offset + e] = copy[e];
}


//
//
void InverseTransform8x8(vector<float>& block, size_t block_id, size_t channel)
{
	size_t offset = block_id * 256 + channel * 64;
	auto iter = block.begin() + offset;
	vector<float> copy(iter, iter + 64), temp(64, 0.f);

	for (size_t i = 0; i < 8; ++i)
		for (size_t j = 0; j < 8; ++j)
			for (size_t k = 0; k < 8; ++k)
				temp[i * 8 + j] += dct_mat8x8[k * 8 + i] * copy[k * 8 + j];

	for (size_t e = 0; e < 64; ++e)
		copy[e] = 0.f;

	for (size_t i = 0; i < 8; ++i)
		for (size_t j = 0; j < 8; ++j)
			for (size_t k = 0; k < 8; ++k)
				copy[i * 8 + j] += temp[i * 8 + k] * dct_mat8x8[k * 8 + j];

	for (size_t e = 0; e < 64; ++e)
		block[offset + e] = copy[e] + 128.f;
}
}
}





namespace jpeg
{
namespace util
{
using std::vector;
using std::string;

const vector<float> quant_mat8x8_jpeg2000{
	 16,  11,  10,  16,  24,  40,  51,  61,
	 12,  12,  14,  19,  26,  58,  60,  55,
	 14,  13,  16,  24,  40,  57,  69,  56,
	 14,  17,  22,  29,  51,  87,  80,  62,
	 18,  22,  37,  56,  68, 109, 103,  77,
	 24,  35,  55,  64,  81, 104, 113,  92,
	 49,  64,  78,  87, 103, 121, 120,  101,
	 72,  92,  95,  98, 112, 100, 103,  99,
};

const vector<int> zigzag_mat8x8{
	0,
	1,8,
	16,9,2,
	3,10,17,24,
	32,25,18,11,4,
	5,12,19,26,33,40,
	48,41,34,27,20,13,6,
	7,14,21,28,35,42,49,56,
	57,50,43,36,29,22,15,
	23,30,37,44,51,58,
	59,52,45,38,31,
	39,46,53,60,
	61,54,47,
	55,62,
	63,
};

const vector<float> rgb2ycc_mat{
	0.299f, 0.587f, 0.114f,
	-0.168736f, -0.331264f, 0.5f,
	0.5f, -0.418688f, -0.081312f,
};

const vector<float> ycc2rgb_mat{
	1.f, 0.f, 1.402f,
	1.f, -0.344136f, -0.714136f,
	1.f, 1.772f, 0.f,
};


//
//
void RGB2YCC(vector<float>& block, size_t block_id)
{
	size_t offset = block_id * 256;
	auto iter = block.begin() + offset;
	vector<float> copy(iter, iter + 256);

	for (size_t e = 0; e < 256; e += 4)
	{
		block[offset + e + 0] = 0.f + 0.299f * copy[e + 0] + 0.587f * copy[e + 1] + 0.114f * copy[e + 2];
		block[offset + e + 1] = 128.f - 0.168736f*copy[e + 0] - 0.331264f*copy[e + 1] + 0.5f*copy[e + 2];
		block[offset + e + 2] = 128.f + 0.5f*copy[e + 0] - 0.418688f*copy[e + 1] - 0.081312f*copy[e + 2];
	}
}


//
//
void YCC2RGB(vector<float>& block, size_t block_id)
{
	size_t offset = block_id * 256;
	auto iter = block.begin() + offset;
	vector<float> copy(iter, iter + 256);

	for (size_t e = 0; e < 256; e += 4)
	{
		block[offset + e + 0] = copy[e + 0] + 1.402f * (copy[e + 2] - 128.f);
		block[offset + e + 1] = copy[e + 0] - 0.344136f * (copy[e + 1] - 128.f) - 0.714136f * (copy[e + 2] - 128.f);
		block[offset + e + 2] = copy[e + 0] + 1.772f * (copy[e + 1] - 128.f);
	}
}


//
//
void DownSampling422(vector<float>& block, size_t block_id, size_t channel)
{
	size_t offset = block_id * 256 + channel * 64;
	
	for (size_t e = 0; e < 64; e += 2)
	{
		float avg = (block[offset + e] + block[offset + e + 1]) * .5f;
		block[offset + e] = block[offset + e + 1] = avg;
	}
}


//
//
void DownSampling420(vector<float>& block, size_t block_id, size_t channel)
{
	size_t offset = block_id * 256 + channel * 64;

	for (size_t i = 0; i < 8; i += 2)
	{
		for (size_t j = 0; j < 8; j += 2)
		{
			float avg = block[offset + i * 8 + j] + block[offset + i * 8 + j + 1]
				+ block[offset + (i + 1) * 8 + j] + block[offset + (i + 1) * 8 + j + 1];
			block[offset + i * 8 + j] = block[offset + i * 8 + j + 1]
				= block[offset + (i + 1) * 8 + j] = block[offset + (i + 1) * 8 + j + 1] = avg * .25f;
		}
	}
}


//
//
void Quantize(vector<float>& block, size_t block_id, size_t channel, float quality)
{
	size_t offset = block_id * 256 + channel * 64;

	for (size_t e = 0; e < 64; ++e)
		block[offset + e] = std::round(block[offset + e] / quant_mat8x8_jpeg2000[e] * quality);
}


//
//
void Dequantize(vector<float>& block, size_t block_id, size_t channel, float quality)
{
	size_t offset = block_id * 256 + channel * 64;

	for (size_t e = 0; e < 64; ++e)
		block[offset + e] *= quant_mat8x8_jpeg2000[e] / quality;
}


//
// [r, g, b, a] x 64  =>  [r]x64, [g]x64, [b]x64, [a]x64
void UnionChannels(vector<float>& block, size_t block_id)
{
	size_t offset = block_id * 256;
	auto iter = block.begin() + offset;
	vector<float> copy(iter, iter + 256);

	for (size_t c = 0; c < 4; ++c)
		for (size_t i = 0; i < 64; ++i)
			block[offset + c * 64 + i] = copy[i * 4 + c];
}


//
// [r]x64, [g]x64, [b]x64, [a]x64  =>  [r, g, b, a] x 64
void ScatterChannels(vector<float>& block, size_t block_id)
{
	size_t offset = block_id * 256;
	auto iter = block.begin() + offset;
	vector<float> copy(iter, iter + 256);

	for (size_t c = 0; c < 4; ++c)
		for (size_t i = 0; i < 64; ++i)
			block[offset + i * 4 + c] = copy[c * 64 + i];
}


//
//
void Zigzag(vector<float>& block, size_t block_id, size_t channel)
{
	size_t offset = block_id * 256 + channel * 64;
	auto iter = block.begin() + offset;
	vector<float> copy(iter, iter + 64);

	for (size_t e = 0; e < 64; ++e)
		block[offset + e] = copy[zigzag_mat8x8[e]];
}


//
//
void Unzigzag(vector<float>& block, size_t block_id, size_t channel)
{
	size_t offset = block_id * 256 + channel * 64;
	auto iter = block.begin() + offset;
	vector<float> copy(iter, iter + 64);

	for (size_t e = 0; e < 64; ++e)
		block[offset + zigzag_mat8x8[e]] = copy[e];
}
}
}





namespace jpeg
{
namespace huffman_coding
{
using std::vector;
using std::string;
using std::priority_queue;
using std::unordered_map;
using std::function;


template <typename T> struct node;


// build_tree: Generate a Huffman binary tree
// in:  histogram of frequencies
// out: tree (nodes are stored in 1D array)
// ret: root node index
template <typename T>
int build_tree(
	const vector<T>& frequencies,
	vector<node<T> >& nodes)
{
	size_t n = frequencies.size();
	priority_queue<int, vector<int>, function<bool(int,int)> > min_heap([&](int a, int b) {
		return nodes[a].freq > nodes[b].freq;
	});

	for (int i = 0; i < n; ++i)
		min_heap.push(i);

	int curr = (int) n;
	while (min_heap.size() > 1)
	{
		int a = min_heap.top(); min_heap.pop();
		int b = min_heap.top(); min_heap.pop();

		nodes.emplace_back(curr, nodes[a].freq + nodes[b].freq, a, b);
		min_heap.push(curr);
		++curr;
	}

	return min_heap.top();
}


// build_codes: Generate codes for each signal
// in:  Huffman binary tree
// in:  index of current node
// in:  prefix till current node
// out: codes of every signal
template <typename T>
void build_codes(
	const vector<node<T> >& nodes,
	int root,
	string& prefix,
	vector<string>& codes)
{
	if (nodes[root].left == -1)
	{
		codes[root] = prefix;
		return;
	}
	else
	{
		prefix.push_back('0');
		build_codes(nodes, nodes[root].left, prefix, codes);
		prefix.pop_back();

		prefix.push_back('1');
		build_codes(nodes, nodes[root].right, prefix, codes);
		prefix.pop_back();
	}
}


// Encode: Encrypt based on signal frequencies
// in:  histogram of frequencies
// out: corresponding Huffman codes
template <typename T>
void Encode(
	const vector<T>& frequencies,
	vector<string>& codes)
{
	size_t n = frequencies.size();
	vector<node<T> > nodes;

	for (int i = 0; i < n; ++i)
		nodes.emplace_back(i, frequencies[i]);

	int root = build_tree(frequencies, nodes);

	if (root == 0)
	{
		codes[0] = "0";
		return;
	}

	string prefix{};
	build_codes(nodes, root, prefix, codes);
}


// Decode: Decrypt based on Huffman codes
// in:  encrypted data or file
// in:  signals collection
// in:  Huffman codes of signals
// out: real data
// ret: error: if encrypted data is ill-formatted
template <typename T>
bool Decode(
	const vector<bool>& data,
	const vector<T>& signals_collection,
	const vector<string>& codes,
	vector<T>& signals_output)
{
	size_t n = codes.size();
	unordered_map<string, size_t> dict;

	for (size_t i = 0; i < n; ++i)
	{
		dict[codes[i]] = i;
	}

	string prefix{};
	for (bool bit : data)
	{
		prefix.push_back(bit ? '1' : '0');
		auto it = dict.find(prefix);
		
		if (it != dict.end())
		{
			signals_output.push_back(signals_collection[it->second]);
			prefix.clear();
		}
	}

	return !prefix.empty();
}



// Explicit definitions
template void Encode(
	const vector<int>&,
	vector<string>&);
template void Encode(
	const vector<float>&,
	vector<string>&);
template void Encode(
	const vector<double>&,
	vector<string>&);

template bool Decode(
	const vector<bool>&,
	const vector<char>&,
	const vector<string>&,
	vector<char>&);
}
}





namespace jpeg
{
namespace huffman_coding
{
using std::pair;
using std::vector;
using std::string;
using std::unordered_map;


struct DC_t
{
	int category;
	int length;
	string basecode;
};

struct AC_t
{
	int run; // 0-run length
	int category;
	int length;
	string basecode;
};



const vector<DC_t> DC_Table{
//  cat      len    code
	{0x0,     3,    "010"      ,}, // 0
	{0x1,     4,    "011"      ,}, // 1
	{0x2,     5,    "100"      ,}, // 2
	{0x3,     5,    "00"       ,}, // 3
	{0x4,     7,    "101"      ,}, // 4
	{0x5,     8,    "110"      ,}, // 5
	{0x6,    10,    "1110"     ,}, // 6
	{0x7,    12,    "11110"    ,}, // 7
	{0x8,    14,    "111110"   ,}, // 8
	{0x9,    16,    "1111110"  ,}, // 9
	{0xA,    18,    "11111110" ,}, // A
	{0xB,    20,    "111111110",}, // B
};

const unordered_map<string, int> DC_BaseCode{
	// code       cat
	{"010"      , 0},
	{"011"      , 1},
	{"100"      , 2},
	{"00"       , 3},
	{"101"      , 4},
	{"110"      , 5},
	{"1110"     , 6},
	{"11110"    , 7},
	{"111110"   , 8},
	{"1111110"  , 9},
	{"11111110" , 10},
	{"111111110", 11},
};


const vector<AC_t> AC_Table{
//	0-run   cat     len    code
	// EOB
	{0x0,   0x0,     4,    "1010"					,},
	// 0
	{0x0,   0x1,     3,    "00"						,},
	{0x0,   0x2,     4,    "01"						,},
	{0x0,   0x3,     6,    "100"					,},
	{0x0,   0x4,     8,    "1011"					,},
	{0x0,   0x5,    10,    "11010"					,},
	{0x0,   0x6,    12,    "111000"					,},
	{0x0,   0x7,    14,    "1111000"				,},
	{0x0,   0x8,    18,    "1111110110"				,},
	{0x0,   0x9,    25,    "1111111110000010"		,},
	{0x0,   0xA,    26,    "1111111110000011"		,},
	// 1
	{0x1,   0x1,     5,    "1100"					,},
	{0x1,   0x2,     8,    "111001"					,},
	{0x1,   0x3,    10,    "1111001"				,},
	{0x1,   0x4,    13,    "111110110"				,},
	{0x1,   0x5,    16,    "11111110110"			,},
	{0x1,   0x6,    22,    "1111111110000100"		,},
	{0x1,   0x7,    23,    "1111111110000101"		,},
	{0x1,   0x8,    24,    "1111111110000110"		,},
	{0x1,   0x9,    25,    "1111111110000111"		,},
	{0x1,   0xA,    26,    "1111111110001000"		,},
	// 2
	{0x2,   0x1,     6,    "11011"					,},
	{0x2,   0x2,    10,    "11111000"				,},
	{0x2,   0x3,    13,    "1111110111"				,},
	{0x2,   0x4,    20,    "1111111110001001"		,},
	{0x2,   0x5,    21,    "1111111110001010"		,},
	{0x2,   0x6,    22,    "1111111110001011"		,},
	{0x2,   0x7,    23,    "1111111110001100"		,},
	{0x2,   0x8,    24,    "1111111110001101"		,},
	{0x2,   0x9,    25,    "1111111110001110"		,},
	{0x2,   0xA,    26,    "1111111110001111"		,},
	// 3
	{0x3,   0x1,     7,    "111010"					,},
	{0x3,   0x2,    11,    "111110111"				,},
	{0x3,   0x3,    14,    "11111110111"			,},
	{0x3,   0x4,    20,    "1111111110010000"		,},
	{0x3,   0x5,    21,    "1111111110010001"		,},
	{0x3,   0x6,    22,    "1111111110010010"		,},
	{0x3,   0x7,    23,    "1111111110010011"		,},
	{0x3,   0x8,    24,    "1111111110010100"		,},
	{0x3,   0x9,    25,    "1111111110010101"		,},
	{0x3,   0xA,    26,    "1111111110010110"		,},
	// 4
	{0x4,   0x1,     7,    "111011"					,},
	{0x4,   0x2,    12,    "1111111000"				,},
	{0x4,   0x3,    19,    "1111111110010111"		,},
	{0x4,   0x4,    20,    "1111111110011000"		,},
	{0x4,   0x5,    21,    "1111111110011001"		,},
	{0x4,   0x6,    22,    "1111111110011010"		,},
	{0x4,   0x7,    23,    "1111111110011011"		,},
	{0x4,   0x8,    24,    "1111111110011100"		,},
	{0x4,   0x9,    25,    "1111111110011101"		,},
	{0x4,   0xA,    26,    "1111111110011110"		,},
	// 5
	{0x5,   0x1,     8,    "1111010"				,},
	{0x5,   0x2,    12,    "1111111001"				,},
	{0x5,   0x3,    19,    "1111111110011111"		,},
	{0x5,   0x4,    20,    "1111111110100000"		,},
	{0x5,   0x5,    21,    "1111111110100001"		,},
	{0x5,   0x6,    22,    "1111111110100010"		,},
	{0x5,   0x7,    23,    "1111111110100011"		,},
	{0x5,   0x8,    24,    "1111111110100100"		,},
	{0x5,   0x9,    25,    "1111111110100101"		,},
	{0x5,   0xA,    26,    "1111111110100110"		,},
	// 6
	{0x6,   0x1,     8,    "1111011"				,},
	{0x6,   0x2,    13,    "11111111000"			,},
	{0x6,   0x3,    19,    "1111111110100111"		,},
	{0x6,   0x4,    20,    "1111111110101000"		,},
	{0x6,   0x5,    21,    "1111111110101001"		,},
	{0x6,   0x6,    22,    "1111111110101010"		,},
	{0x6,   0x7,    23,    "1111111110101011"		,},
	{0x6,   0x8,    24,    "1111111110101100"		,},
	{0x6,   0x9,    25,    "1111111110101101"		,},
	{0x6,   0xA,    26,    "1111111110101110"		,},
	// 7
	{0x7,   0x1,     9,    "11111001"				,},
	{0x7,   0x2,    13,    "11111111001"			,},
	{0x7,   0x3,    19,    "1111111110101111"		,},
	{0x7,   0x4,    20,    "1111111110110000"		,},
	{0x7,   0x5,    21,    "1111111110110001"		,},
	{0x7,   0x6,    22,    "1111111110110010"		,},
	{0x7,   0x7,    23,    "1111111110110011"		,},
	{0x7,   0x8,    24,    "1111111110110100"		,},
	{0x7,   0x9,    25,    "1111111110110101"		,},
	{0x7,   0xA,    26,    "1111111110110110"		,},
	// 8
	{0x8,   0x1,     9,    "11111001"				,},
	{0x8,   0x2,    17,    "111111111000000"		,},
	{0x8,   0x3,    19,    "1111111110110111"		,},
	{0x8,   0x4,    20,    "1111111110111000"		,},
	{0x8,   0x5,    21,    "1111111110111001"		,},
	{0x8,   0x6,    22,    "1111111110111010"		,},
	{0x8,   0x7,    23,    "1111111110111011"		,},
	{0x8,   0x8,    24,    "1111111110111100"		,},
	{0x8,   0x9,    25,    "1111111110111101"		,},
	{0x8,   0xA,    26,    "1111111110111110"		,},
	// 9
	{0x9,   0x1,    10,    "111111000"				,},
	{0x9,   0x2,    18,    "1111111110111111"		,},
	{0x9,   0x3,    19,    "1111111111000000"		,},
	{0x9,   0x4,    20,    "1111111111000001"		,},
	{0x9,   0x5,    21,    "1111111111000010"		,},
	{0x9,   0x6,    22,    "1111111111000011"		,},
	{0x9,   0x7,    23,    "1111111111000100"		,},
	{0x9,   0x8,    24,    "1111111111000101"		,},
	{0x9,   0x9,    25,    "1111111111000110"		,},
	{0x9,   0xA,    26,    "1111111111000111"		,},
	// A
	{0xA,   0x1,    10,    "111111001"				,},
	{0xA,   0x2,    18,    "1111111111001000"		,},
	{0xA,   0x3,    19,    "1111111111001001"		,},
	{0xA,   0x4,    20,    "1111111111001010"		,},
	{0xA,   0x5,    21,    "1111111111001011"		,},
	{0xA,   0x6,    22,    "1111111111001100"		,},
	{0xA,   0x7,    23,    "1111111111001101"		,},
	{0xA,   0x8,    24,    "1111111111001110"		,},
	{0xA,   0x9,    25,    "1111111111001111"		,},
	{0xA,   0xA,    26,    "1111111111010000"		,},
	// B
	{0xB,   0x1,    10,    "111111010"				,},
	{0xB,   0x2,    18,    "1111111111010001"		,},
	{0xB,   0x3,    19,    "1111111111010010"		,},
	{0xB,   0x4,    20,    "1111111111010011"		,},
	{0xB,   0x5,    21,    "1111111111010100"		,},
	{0xB,   0x6,    22,    "1111111111010101"		,},
	{0xB,   0x7,    23,    "1111111111010110"		,},
	{0xB,   0x8,    24,    "1111111111010111"		,},
	{0xB,   0x9,    25,    "1111111111011000"		,},
	{0xB,   0xA,    26,    "1111111111011001"		,},
	// C
	{0xC,   0x1,    11,    "1111111010"				,},
	{0xC,   0x2,    18,    "1111111111011010"		,},
	{0xC,   0x3,    19,    "1111111111011011"		,},
	{0xC,   0x4,    20,    "1111111111011100"		,},
	{0xC,   0x5,    21,    "1111111111011101"		,},
	{0xC,   0x6,    22,    "1111111111011110"		,},
	{0xC,   0x7,    23,    "1111111111011111"		,},
	{0xC,   0x8,    24,    "1111111111100000"		,},
	{0xC,   0x9,    25,    "1111111111100001"		,},
	{0xC,   0xA,    26,    "1111111111100010"		,},
	// D
	{0xD,   0x1,    12,    "11111111010"			,},
	{0xD,   0x2,    18,    "1111111111100011"		,},
	{0xD,   0x3,    19,    "1111111111100100"		,},
	{0xD,   0x4,    20,    "1111111111100101"		,},
	{0xD,   0x5,    21,    "1111111111100110"		,},
	{0xD,   0x6,    22,    "1111111111100111"		,},
	{0xD,   0x7,    23,    "1111111111101000"		,},
	{0xD,   0x8,    24,    "1111111111101001"		,},
	{0xD,   0x9,    25,    "1111111111101010"		,},
	{0xD,   0xA,    26,    "1111111111101011"		,},
	// E
	{0xE,   0x1,    13,    "111111110110"			,},
	{0xE,   0x2,    18,    "1111111111101100"		,},
	{0xE,   0x3,    19,    "1111111111101101"		,},
	{0xE,   0x4,    20,    "1111111111101110"		,},
	{0xE,   0x5,    21,    "1111111111101111"		,},
	{0xE,   0x6,    22,    "1111111111110000"		,},
	{0xE,   0x7,    23,    "1111111111110001"		,},
	{0xE,   0x8,    24,    "1111111111110010"		,},
	{0xE,   0x9,    25,    "1111111111110011"		,},
	{0xE,   0xA,    26,    "1111111111110100"		,},
	// F
	{0xF,   0x1,    17,    "1111111111110101"		,},
	{0xF,   0x2,    18,    "1111111111110110"		,},
	{0xF,   0x3,    19,    "1111111111110111"		,},
	{0xF,   0x4,    20,    "1111111111111000"		,},
	{0xF,   0x5,    21,    "1111111111111001"		,},
	{0xF,   0x6,    22,    "1111111111111010"		,},
	{0xF,   0x7,    23,    "1111111111111011"		,},
	{0xF,   0x8,    24,    "1111111111111100"		,},
	{0xF,   0x9,    25,    "1111111111111101"		,},
	{0xF,   0xA,    26,    "1111111111111110"		,},
	//10
	{0x10,  0x0,    12,    "111111110111"			,},
};

const unordered_map<string, int> AC_BaseCode{
	{"1010"					,  0},

	{"00"					,  1},
	{"01"					,  2},
	{"100"					,  3},
	{"1011"					,  4},
	{"11010"				,  5},
	{"111000"				,  6},
	{"1111000"				,  7},
	{"1111110110"			,  8},
	{"1111111110000010"		,  9},
	{"1111111110000011"		, 10},

	{"1100"					, 11},
	{"111001"				, 12},
	{"1111001"				, 13},
	{"111110110"			, 14},
	{"11111110110"			, 15},
	{"1111111110000100"		, 16},
	{"1111111110000101"		, 17},
	{"1111111110000110"		, 18},
	{"1111111110000111"		, 19},
	{"1111111110001000"		, 20},

	{"11011"				, 21},
	{"11111000"				, 22},
	{"1111110111"			, 23},
	{"1111111110001001"		, 24},
	{"1111111110001010"		, 25},
	{"1111111110001011"		, 26},
	{"1111111110001100"		, 27},
	{"1111111110001101"		, 28},
	{"1111111110001110"		, 29},
	{"1111111110001111"		, 30},

	{"111010"				, 31},
	{"111110111"			, 32},
	{"11111110111"			, 33},
	{"1111111110010000"		, 34},
	{"1111111110010001"		, 35},
	{"1111111110010010"		, 36},
	{"1111111110010011"		, 37},
	{"1111111110010100"		, 38},
	{"1111111110010101"		, 39},
	{"1111111110010110"		, 40},

	{"111011"				, 41},
	{"1111111000"			, 42},
	{"1111111110010111"		, 43},
	{"1111111110011000"		, 44},
	{"1111111110011001"		, 45},
	{"1111111110011010"		, 46},
	{"1111111110011011"		, 47},
	{"1111111110011100"		, 48},
	{"1111111110011101"		, 49},
	{"1111111110011110"		, 50},

	{"1111010"				, 51},
	{"1111111001"			, 52},
	{"1111111110011111"		, 53},
	{"1111111110100000"		, 54},
	{"1111111110100001"		, 55},
	{"1111111110100010"		, 56},
	{"1111111110100011"		, 57},
	{"1111111110100100"		, 58},
	{"1111111110100101"		, 59},
	{"1111111110100110"		, 60},

	{"1111011"				, 61},
	{"11111111000"			, 62},
	{"1111111110100111"		, 63},
	{"1111111110101000"		, 64},
	{"1111111110101001"		, 65},
	{"1111111110101010"		, 66},
	{"1111111110101011"		, 67},
	{"1111111110101100"		, 68},
	{"1111111110101101"		, 69},
	{"1111111110101110"		, 70},

	{"11111001"				, 71},
	{"11111111001"			, 72},
	{"1111111110101111"		, 73},
	{"1111111110110000"		, 74},
	{"1111111110110001"		, 75},
	{"1111111110110010"		, 76},
	{"1111111110110011"		, 77},
	{"1111111110110100"		, 78},
	{"1111111110110101"		, 79},
	{"1111111110110110"		, 80},

	{"11111001"				, 81},
	{"111111111000000"		, 82},
	{"1111111110110111"		, 83},
	{"1111111110111000"		, 84},
	{"1111111110111001"		, 85},
	{"1111111110111010"		, 86},
	{"1111111110111011"		, 87},
	{"1111111110111100"		, 88},
	{"1111111110111101"		, 89},
	{"1111111110111110"		, 90},

	{"111111000"			, 91},
	{"1111111110111111"		, 92},
	{"1111111111000000"		, 93},
	{"1111111111000001"		, 94},
	{"1111111111000010"		, 95},
	{"1111111111000011"		, 96},
	{"1111111111000100"		, 97},
	{"1111111111000101"		, 98},
	{"1111111111000110"		, 99},
	{"1111111111000111"		,100},

	{"111111001"			,101},
	{"1111111111001000"		,102},
	{"1111111111001001"		,103},
	{"1111111111001010"		,104},
	{"1111111111001011"		,105},
	{"1111111111001100"		,106},
	{"1111111111001101"		,107},
	{"1111111111001110"		,108},
	{"1111111111001111"		,109},
	{"1111111111010000"		,110},

	{"111111010"			,111},
	{"1111111111010001"		,112},
	{"1111111111010010"		,113},
	{"1111111111010011"		,114},
	{"1111111111010100"		,115},
	{"1111111111010101"		,116},
	{"1111111111010110"		,117},
	{"1111111111010111"		,118},
	{"1111111111011000"		,119},
	{"1111111111011001"		,120},

	{"1111111010"			,121},
	{"1111111111011010"		,122},
	{"1111111111011011"		,123},
	{"1111111111011100"		,124},
	{"1111111111011101"		,125},
	{"1111111111011110"		,126},
	{"1111111111011111"		,127},
	{"1111111111100000"		,128},
	{"1111111111100001"		,129},
	{"1111111111100010"		,130},

	{"11111111010"			,131},
	{"1111111111100011"		,132},
	{"1111111111100100"		,133},
	{"1111111111100101"		,134},
	{"1111111111100110"		,135},
	{"1111111111100111"		,136},
	{"1111111111101000"		,137},
	{"1111111111101001"		,138},
	{"1111111111101010"		,139},
	{"1111111111101011"		,140},

	{"111111110110"			,141},
	{"1111111111101100"		,142},
	{"1111111111101101"		,143},
	{"1111111111101110"		,144},
	{"1111111111101111"		,145},
	{"1111111111110000"		,146},
	{"1111111111110001"		,147},
	{"1111111111110010"		,148},
	{"1111111111110011"		,149},
	{"1111111111110100"		,150},

	{"1111111111110101"		,151},
	{"1111111111110110"		,152},
	{"1111111111110111"		,153},
	{"1111111111111000"		,154},
	{"1111111111111001"		,155},
	{"1111111111111010"		,156},
	{"1111111111111011"		,157},
	{"1111111111111100"		,158},
	{"1111111111111101"		,159},
	{"1111111111111110"		,160},

	{"111111110111"			,161},
};



// data2category
// Determine which category data belongs to
// in:  data to be encrypted
// out: category
int data2category(int val)
{
	int category{};

	for (int a = std::abs(val); a > 0; a >>= 1, ++category) {}

	return category;
}


// data2code
// Build datacode based on data and its category
// in:  data
// in:  category
// out: datacode of LSBs expression
string data2code(int val, int category)
{
	// get last significant bits
	int mask = (1 << category) - 1;
	int lsbs = (val & mask) - (val < 0 ? 1 : 0);

	// get datacode
	string datacode(category, '0');
	for (int i = category - 1; i >= 0; --i, lsbs >>= 1)
	{ if (lsbs & 1) datacode[i] = '1'; }

	return datacode;
}


//
//
void Encode_DC(int val, BitStream* out)
{
	string code{};

	// determine category data belongs to
	int category = data2category(val);

	// get basecode
	code.append(DC_Table[category].basecode);

	// get datacode
	if (category != 0)
		code.append(data2code(val, category));

	out->Add(code);
}


//
//
void Encode_AC(int run, int val, BitStream* out)
{
	string code{};

	// in case run length exceeds encoding standard
	if (run > 0xF)
	{
		code.append(AC_Table[0xA1].basecode);
		out->Add(code);
		return;
	}

	// determine category data belongs to
	int category = data2category(val);

	// if category is 0, nothing to do
	int id = run * 10 + category;

	// get basecode
	code.append(AC_Table[id].basecode);

	// get datacode
	code.append(data2code(val, category));

	out->Add(code);
}


// EncodeBlock
// Encode a data block
// in:  8x8 block of float data
// in:  previous DC coefficients
// out: JPEG code of data
void EncodeBlock(
	const vector<float>& block,
	size_t block_id,
	size_t channel,
	int& prev,
	BitStream* out)
{
	size_t offset = block_id * 256 + channel * 64;

	// diff between current DC coef and previous one
	int diff = (int)block[offset + 0] - prev;
	prev = (int)block[offset + 0];

	// encode DC component
	jpeg::huffman_coding::Encode_DC(diff, out);

	// encode AC components
	int run{}, val{};
	for (int i = 1; i < 64; ++i)
	{
		val = (int)block[offset + i];

		if (val != 0)
		{
			jpeg::huffman_coding::Encode_AC(run, val, out);
			run = 0;
		}
		else
		{
			++run;
		}
	}

	// Attach END of BLOCK code segment
	jpeg::huffman_coding::Encode_AC(0, 0, out);
}



// code2data
// Decrypt datacode based on LSBs expression
// in:  datacode of LSBs expression
// in:  category
// out: data
int code2data(const string& datacode, int category)
{
	int val{};

	// get data from last significant bits
	for (char c : datacode)
	{
		val <<= 1;
		val += c - '0';
	}

	// do something if original data is negative
	if (datacode[0] == '0')
		val -= (1 << category) - 1;

	return val;
}


// scan_code
// Scan over codes to decrypt data and push forward
// in:  code (will be deleted visited bits during scanning)
// in:  AC(1) or DC(0)
// out: id or pointer to table entity
// out: datacode of LSBs expression
int scan_code(BitStream* in, bool AC, string& datacode)
{
	int id{ -1 }, len{ }, bit{};
	string basecode{};
	const unordered_map<string, int>& CodeDict = AC ? AC_BaseCode : DC_BaseCode;

	datacode.clear();

	while (!in->empty())
	{
		bit = in->Pop();

		if (bit == -1)
			throw std::exception("Invalid code format");

		basecode.push_back('0' + bit);
		auto jt = CodeDict.find(basecode);

		// code buffer matches one of basecodes
		if (jt != CodeDict.end())
		{
			id = jt->second;

			if (AC)
				len = AC_Table[id].category;
			else
				len = DC_Table[id].category;

			// read corresponding datacode
			for (size_t i = 0; i < len; ++i)
			{
				bit = in->Pop();

				if (bit == -1)
					throw std::exception("Invalid code format");

				datacode.push_back('0' + bit);
			}

			break;
		}
	}

	return id;
}


//
//
int Decode_DC(BitStream* in)
{
	int val{};
	string datacode{};

	// scan datacode
	int id = scan_code(in, false, datacode);

	// code is not well-formatted
	if (id == -1) throw std::exception("DC coef ill format");

	// get category of data
	int category = DC_Table[id].category;

	// if category is 0, return 0 directly
	if (category == 0) return 0;

	// get data from last significant bits
	val = code2data(datacode, category);

	return val;
}


//
//
pair<int,int> Decode_AC(BitStream* in)
{
	int val{};
	string datacode{};

	// extract datacode and determine category
	int id = scan_code(in, true, datacode);

	// code is not well-formatted
	if (id == -1) throw std::exception("AC coef ill format");

	// get run length and category of data
	int run = AC_Table[id].run;
	int category = AC_Table[id].category;

	// get data from last significant bits
	val = code2data(datacode, category);

	return { run, val };
}


//
//
void DecodeBlock(
	vector<float>& block,
	size_t block_id,
	size_t channel,
	int& prev,
	BitStream* in)
{
	size_t offset = block_id * 256 + channel * 64;

	// decode DC component
	int curr = jpeg::huffman_coding::Decode_DC(in) + prev;
	prev = curr;
	block[offset + 0] = (float)curr;

	// decode AC components
	int run{}, val{}, id{ 1 };
	do
	{
		auto p = jpeg::huffman_coding::Decode_AC(in);
		run = p.first, val = p.second;

		if (id + run >= 64)
		{
			if (run > 0)
				throw std::out_of_range("Invalid run length");
			else
				break;
		}

		for (int i = 0; i < run; ++i, ++id)
			block[offset + id] = 0.0f;

		block[offset + id] = (float)val;
		++id;
	}
	while (run != 0 || val != 0);
}
}
}