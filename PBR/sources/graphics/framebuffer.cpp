#include "framebuffer.h"

FrameBuffer::FrameBuffer(int width, int height)
	: ID(), depthBufferID()
{
	glGenFramebuffers(1, &ID);
	glBindFramebuffer(GL_FRAMEBUFFER, ID);

	attachRenderBufferAsDepthBuffer(width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, ID);
}

void FrameBuffer::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::bindColorBufferToFrameBuffer(unsigned int colorBufferID, int attachmentNumber, int target, int mipLevel)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentNumber, target, colorBufferID, mipLevel);
}

void FrameBuffer::resizeDepthBuffer(int width, int height)
{
	glBindRenderbuffer(GL_RENDERBUFFER, depthBufferID);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
}

void FrameBuffer::attachRenderBufferAsDepthBuffer(int width, int height)
{
	glGenRenderbuffers(1, &depthBufferID);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBufferID);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferID);

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}
