#include "App.h"

bool App::startup()
{
	setBackgroundColour(0.25f, 0.25f, 0.25f);

	// initialise gizmo primitive counts
	Gizmos::create(10000, 10000, 10000, 10000);

	// create simple camera transforms
	// glm::lookAt(camera position, camera target, camera up)
	m_viewMatrix = glm::lookAt(vec3(0,0,10), vec3(0), vec3(0, 1, 0));
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
		getWindowWidth() / (float)getWindowHeight(),
		0.1f, 1000.f);

	client->InitialiseClientConnection();

	// Set client defaults
	client->Data().SetElement("Size", vec3(.2, 1.5, 1.5));
	client->Data().SetElement("Color", vec4(0.45, 0.04, 0.51, 1));

	return true;
}

void App::shutdown()
{
	Gizmos::destroy();
	delete client;
}

void App::update(float deltaTime)
{
	static bool firstSend = true;

	if (client->ID() != -1 && firstSend)
	{
		client->SendClientObject();
		firstSend = false;
	}

	// wipe the gizmos clean for this frame
	Gizmos::clear();

	// quit if we press escape
	aie::Input* input = aie::Input::getInstance();

	vec3 pos = client->Data().GetElement<vec3>("Position");
	vec3 vel = client->Data().GetElement<vec3>("Velocity");
	
	// Store previous position and velocity
	static vec3 oldPosition = pos;
	static vec3 oldVelocity = vel;

	// zeroed in case no keys are pressed
	vel = vec3(0);

	float verticalInput = input->isKeyDown(aie::INPUT_KEY_UP) - input->isKeyDown(aie::INPUT_KEY_DOWN);
	pos.y += 10.f * deltaTime * verticalInput;
	vel.y += 10.f * verticalInput;

	pos.x = (client->ID() % 2 == 0) ? 5.f : -5.f;

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

	// Quit if the player presses 'esc' or server full
	if (input->isKeyDown(aie::INPUT_KEY_ESCAPE) || client->IsServerFull())
	{
		quit();
	}
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

	Gizmos::addCapsule(pos, size.y, size.x, 10, 10, color);

	// Draw other clients bodies
	for (auto otherClient : client->OtherObjects())
	{
		vec3 localPos = otherClient.second.networkData.GetElement<vec3>("LocalPosition");
		vec4 color = otherClient.second.networkData.GetElement<vec4>("Color");
		vec3 size = otherClient.second.networkData.GetElement<vec3>("Size");

		Gizmos::addAABBFilled(localPos, size * 0.5f, color);
	}

	Gizmos::draw(m_projectionMatrix * m_viewMatrix);
}