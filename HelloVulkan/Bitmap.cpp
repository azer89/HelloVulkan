#include "Bitmap.h"

void Bitmap::InitGetSetFuncs()
{
	switch (fmt_)
	{
	case eBitmapFormat_UnsignedByte:
		setPixelFunc = &Bitmap::SetPixelUnsignedByte;
		getPixelFunc = &Bitmap::GetPixelUnsignedByte;
		break;
	case eBitmapFormat_Float:
		setPixelFunc = &Bitmap::SetPixelFloat;
		getPixelFunc = &Bitmap::GetPixelFloat;
		break;
	}
}

void Bitmap::SetPixelFloat(int x, int y, const glm::vec4& c)
{
	const int ofs = comp_ * (y * w_ + x);
	float* data = reinterpret_cast<float*>(data_.data());
	if (comp_ > 0) data[ofs + 0] = c.x;
	if (comp_ > 1) data[ofs + 1] = c.y;
	if (comp_ > 2) data[ofs + 2] = c.z;
	if (comp_ > 3) data[ofs + 3] = c.w;
}
glm::vec4 Bitmap::GetPixelFloat(int x, int y) const
{
	const int ofs = comp_ * (y * w_ + x);
	const float* data = reinterpret_cast<const float*>(data_.data());
	return glm::vec4(
		comp_ > 0 ? data[ofs + 0] : 0.0f,
		comp_ > 1 ? data[ofs + 1] : 0.0f,
		comp_ > 2 ? data[ofs + 2] : 0.0f,
		comp_ > 3 ? data[ofs + 3] : 0.0f);
}

void Bitmap::SetPixelUnsignedByte(int x, int y, const glm::vec4& c)
{
	const int ofs = comp_ * (y * w_ + x);
	if (comp_ > 0) data_[ofs + 0] = uint8_t(c.x * 255.0f);
	if (comp_ > 1) data_[ofs + 1] = uint8_t(c.y * 255.0f);
	if (comp_ > 2) data_[ofs + 2] = uint8_t(c.z * 255.0f);
	if (comp_ > 3) data_[ofs + 3] = uint8_t(c.w * 255.0f);
}
glm::vec4 Bitmap::GetPixelUnsignedByte(int x, int y) const
{
	const int ofs = comp_ * (y * w_ + x);
	return glm::vec4(
		comp_ > 0 ? float(data_[ofs + 0]) / 255.0f : 0.0f,
		comp_ > 1 ? float(data_[ofs + 1]) / 255.0f : 0.0f,
		comp_ > 2 ? float(data_[ofs + 2]) / 255.0f : 0.0f,
		comp_ > 3 ? float(data_[ofs + 3]) / 255.0f : 0.0f);
}