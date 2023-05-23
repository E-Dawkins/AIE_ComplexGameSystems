#pragma once
#include "Application.h"
#include <glm/ext.hpp>
#include <glm/mat4x4.hpp>
#include "Gizmos.h"
#include "Input.h"
#include <iostream>

#include "Client.h"

using glm::vec3;
using glm::vec4;
using glm::mat4;
using aie::Gizmos;

class App : public aie::Application {
public:

	App() = default;
	~App() = default;

	virtual bool startup();
	virtual void shutdown();

	virtual void update(float deltaTime);
	virtual void draw();

	Client* client = new Client();

protected:
	mat4 m_viewMatrix;
	mat4 m_projectionMatrix;
};