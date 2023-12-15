#ifndef BITMAP
#define BITMAP

#include <vector>

#include "glm/glm.hpp"

enum eBitmapType
{
	eBitmapType_2D,
	eBitmapType_Cube
};

enum eBitmapFormat
{
	eBitmapFormat_UnsignedByte,
	eBitmapFormat_Float,
};

/// R/RG/RGB/RGBA bitmaps
class Bitmap
{
public:
	Bitmap() = default;
	Bitmap(int w, int h, int comp, eBitmapFormat fmt) :
		w_(w),
		h_(h),
		comp_(comp),
		fmt_(fmt),
		data_(w * h * comp * GetBytesPerComponent(fmt))
	{
		InitGetSetFuncs();
	}
	Bitmap(int w, int h, int d, int comp, eBitmapFormat fmt) :
		w_(w),
		h_(h),
		d_(d),
		comp_(comp),
		fmt_(fmt),
		data_(w * h * d * comp * GetBytesPerComponent(fmt))
	{
		InitGetSetFuncs();
	}
	Bitmap(int w, int h, int comp, eBitmapFormat fmt, const void* ptr) :
		w_(w),
		h_(h),
		comp_(comp),
		fmt_(fmt),
		data_(w * h * comp * GetBytesPerComponent(fmt))
	{
		InitGetSetFuncs();
		memcpy(data_.data(), ptr, data_.size());
	}
	int w_ = 0;
	int h_ = 0;
	int d_ = 1;
	int comp_ = 3;
	eBitmapFormat fmt_ = eBitmapFormat_UnsignedByte;
	eBitmapType type_ = eBitmapType_2D;
	std::vector<uint8_t> data_;

	static int GetBytesPerComponent(eBitmapFormat fmt)
	{
		if (fmt == eBitmapFormat_UnsignedByte) return 1;
		if (fmt == eBitmapFormat_Float) return 4;
		return 0;
	}

	void SetPixel(int x, int y, const glm::vec4& c)
	{
		(*this.*setPixelFunc)(x, y, c);
	}

	glm::vec4 GetPixel(int x, int y) const
	{
		return ((*this.*getPixelFunc)(x, y));
	}

private:
	using setPixel_t = void(Bitmap::*)(int, int, const glm::vec4&);
	using getPixel_t = glm::vec4(Bitmap::*)(int, int) const;
	setPixel_t setPixelFunc = &Bitmap::SetPixelUnsignedByte;
	getPixel_t getPixelFunc = &Bitmap::GetPixelUnsignedByte;

	void InitGetSetFuncs();

	void SetPixelFloat(int x, int y, const glm::vec4& c);

	glm::vec4 GetPixelFloat(int x, int y) const;

	void SetPixelUnsignedByte(int x, int y, const glm::vec4& c);

	glm::vec4 GetPixelUnsignedByte(int x, int y) const;
};

#endif
