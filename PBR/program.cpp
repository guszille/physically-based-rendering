// PBR.

#include <vector>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "sources/graphics/vao.h"
#include "sources/graphics/vbo.h"
#include "sources/graphics/ibo.h"
#include "sources/graphics/shader.h"

#include "sources/utils/camera.h"

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

ShaderProgram* shader;

VAO* sphereVAO;
VBO* sphereVBO;
IBO* sphereIBO;

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

unsigned int indexCount = 0;

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
	std::vector<float> vertices;
	std::vector<unsigned int> indices;

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
				indices.push_back((y) * (X_SEGMENTS + 1) + x);
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else // Odd rows.
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				indices.push_back((y) * (X_SEGMENTS + 1) + x);
			}
		}
	}

	indexCount = static_cast<unsigned int>(indices.size());

	for (unsigned int i = 0; i < positions.size(); ++i)
	{
		vertices.push_back(positions[i].x);
		vertices.push_back(positions[i].y);
		vertices.push_back(positions[i].z);

		if (normals.size() > 0)
		{
			vertices.push_back(normals[i].x);
			vertices.push_back(normals[i].y);
			vertices.push_back(normals[i].z);
		}

		if (uvs.size() > 0)
		{
			vertices.push_back(uvs[i].x);
			vertices.push_back(uvs[i].y);
		}
	}

	shader = new ShaderProgram("sources/shaders/vertex_shader.glsl", "sources/shaders/fragment_shader.glsl");

	shader->bind();

	shader->setUniform1f("uAO", 1.0f);

	shader->unbind();

	sphereVAO = new VAO();
	sphereVBO = new VBO(&vertices[0], vertices.size() * sizeof(float));
	sphereIBO = new IBO(&indices[0], indices.size() * sizeof(unsigned int));

	sphereVAO->bind();
	sphereVBO->bind();
	sphereIBO->bind();

	sphereVAO->setVertexAttribute(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(0));
	sphereVAO->setVertexAttribute(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	sphereVAO->setVertexAttribute(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	sphereVAO->unbind();
	sphereVBO->unbind();
	sphereIBO->unbind();
}

void renderSphere()
{
	sphereVAO->bind();

	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);

	sphereVAO->unbind();
}

void render()
{
	int tableDims[2] = { 7, 7 };
	float spacing = 2.5f;

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->bind();

	shader->setUniformMatrix4fv("uProjection", projectionMatrix);
	shader->setUniformMatrix4fv("uView", camera.getViewMatrix());
	shader->setUniform3f("uCameraPos", camera.getPosition());

	for (int n = 0; n < 4; ++n)
	{
		shader->setUniform3f(("uLightPositions[" + std::to_string(n) + "]").c_str(), lightPositions[n]);
		shader->setUniform3f(("uLightColors[" + std::to_string(n) + "]").c_str(), lightColors[n]);
	}

	shader->setUniform3f("uAlbedo", 0.5f, 0.0f, 0.0f);

	// Rendering materials.
	for (int row = 0; row < tableDims[0]; ++row)
	{
		shader->setUniform1f("uMetallic", (float)row / (float)tableDims[0]);

		for (int column = 0; column < tableDims[1]; ++column)
		{
			glm::mat4 modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3((column - (tableDims[1] / 2)) * spacing, (row - (tableDims[0] / 2)) * spacing, 0.0f));
			
			shader->setUniformMatrix4fv("uModel", modelMatrix);
			shader->setUniformMatrix3fv("uNormalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));
			
			// We clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off on direct lighting.
			//
			shader->setUniform1f("uRoughness", glm::clamp((float)column / (float)tableDims[1], 0.05f, 1.0f));
			
			renderSphere();
		}
	}

	shader->setUniform3f("uAlbedo", 1.0f, 1.0f, 1.0f);

	// Rendering light sources.
	for (int n = 0; n < 4; ++n)
	{
		glm::mat4 modelMatrix = glm::mat4(1.0f);

		modelMatrix = glm::translate(modelMatrix, lightPositions[n]);
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f));

		shader->setUniformMatrix4fv("uModel", modelMatrix);
		shader->setUniformMatrix3fv("uNormalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));

		renderSphere();
	}

	shader->unbind();
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
