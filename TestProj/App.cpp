#include "App.h"

bool App::startup()
{
	setBackgroundColour(0.25f, 0.25f, 0.25f);

	// initialise gizmo primitive counts
	Gizmos::create(10000, 10000, 10000, 10000);

	m_windowSize = vec2(getWindowWidth(), getWindowHeight());

	// create simple camera transforms
	// glm::lookAt(camera position, camera target, camera up)
	m_viewMatrix = glm::lookAt(vec3(0,0,10), vec3(0), vec3(0, 1, 0));
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
		getWindowWidth() / (float)getWindowHeight(),
		0.1f, 1000.f);

	client->InitialiseClientConnection();

	// Set client defaults
	client->Data().SetElement("Size", vec3(.2f, 1.5f, 1.5f));
	client->Data().SetElement("Color", vec4(0.45f, 0.04f, 0.51f, 1.f));

	return true;
}

void App::shutdown()
{
	Gizmos::destroy();
	delete client;
}

void App::update(float deltaTime)
{
	if (client->ID() != -1 && m_firstSend)
	{
		OnFirstSend();
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

	pos.x = (client->ID() == 1) ? -5.f : 5.f;

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

	CheckPaddleCollision();
	CheckScreenCollision();

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

		if (otherClient.first == 1000) // the ball
			Gizmos::addSphere(localPos, glm::length(size), 8, 8, color);
		
		else Gizmos::addCapsule(localPos, size.y, size.x, 8, 8, color);
	}

	Gizmos::draw(m_projectionMatrix * m_viewMatrix);
}

// This runs on the first frame that client has been set an id
void App::OnFirstSend()
{
	client->SendClientObject();
	m_firstSend = false;

	// If player 2, then spawn the ball
	if (client->ID() == 2)
	{
		GameObject ball = GameObject();
		ball.networkData.SetElement("Size", vec3(0.1));
		ball.networkData.SetElement("Color", vec4(1));
		ball.networkData.SetElement("Velocity", vec3(0.94f, 0.342f, 0) * 3.f);
		ball.lifeDecays = false;
		client->SendGameObject(ball);
	}
}

void App::CheckPaddleCollision()
{
	GameObject ball = client->OtherObjects()[1000];
	vec3 ballPos = ball.networkData.GetElement<vec3>("LocalPosition");

	vec3 clientPos = client->Data().GetElement<vec3>("Position");
	vec3 clientExtents = client->Data().GetElement<vec3>("Size") * 0.5f;

	// Check if ball's position is within paddle x/y extents
	if (ballPos.x >= clientPos.x - clientExtents.x && ballPos.x <= clientPos.x + clientExtents.x &&
		ballPos.y >= clientPos.y - clientExtents.y && ballPos.y <= clientPos.y + clientExtents.y)
	{
		vec3 vel = ball.networkData.GetElement<vec3>("Velocity");

		vec3 signBall = glm::sign(vel);
		vec3 signPos = glm::sign(clientPos - ballPos);

		// If ball going towards paddle, flip velocity-x
		if (signBall.x == signPos.x)
		{
			vel.x *= -1.f;

			ball.networkData.SetElement("Velocity", vel);
			ball.networkData.SetElement("Position", ballPos);
			client->SendGameObject(ball);
		}
	}
}

void App::CheckScreenCollision()
{
	GameObject ball = client->OtherObjects()[1000];
	vec3 ballPos = ball.networkData.GetElement<vec3>("LocalPosition");
	vec3 ballVel = ball.networkData.GetElement<vec3>("Velocity");
	vec3 oldBallVel = ballVel;
	vec3 signBall = glm::sign(ballVel);
	vec2 windowPos = ToWindowPos(ballPos);

	int width = getWindowWidth();
	int height = getWindowHeight();

	// If ball y pos outside screen height, flip velocity y
	if ((windowPos.y < 0 && signBall.y < 0) ||
		(windowPos.y > height && signBall.y > 0))
		ballVel.y *= -1;

	// Same for ball x pos / velocity x
	if ((windowPos.x < 0 && signBall.x < 0) ||
		(windowPos.x > width && signBall.x > 0))
		ballVel.x *= -1;

	if (ballVel != oldBallVel) // velocity has been changed, send network data
	{
		ball.networkData.SetElement("Velocity", ballVel);
		ball.networkData.SetElement("Position", ballPos);
		client->SendGameObject(ball);
	}
}
