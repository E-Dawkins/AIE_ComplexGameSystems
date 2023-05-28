#include "App.h"

bool App::startup()
{
	setBackgroundColour(0.25f, 0.25f, 0.25f);

	// initialise gizmo primitive counts
	Gizmos::create(10000, 10000, 10000, 10000);

	m_2dRenderer = new aie::Renderer2D();
	m_font = new aie::Font("./font/consolas.ttf", 50);

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
	client->Data().SetElement("Score", 1);

	client->AddOnReceiveCall(1000, [this](GameObject& _obj) {App::OnBallReceived(_obj);});

	return true;
}

void App::shutdown()
{
	Gizmos::destroy();
	delete client;
	delete m_2dRenderer;
	delete m_font;
}

void App::update(float deltaTime)
{
	// Update client
	client->update(deltaTime);

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
	if (oldPosition != pos && client->NetworkFrame())
	{
		client->SendClientObject();
		oldPosition = pos;
	}

	// Only check collision if the ball has been spawned
	if (client->OtherObjects().contains(1000))
	{
		CheckPaddleCollision();
		CheckScreenCollision();
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
	for (auto& otherClient : client->OtherObjects())
	{
		vec3 localPos = otherClient.second.networkData.GetElement<vec3>("LocalPosition");
		vec4 color = otherClient.second.networkData.GetElement<vec4>("Color");
		vec3 size = otherClient.second.networkData.GetElement<vec3>("Size");

		if (otherClient.first == 1000) // the ball
			Gizmos::addSphere(localPos, glm::length(size), 8, 8, color);
		
		else Gizmos::addCapsule(localPos, size.y, size.x, 8, 8, color);
	}

	// Draw text / sprites
	m_2dRenderer->begin();

	// Draw client score
	int score = client->Data().GetElement<int>("Score");
	std::string scoreStr = std::to_string(score);
	m_2dRenderer->drawText(m_font, scoreStr.c_str(), ToWindowPos(pos).x, getWindowHeight() - 75);

	// Draw other client score
	score = client->OtherObjects()[3 - client->ID()].networkData.GetElement<int>("Score");
	scoreStr = std::to_string(score);
	m_2dRenderer->drawText(m_font, scoreStr.c_str(), ToWindowPos(-pos).x, getWindowHeight() - 75);

	m_2dRenderer->end();

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

	// Same for x, only for side client is on and update their score
	if ((windowPos.x < 0 && client->ID() == 1 && signBall.x < 0) ||
		(windowPos.x > width && client->ID() == 2 && signBall.x > 0))
	{
		if (m_canSetScore)
		{
			ballVel.x *= -1;
			ballPos = vec3(0);
			int score = client->Data().GetElement<int>("Score") + 1;
			client->Data().SetElement("Score", score);
			client->SendClientObject();
			m_canSetScore = false;
		}
	}

	if (ballVel != oldBallVel) // velocity has been changed, send network data
	{
		ball.networkData.SetElement("Velocity", ballVel);
		ball.networkData.SetElement("Position", ballPos);
		client->SendGameObject(ball);
	}
}

void App::OnBallReceived(GameObject& _gameobject)
{
	if (_gameobject.networkData.GetElement<vec3>("Position") == vec3(0))
	{
		m_canSetScore = true;
	}
}
