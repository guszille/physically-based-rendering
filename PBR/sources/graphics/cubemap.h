#pragma once

#include <iostream>

#include <glad/glad.h>

#if !defined _STB_IMAGE_INCLUDED
#define _STB_IMAGE_INCLUDED

#include <stbi/stb_image.h>
#endif // _STB_IMAGE_INCLUDED

class CubeMap
{
public:
	CubeMap(int width, int height, int internalFormat, int format, int type, bool generateMipMaps = false);

	unsigned int getID();

	void bind(int unit);
	void unbind();

private:
	unsigned int ID;
};
