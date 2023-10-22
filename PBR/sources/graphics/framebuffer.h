#pragma once

#include <iostream>

#include <glad/glad.h>

class FrameBuffer
{
public:
	FrameBuffer(int width, int height);

	void bind();
	void unbind();

	void bindColorBufferToFrameBuffer(unsigned int colorBufferID, int attachmentNumber, int target);

private:
	unsigned int ID, depthBufferID;

	void attachRenderBufferAsDepthBuffer(int width, int height);
};
