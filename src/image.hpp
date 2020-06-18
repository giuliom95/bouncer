#ifndef _IMAGE_HPP_
#define _IMAGE_HPP_

#include "math.hpp"
#include <vector>

class Image3f
{
public:
	unsigned int width, height;

	Image3f(unsigned int w, unsigned int h);

private:
	std::vector<Vec3f> data;
};

#endif