#pragma once

#include <iostream>

#include <glad/glad.h>

#if !defined _STB_IMAGE_INCLUDED
#define _STB_IMAGE_INCLUDED

#include <stbi/stb_image.h>
#endif // _STB_IMAGE_INCLUDED

class Texture
{
public:
	Texture(const char* filepath, bool hdr = false, bool gammaCorrection = false);
	Texture(int width, int height, int internalFormat, int format, int type);

	unsigned int getID();

	void bind(int unit);
	void unbind();

private:
	unsigned int ID;
	int width, height, colorChannels;
};
