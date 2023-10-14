#pragma once

#include <glad/glad.h>

class IBO
{
public:
	IBO(const unsigned int* indices, int size);

	void bind();
	void unbind();

private:
	unsigned int ID;
};
