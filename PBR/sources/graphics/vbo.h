#pragma once

#include <glad/glad.h>

class VBO
{
public:
	VBO(const float* vertices, int size);

	void bind();
	void unbind();

private:
	unsigned int ID;
};
