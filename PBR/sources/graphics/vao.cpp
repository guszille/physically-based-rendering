#include "vao.h"

VAO::VAO() : ID()
{
	glGenVertexArrays(1, &ID);
}

void VAO::bind()
{
	glBindVertexArray(ID);
}

void VAO::unbind()
{
	glBindVertexArray(0);
}

void VAO::setVertexAttribute(unsigned int index, int size, int type, bool normalized, unsigned int stride, void* pointer, int divisor)
{
	glVertexAttribPointer(index, size, type, normalized, stride, pointer);
	glEnableVertexAttribArray(index);

	// By default, the attribute divisor is 0 which tells OpenGL to update the content of the vertex attribute each iteration of the vertex shader.
	// By setting this attribute to 1 we're telling  OpenGL that we want to update the content of the vertex attribute when we start to render a new instance.
	// By setting it to 2 we'd update the content every 2 instances and so on.
	//
	glVertexAttribDivisor(index, divisor);
}

int VAO::retrieveMaxVertexAttributes()
{
	int numberOfAttributes;

	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &numberOfAttributes);

	return numberOfAttributes;
}
