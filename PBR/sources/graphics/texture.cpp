#include "texture.h"

Texture::Texture(const char* filepath, bool hdr, bool gammaCorrection)
	: ID(), width(), height(), colorChannels()
{
	stbi_set_flip_vertically_on_load(true);

	if (!hdr)
	{
		unsigned char* data = stbi_load(filepath, &width, &height, &colorChannels, 0);
		int internalFormat = GL_RED, format = GL_RED; // Default format.

		// WARNING: We are only expecting an image with 1, 3 or 4 color channels. Any other format may generate some OpenGL error.
		//			Also, images with only 1 color channel don't support gamma correction.

		glGenTextures(1, &ID);
		glBindTexture(GL_TEXTURE_2D, ID);

		switch (colorChannels)
		{
		case 1:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			break;

		case 3:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
			format = GL_RGB;

			break;

		case 4:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
			format = GL_RGBA;

			break;

		default:
			std::cout << "[ERROR] TEXTURE: Texture format not supported, loaded with " << colorChannels << " channel(s)." << std::endl;

			break;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "[ERROR] TEXTURE: Failed to load texture in \"" << filepath << "\"." << std::endl;
		}

		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(data);
	}
	else
	{
		float* data = stbi_loadf(filepath, &width, &height, &colorChannels, 0);

		glGenTextures(1, &ID);
		glBindTexture(GL_TEXTURE_2D, ID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
		}
		else
		{
			std::cout << "[ERROR] TEXTURE: Failed to load HDR image in \"" << filepath << "\"." << std::endl;
		}

		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(data);
	}
}

void Texture::bind(int unit)
{
	if (unit >= 0 && unit <= 15)
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, ID);
	}
	else
	{
		std::cout << "[ERROR] TEXTURE: Failed to bind texture in " << unit << " unit." << std::endl;
	}
}

void Texture::unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}
