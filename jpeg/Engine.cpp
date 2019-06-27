#include "Engine.h"
#include "jpeg.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>
#include <cstdio>
#include <vector>
#include <functional>
#include <stdexcept>

#include <omp.h>

using namespace std;

#define DEBUG

void DisplayModuleWallTime(const string& info)
{
#ifdef DEBUG
	static float last = omp_get_wtime();
	float curr = omp_get_wtime();
	printf("%s: %f\n", info.c_str(), curr - last);
	last = curr;
#endif // DEBUG
}


Canvas::Canvas() : m_Pixels(nullptr), m_stream(nullptr)
{}

Canvas::~Canvas()
{
	Free();
}

bool Canvas::Init(int w, int h)
{
	m_width = w;
	m_height = h;

	allocPixel(w, h);
	if (!m_Pixels)
		throw std::exception("Bad alloc");

	// choose one stream
	m_stream = new StringBitStream;
	if (!m_stream)
		throw std::exception("Bad alloc");

	return true;
}

void Canvas::Free()
{
	if (m_Pixels)
	{
		freePixel();
	}

	if (m_stream)
	{
		delete m_stream;
	}
}

bool Canvas::allocPixel(int w, int h)
{
	m_Pixels = (unsigned char*) malloc(sizeof(unsigned char) * w * h * 4);
	return m_Pixels != nullptr;
}

void Canvas::freePixel()
{
	free(m_Pixels);
	m_Pixels = nullptr;
}

void Canvas::EditPixel(
	int x, int y,
	vector<unsigned char>&& color,
	int scale)
{
	const int w = m_width;
	const int h = m_height;
	const int sx = (x - scale) > 0 ? x - scale : 0;
	const int ex = (x + scale) < w - 1 ? x + scale : w - 1;
	const int sy = (y - scale) > 0 ? y - scale : 0;
	const int ey = (y + scale) < h - 1 ? y + scale : h - 1;
	for (int i = sy; i <= ey; ++i)
	{
		for (int j = sx; j <= ex; ++j)
		{
			const unsigned int offset = (w * i + j) * 4;
			m_Pixels[offset + 0] = color[0];
			m_Pixels[offset + 1] = color[1];
			m_Pixels[offset + 2] = color[2];
			m_Pixels[offset + 3] = color[3];
		}
	}
}

void Canvas::SetAllPixels(std::vector<unsigned char>&& color)
{
	const int w = m_width;
	const int h = m_height;
	for (int i = 0; i < h; ++i)
	{
		for (int j = 0; j < w; ++j)
		{
			const unsigned int offset = (w * i + j) * 4;
			m_Pixels[offset + 0] = color[0];
			m_Pixels[offset + 1] = color[1];
			m_Pixels[offset + 2] = color[2];
			m_Pixels[offset + 3] = color[3];
		}
	}
}

void Canvas::SetAllPixels(
	function<float(int, int, const vector<float>&)> func,
	const vector<float>& param,
	bool alpha)
{
	const int w = m_width;
	const int h = m_height;

	for (int i = 0; i < h; ++i)
	{
		for (int j = 0; j < w; ++j)
		{
			const unsigned int offset = (w * i + j) * 4;
			float val = func(i, j, param);
			unsigned char color = val > 255.f ? 255 : val < 0.f ? 0 : (unsigned char)val;
			m_Pixels[offset + 0] = color;
			m_Pixels[offset + 1] = color;
			m_Pixels[offset + 2] = color;
			m_Pixels[offset + 3] = alpha ? color : 255;
		}
	}
}

void Canvas::SetAllPixels(
	vector<function<float(int, int, const vector<float>&)>> funcs,
	const vector<vector<float>>& params,
	int channel)
{
	const int w = m_width;
	const int h = m_height;

	for (int i = 0; i < h; ++i)
	{
		for (int j = 0; j < w; ++j)
		{
			for (int c = 0; c < 4; ++c)
			{
				if (((1 << (3 - c)) & channel) == 0) continue;
				float val = funcs[c](i, j, params[c]);
				unsigned char color = val > 255.f ? 255 : val < 0.f ? 0 : (unsigned char)val;
				m_Pixels[(w * i + j) * 4 + c] = color;
			}
		}
	}
}

bool Canvas::SaveAsJPEG(const string& filename, double quality)
{
	fstream fs(filename, ios::out);
	if (!fs) throw std::exception("File missing");

	// write image config to file header
	string config = to_string(m_width) + " " + to_string(m_height) + " " + to_string(quality);
	fs << config << endl;

	// jpeg code
	writeCodeJPEG(quality);

	// save encoded stuff to file or buffer
	m_stream->Write(fs);

	// calculate compress ratio
	cout << "Save image to: " << filename << endl;
	cout << "Compress ratio: " << (float)(m_width * m_height * 4) * 8.f / (float)m_stream->size() << endl;

	return true;
}

// writeCodeJPEG
//
void Canvas::writeCodeJPEG(float quality)
{
	const int w = m_width;
	const int h = m_height;
	const int n = w * h;
	const int nbw = w / 8;
	const int nbh = h / 8;
	const int nb = nbw * nbh;
	vector<float> blocks(nbw * nbh * 256, 0.f);
	vector<int> prev_dc_coef(4, 0);

	DisplayModuleWallTime("");

	// divide pixels into 8x8 blocks
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			for (size_t l = 0; l < 8; ++l)
				for (size_t e = 0; e < 32; ++e)
					blocks[(i *nbw + j) * 256 + l * 32 + e] = m_Pixels[((i * 8 + l) *w + j * 8) * 4 + e];

	DisplayModuleWallTime("Dividing image into blocks");

	// RGBA to YCrCb
	// TODO

	// Union same channel in buffer
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			jpeg::util::UnionChannels(blocks, i * nbw + j);

	DisplayModuleWallTime("Unioning channels");

	// DCT
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			for (size_t c = 0; c < 4; ++c)
				jpeg::dct::ForwardTransform8x8(blocks, i * nbw + j, c);

	DisplayModuleWallTime("DCT");

	// Quantize
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			for (size_t c = 0; c < 4; ++c)
				jpeg::util::Quantize(blocks, i * nbw + j, c, quality);

	DisplayModuleWallTime("Quantization");

	// Zigzag
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			for (size_t c = 0; c < 4; ++c)
				jpeg::util::Zigzag(blocks, i * nbw + j, c);

	DisplayModuleWallTime("Zigzaging");

	// Huffman coding
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			for (size_t c = 0; c < 4; ++c)
				jpeg::huffman_coding::EncodeBlock(blocks, i * nbw + j, c, prev_dc_coef[c], m_stream);

	DisplayModuleWallTime("Coding");
}

bool Canvas::ReadAsJPEG(const std::string& filename)
{
	string config{};
	fstream fs(filename, ios::in);
	if (!fs) throw std::exception("File missing");

	// read file header
	if (!getline(fs, config))
		throw std::exception("Incomplete code");

	int w, h;
	double quality;
	stringstream ss(config);

	ss >> w >> h >> quality;

	if (w != m_width || h != m_height)
	{
		m_width = w;
		m_height = h;
		if (m_Pixels)
			freePixel();
		if (!allocPixel(w, h))
			throw std::exception("Bad alloc");
	}

	m_stream->Read(fs);

	readCodeJPEG(quality);

	return 1;
}

void Canvas::readCodeJPEG(float quality)
{
	const int w = m_width;
	const int h = m_height;
	const int n = w * h;
	const int nbw = w / 8;
	const int nbh = h / 8;
	const int nb = nbw * nbh;
	vector<float> blocks(nbw * nbh * 256, 0.f);
	vector<int> prev_dc_coef(4, 0);

	DisplayModuleWallTime("");

	// Huffman decoding
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			for (size_t c = 0; c < 4; ++c)
				jpeg::huffman_coding::DecodeBlock(blocks, i * nbw + j, c, prev_dc_coef[c], m_stream);

	DisplayModuleWallTime("Decoding");

	// Unzigzag
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			for (size_t c = 0; c < 4; ++c)
				jpeg::util::Unzigzag(blocks, i * nbw + j, c);

	DisplayModuleWallTime("Unigzaging");

	// Dequantize
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			for (size_t c = 0; c < 4; ++c)
				jpeg::util::Dequantize(blocks, i * nbw + j, c, quality);

	DisplayModuleWallTime("Dequantization");

	// Inverse DCT
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			for (size_t c = 0; c < 4; ++c)
				jpeg::dct::InverseTransform8x8(blocks, i * nbw + j, c);

	DisplayModuleWallTime("INV DCT");

	// union same channel in buffer
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			jpeg::util::ScatterChannels(blocks, i * nbw + j);

	DisplayModuleWallTime("Scatter channels");

	// YCrCb to RGBA
	// TODO

	// write 8x8 blocks back to pixels
	for (size_t i = 0; i < nbh; ++i)
		for (size_t j = 0; j < nbw; ++j)
			for (size_t l = 0; l < 8; ++l)
				for (size_t e = 0; e < 32; ++e)
					m_Pixels[((i * 8 + l) *w + j * 8) * 4 + e] = blocks[(i *nbw + j) * 256 + l * 32 + e];

	DisplayModuleWallTime("Writing blocks back to image");
}
