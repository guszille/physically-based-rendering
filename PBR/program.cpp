// PBR.

#define STB_IMAGE_IMPLEMENTATION

#include <vector>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "sources/graphics/vao.h"
#include "sources/graphics/vbo.h"
#include "sources/graphics/ibo.h"
#include "sources/graphics/shader.h"
#include "sources/graphics/texture.h"
#include "sources/graphics/cubemap.h"
#include "sources/graphics/framebuffer.h"

#include "sources/utils/camera.h"
#include "sources/utils/debug.h"

// Global variables.
int   WINDOW_WIDTH        = 1280;
int   WINDOW_HEIGHT       = 720;
float WINDOW_ASPECT_RATIO = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
float FIELD_OF_VIEW       = 45.0f;
float DELTA_TIME          = 0.0f;
float LAST_FRAME          = 0.0f;
float CAMERA_SPEED        = 7.5f;
float CAMERA_SENSITIVITY  = 0.05f;
float CURSOR_POS_X        = (float)WINDOW_WIDTH / 2.0f;
float CURSOR_POS_Y        = (float)WINDOW_HEIGHT / 2.0f;
bool  CURSOR_ATTACHED     = false;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
glm::mat4 projectionMatrix = glm::perspective(glm::radians(FIELD_OF_VIEW), WINDOW_ASPECT_RATIO, 0.1f, 100.0f);

ShaderProgram* pbrShader;
ShaderProgram* equirectangularToCubemapShader;
ShaderProgram* environmentShader;
ShaderProgram* irradianceShader;
ShaderProgram* prefilterShader;
ShaderProgram* brdfShader;

VAO* sphereVAO;
VBO* sphereVBO;
IBO* sphereIBO;

unsigned int sphereIndexCount = 0;

VAO* cubeVAO;
VBO* cubeVBO;

VAO* quadVAO;
VBO* quadVBO;

Texture* equirectangularHDRTex;
Texture* brdfLUTTex;

FrameBuffer* captureFB;

int CAPTURE_FB_WIDTH  = 512;
int CAPTURE_FB_HEIGHT = 512;

CubeMap* environmentCM;
CubeMap* irradianceCM;
CubeMap* prefilterCM;

glm::mat4 envProjectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 envViewMatrices[] = {
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

glm::vec3 lightPositions[] = {
	glm::vec3(-10.0f,  10.0f,  10.0f),
	glm::vec3( 10.0f,  10.0f,  10.0f),
	glm::vec3(-10.0f, -10.0f,  10.0f),
	glm::vec3( 10.0f, -10.0f,  10.0f),
};

glm::vec3 lightColors[] = {
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f)
};

// GLFW window callbacks.
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void keyboardCallback(GLFWwindow* window, int key, int scanCode, int action, int mods);
void cursorPositionCallback(GLFWwindow* window, double xPos, double yPos);
void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
void processInput(GLFWwindow* window);

void setupApplication()
{
	const unsigned int X_SEGMENTS = 64;
	const unsigned int Y_SEGMENTS = 64;
	const float PI = 3.14159265359f;

	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<float> sphereVertices;
	std::vector<unsigned int> sphereIndices;

	for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
	{
		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			float yPos = std::cos(ySegment * PI);
			float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

			positions.push_back(glm::vec3(xPos, yPos, zPos));
			normals.push_back(glm::vec3(xPos, yPos, zPos));
			uvs.push_back(glm::vec2(xSegment, ySegment));
		}
	}

	for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
	{
		if (y % 2 == 0) // Even rows.
		{
			for (int x = 0; x <= X_SEGMENTS; ++x)
			{
				sphereIndices.push_back((y) * (X_SEGMENTS + 1) + x);
				sphereIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else // Odd rows.
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				sphereIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				sphereIndices.push_back((y) * (X_SEGMENTS + 1) + x);
			}
		}
	}

	sphereIndexCount = static_cast<unsigned int>(sphereIndices.size());

	for (unsigned int i = 0; i < positions.size(); ++i)
	{
		sphereVertices.push_back(positions[i].x);
		sphereVertices.push_back(positions[i].y);
		sphereVertices.push_back(positions[i].z);

		if (normals.size() > 0)
		{
			sphereVertices.push_back(normals[i].x);
			sphereVertices.push_back(normals[i].y);
			sphereVertices.push_back(normals[i].z);
		}

		if (uvs.size() > 0)
		{
			sphereVertices.push_back(uvs[i].x);
			sphereVertices.push_back(uvs[i].y);
		}
	}

	float cubeVertices[] = {
		// back face
		-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
		 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
		 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
		 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
		-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
		-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
		// front face
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
		 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
		-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
		// left face
		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
		-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
		-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
		// right face
		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
		 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
		 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
		 // bottom face
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
		 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
		 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
		 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
		-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
		 // top face
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
		 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
		 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
		 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
		-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
	};

	float quadVertices[] = {
		// positions        // texture coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f
	};

	pbrShader = new ShaderProgram("sources/shaders/1_pbr_vs.glsl", "sources/shaders/1_pbr_fs.glsl");
	equirectangularToCubemapShader = new ShaderProgram("sources/shaders/3_equirectangular2cubemap_vs.glsl", "sources/shaders/3_equirectangular2cubemap_fs.glsl");
	environmentShader = new ShaderProgram("sources/shaders/3_environment_vs.glsl", "sources/shaders/3_environment_fs.glsl");
	irradianceShader = new ShaderProgram("sources/shaders/3_irradiance_convolution_vs.glsl", "sources/shaders/3_irradiance_convolution_fs.glsl");
	prefilterShader = new ShaderProgram("sources/shaders/4_prefilter_convolution_vs.glsl", "sources/shaders/4_prefilter_convolution_fs.glsl");
	brdfShader = new ShaderProgram("sources/shaders/4_brdf_vs.glsl", "sources/shaders/4_brdf_fs.glsl");

	pbrShader->bind();
	pbrShader->setUniform3f("uAlbedo", 0.5f, 0.0f, 0.0f);
	pbrShader->setUniform1f("uAO", 1.0f);
	pbrShader->setUniform1i("uIrradianceMap", 0);
	pbrShader->setUniform1i("uPrefilterMap", 1);
	pbrShader->setUniform1i("uBRDFLUTMap", 2);
	pbrShader->unbind();

	equirectangularToCubemapShader->bind();
	equirectangularToCubemapShader->setUniform1i("uEquirectangularMap", 0);
	equirectangularToCubemapShader->setUniformMatrix4fv("uProjection", envProjectionMatrix);
	equirectangularToCubemapShader->unbind();

	environmentShader->bind();
	environmentShader->setUniform1i("uEnvironmentMap", 0);
	environmentShader->unbind();

	irradianceShader->bind();
	irradianceShader->setUniform1i("uEnvironmentMap", 0);
	irradianceShader->setUniformMatrix4fv("uProjection", envProjectionMatrix);
	irradianceShader->unbind();

	prefilterShader->bind();
	prefilterShader->setUniform1i("uEnvironmentMap", 0);
	prefilterShader->setUniformMatrix4fv("uProjection", envProjectionMatrix);
	prefilterShader->unbind();

	sphereVAO = new VAO();
	sphereVBO = new VBO(&sphereVertices[0], sphereVertices.size() * sizeof(float));
	sphereIBO = new IBO(&sphereIndices[0], sphereIndices.size() * sizeof(unsigned int));

	sphereVAO->bind();
	sphereVBO->bind();
	sphereIBO->bind();

	sphereVAO->setVertexAttribute(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(0));
	sphereVAO->setVertexAttribute(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	sphereVAO->setVertexAttribute(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	sphereVAO->unbind();
	sphereVBO->unbind();
	sphereIBO->unbind();

	cubeVAO = new VAO();
	cubeVBO = new VBO(cubeVertices, sizeof(cubeVertices));

	cubeVAO->bind();
	cubeVBO->bind();

	cubeVAO->setVertexAttribute(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(0));
	cubeVAO->setVertexAttribute(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	cubeVAO->setVertexAttribute(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	cubeVAO->unbind();
	cubeVBO->unbind();

	quadVAO = new VAO();
	quadVBO = new VBO(quadVertices, sizeof(quadVertices));

	quadVAO->bind();
	quadVBO->bind();

	quadVAO->setVertexAttribute(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0));
	quadVAO->setVertexAttribute(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	quadVAO->unbind();
	quadVBO->unbind();

	equirectangularHDRTex = new Texture("resources/textures/environment/equirectangular_map.hdr", true);
	brdfLUTTex = new Texture(512, 512, GL_RG16F, GL_RG, GL_FLOAT);

	captureFB = new FrameBuffer(CAPTURE_FB_WIDTH, CAPTURE_FB_HEIGHT);

	environmentCM = new CubeMap(512, 512, GL_RGB16F, GL_RGB, GL_FLOAT);
	irradianceCM = new CubeMap(32, 32, GL_RGB16F, GL_RGB, GL_FLOAT);
	prefilterCM = new CubeMap(128, 128, GL_RGB16F, GL_RGB, GL_FLOAT, true);

	// Convert the HDR equirectangular environment map to a cubemap.
	{
		equirectangularToCubemapShader->bind();
		captureFB->bind();
		cubeVAO->bind();
		equirectangularHDRTex->bind(0);

		glViewport(0, 0, 512, 512);

		for (unsigned int i = 0; i < 6; ++i)
		{
			equirectangularToCubemapShader->setUniformMatrix4fv("uView", envViewMatrices[i]);
			captureFB->bindColorBufferToFrameBuffer(environmentCM->getID(), 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		equirectangularHDRTex->unbind();
		cubeVAO->unbind();
		captureFB->unbind();
		equirectangularToCubemapShader->unbind();
	}

	// Solve diffuse integral by convolution to create an irradiance (cube)map.
	{
		irradianceShader->bind();
		captureFB->bind();
		cubeVAO->bind();
		environmentCM->bind(0);

		// No need to resize the framebuffer's depth buffer.

		glViewport(0, 0, 32, 32);

		for (unsigned int i = 0; i < 6; ++i)
		{
			irradianceShader->setUniformMatrix4fv("uView", envViewMatrices[i]);
			captureFB->bindColorBufferToFrameBuffer(irradianceCM->getID(), 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		environmentCM->unbind();
		cubeVAO->unbind();
		captureFB->unbind();
		irradianceShader->unbind();
	}

	// Run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
	{
		prefilterShader->bind();
		captureFB->bind();
		cubeVAO->bind();
		environmentCM->bind(0);

		unsigned int maxMipLevels = 5;

		for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
		{
			float roughness = (float)mip / (float)(maxMipLevels - 1);
			unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
			unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

			prefilterShader->setUniform1f("uRoughness", roughness);
			captureFB->resizeDepthBuffer(mipWidth, mipHeight);

			glViewport(0, 0, mipWidth, mipHeight);

			for (unsigned int i = 0; i < 6; ++i)
			{
				prefilterShader->setUniformMatrix4fv("uView", envViewMatrices[i]);
				captureFB->bindColorBufferToFrameBuffer(prefilterCM->getID(), 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip);
				
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
		}

		environmentCM->unbind();
		cubeVAO->unbind();
		captureFB->unbind();
		prefilterShader->unbind();
	}

	// Generate a 2D LUT from the BRDF equations used.
	{
		brdfShader->bind();
		captureFB->bind();
		quadVAO->bind();
		brdfLUTTex->bind(0);

		captureFB->resizeDepthBuffer(512, 512);
		captureFB->bindColorBufferToFrameBuffer(brdfLUTTex->getID(), 0, GL_TEXTURE_2D);

		glViewport(0, 0, 512, 512);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		brdfLUTTex->unbind();
		quadVAO->unbind();
		captureFB->unbind();
		brdfShader->unbind();
	}

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void renderSphere()
{
	sphereVAO->bind();

	glDrawElements(GL_TRIANGLE_STRIP, sphereIndexCount, GL_UNSIGNED_INT, 0);

	sphereVAO->unbind();
}

void renderCube()
{
	cubeVAO->bind();

	glDrawArrays(GL_TRIANGLES, 0, 36);

	cubeVAO->unbind();
}

void render()
{
	int nRows = 7;
	int nColumns = 7;
	float spacing = 2.5f;

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	pbrShader->bind();

	pbrShader->setUniformMatrix4fv("uProjection", projectionMatrix);
	pbrShader->setUniformMatrix4fv("uView", camera.getViewMatrix());
	pbrShader->setUniform3f("uCameraPos", camera.getPosition());

	for (int n = 0; n < 4; ++n)
	{
		pbrShader->setUniform3f(("uLightPositions[" + std::to_string(n) + "]").c_str(), lightPositions[n]);
		pbrShader->setUniform3f(("uLightColors[" + std::to_string(n) + "]").c_str(), lightColors[n]);
	}

	irradianceCM->bind(0);
	prefilterCM->bind(1);
	brdfLUTTex->bind(2);

	// Rendering materials.
	for (int row = 0; row < nRows; ++row)
	{
		pbrShader->setUniform1f("uMetallic", (float)row / (float)nRows);

		for (int column = 0; column < nColumns; ++column)
		{
			glm::mat4 modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3((column - (nColumns / 2)) * spacing, (row - (nRows / 2)) * spacing, 0.0f));
			
			pbrShader->setUniformMatrix4fv("uModel", modelMatrix);
			pbrShader->setUniformMatrix3fv("uNormalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));

			// We clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off on direct lighting.
			//
			pbrShader->setUniform1f("uRoughness", glm::clamp((float)column / (float)nColumns, 0.05f, 1.0f));
			
			renderSphere();
		}
	}

	pbrShader->unbind();

	// Rendering background.
	environmentShader->bind();

	environmentShader->setUniformMatrix4fv("uProjection", projectionMatrix);
	environmentShader->setUniformMatrix4fv("uView", camera.getViewMatrix());

	environmentCM->bind(0);

	renderCube();

	environmentShader->unbind();
}

int main()
{
	if (!glfwInit())
	{
		std::cout << "Failed to initialize GLFW!" << std::endl;

		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "PBR", NULL, NULL);

	if (!window)
	{
		std::cout << "Failed to create GLFW context/window!" << std::endl;
		glfwTerminate();

		return -1;
	}

	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetKeyCallback(window, keyboardCallback);
	glfwSetCursorPosCallback(window, cursorPositionCallback);
	glfwSetScrollCallback(window, scrollCallback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD!" << std::endl;
		glfwTerminate();

		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); // Set depth function to "less than AND equal" for SKYBOX depth trick.
	
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // Enable seamless cubemap sampling for lower mip levels in the pre-filter map.

	int contextFlags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);

	if (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		std::cout << "OpenGL DEBUG context initialized!" << std::endl;

		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		glDebugMessageCallback(checkGLDebugMessage, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}

	setupApplication();

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());

		DELTA_TIME = currentFrame - LAST_FRAME;
		LAST_FRAME = currentFrame;

		processInput(window);
		render();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;
	WINDOW_ASPECT_RATIO = (float)width / (float)height;

	glViewport(0, 0, width, height);

	projectionMatrix = glm::perspective(glm::radians(FIELD_OF_VIEW), WINDOW_ASPECT_RATIO, 0.1f, 100.0f);
}

void keyboardCallback(GLFWwindow* window, int key, int scanCode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) // Window should close.
	{
		glfwSetWindowShouldClose(window, true);
	}
}

void cursorPositionCallback(GLFWwindow* window, double xPos, double yPos)
{
	float x = (float)xPos;
	float y = (float)yPos;

	if (!CURSOR_ATTACHED)
	{
		CURSOR_POS_X = x;
		CURSOR_POS_Y = y;

		CURSOR_ATTACHED = true;
	}

	float xOffset = x - CURSOR_POS_X;
	float yOffset = CURSOR_POS_Y - y;

	CURSOR_POS_X = x;
	CURSOR_POS_Y = y;

	xOffset *= CAMERA_SENSITIVITY;
	yOffset *= CAMERA_SENSITIVITY;

	camera.processRotation(xOffset, yOffset);
}

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	FIELD_OF_VIEW = FIELD_OF_VIEW - (float)yOffset;
	FIELD_OF_VIEW = std::min(std::max(FIELD_OF_VIEW, 1.0f), 45.0f);

	projectionMatrix = glm::perspective(glm::radians(FIELD_OF_VIEW), WINDOW_ASPECT_RATIO, 0.1f, 100.0f);
}

void processInput(GLFWwindow* window)
{
	float realSpeed = CAMERA_SPEED * DELTA_TIME;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		camera.processTranslation(Camera::Direction::FORWARD, realSpeed);
	}

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		camera.processTranslation(Camera::Direction::BACKWARD, realSpeed);
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		camera.processTranslation(Camera::Direction::RIGHT, realSpeed);
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		camera.processTranslation(Camera::Direction::LEFT, realSpeed);
	}
}
