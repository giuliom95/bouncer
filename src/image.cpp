#include "image.hpp"

Image3f::Image3f(unsigned int w, unsigned int h)
{
	width = w;
	height = h;

	data = std::vector<Vec3f>(w*h);
}