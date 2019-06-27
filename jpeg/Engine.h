#ifndef ENGINE_H
#define ENGINE_H

#ifdef UNIT_TEST_FLAG
#include "Engine.cpp"
#endif

#include <vector>
#include <string>
#include <memory>
#include <functional>

#include "BitStream.h"

class Canvas
{
public:
	Canvas();
	~Canvas();

	bool Init(int w, int h);
	void Free();

	int width() const { return m_width; }
	int height() const { return m_height; }
	const void* pixels() const { return (void*)m_Pixels; }

	void EditPixel(int x, int y, std::vector<unsigned char>&& color, int scale = 0);

	void SetAllPixels(
		std::vector<unsigned char>&& color);
	void SetAllPixels(
		std::function<float(int, int, const std::vector<float>&)>,
		const std::vector<float>&,
		bool alpha = false);
	void SetAllPixels(
		std::vector<std::function<float(int, int, const std::vector<float>&)>>,
		const std::vector<std::vector<float>>&,
		const int channel);

	bool SaveAsJPEG(const std::string& filename, double quality = 1.0);
	bool ReadAsJPEG(const std::string& filename);

private:
	bool allocPixel(int w, int h);
	void freePixel();

	void writeCodeJPEG(float quality);
	void readCodeJPEG(float quality);

private:
	int m_width, m_height;
	unsigned char* m_Pixels;
	BitStream* m_stream;
};

#endif // !ENGINE_H
