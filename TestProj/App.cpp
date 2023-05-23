#include "App.h"

bool App::startup()
{
	setBackgroundColour(0.25f, 0.25f, 0.25f);

	// initialise gizmo primitive counts
	Gizmos::create(10000, 10000, 10000, 10000);

	// create simple camera transforms
	m_viewMatrix = glm::lookAt(vec3(10), vec3(0), vec3(0, 1, 0));
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
		getWindowWidth() / (float)getWindowHeight(),
		0.1f, 1000.f);

	client->InitialiseClientConnection();

	return true;
}

void App::shutdown()
{
	Gizmos::destroy();
	delete client;
}

void App::update(float deltaTime)
{
	// wipe the gizmos clean for this frame
	Gizmos::clear();

	// quit if we press escape
	aie::Input* input = aie::Input::getInstance();

	vec3 pos = client->Data().GetElement<vec3>("Position");
	vec3 vel = vec3(0); // zeroed in case no keys are pressed

	// Store previous position and velocity
	static vec3 oldPosition = client->Data().GetElement<vec3>("Position");
	static vec3 oldVelocity = client->Data().GetElement<vec3>("Velocity");

	if (input->isKeyDown(aie::INPUT_KEY_LEFT))
	{
		pos.x -= 10.f * deltaTime;
		vel.x = -10;
		m_facing = glm::vec3(-1, 0, 0);
	}
	if (input->isKeyDown(aie::INPUT_KEY_RIGHT))
	{
		pos.x += 10.f * deltaTime;
		vel.x = 10;
		m_facing = glm::vec3(1, 0, 0);
	}
	if (input->isKeyDown(aie::INPUT_KEY_UP))
	{
		pos.z -= 10.f * deltaTime;
		vel.z = -10;
		m_facing = glm::vec3(0, 0, -1);
	}
	if (input->isKeyDown(aie::INPUT_KEY_DOWN))
	{
		pos.z += 10.f * deltaTime;
		vel.z = 10;
		m_facing = glm::vec3(0, 0, 1);
	}

	client->Data().SetElement("Position", pos);
	client->Data().SetElement("Velocity", vel);

	// Only send a network message when we change
	// our movement state, and it is a network frame
	if ((oldVelocity != vel || oldPosition != pos) && client->NetworkFrame())
	{
		client->SendClientObject();
		oldPosition = pos;
		oldVelocity = vel;
	}

	if (input->wasKeyPressed(aie::INPUT_KEY_SPACE))
		client->SendSpawnedObject(pos, m_facing, 3, 5);

	if (input->isKeyDown(aie::INPUT_KEY_ESCAPE))
		quit();
}

void App::draw()
{
	// wipe the screen to the background colour
	clearScreen();

	// update perspective in case window resized
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
		getWindowWidth() / (float)getWindowHeight(),
		0.1f, 1000.f);

	// Draw body
	vec3 pos = client->Data().GetElement<vec3>("Position");
	vec4 color = client->Data().GetElement<vec4>("Color");
	vec3 size = client->Data().GetElement<vec3>("Size");

	Gizmos::addSphere(pos, glm::length(size), 8, 8, color);

	// Draw other clients bodies
	for (auto otherClient : client->OtherObjects())
	{
		vec3 localPos = otherClient.second.networkData.GetElement<vec3>("LocalPosition");
		vec4 color = otherClient.second.networkData.GetElement<vec4>("Color");
		vec3 size = otherClient.second.networkData.GetElement<vec3>("Size");

		Gizmos::addSphere(localPos, glm::length(size), 8, 8, color);
	}

	Gizmos::draw(m_projectionMatrix * m_viewMatrix);
}