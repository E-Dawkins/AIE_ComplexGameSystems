#pragma once
#include "Application.h"
#include <glm/ext.hpp>
#include <glm/mat4x4.hpp>
#include "Client.h"
#include "Gizmos.h"
#include "Input.h"

using glm::vec3;
using glm::mat4;
using aie::Gizmos;

class ClientApp : public aie::Application
{
public:
	ClientApp();
	~ClientApp();

	virtual bool startup();
	virtual void shutdown();

	virtual void update(float deltaTime);
	virtual void draw();

	Client* client = new Client();

protected:
	vec3 m_facing = vec3(1, 0, 0);

	mat4 m_viewMatrix;
	mat4 m_projectionMatrix;
};