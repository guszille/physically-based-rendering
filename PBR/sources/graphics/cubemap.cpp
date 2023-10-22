#include "cubemap.h"

CubeMap::CubeMap(int width, int height, int internalFormat, int format, int type, bool generateMipMaps)
	: ID()
{
	glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, ID);

	for (unsigned int i = 0; i < 6; i++)
	{
		// Texture Target					Orientation
		// 
		// GL_TEXTURE_CUBE_MAP_POSITIVE_X	Right
		// GL_TEXTURE_CUBE_MAP_NEGATIVE_X	Left
		// GL_TEXTURE_CUBE_MAP_POSITIVE_Y	Top
		// GL_TEXTURE_CUBE_MAP_NEGATIVE_Y	Bottom
		// GL_TEXTURE_CUBE_MAP_POSITIVE_Z	Back
		// GL_TEXTURE_CUBE_MAP_NEGATIVE_Z	Front
		//
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, type, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	if (generateMipMaps)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

unsigned int CubeMap::getID()
{
	return ID;
}

void CubeMap::bind(int unit)
{
	if (unit >= 0 && unit <= 15)
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
	}
	else
	{
		std::cout << "[ERROR] CUBEMAP: Failed to bind cubemap texture in " << unit << " unit." << std::endl;
	}
}

void CubeMap::unbind()
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}
