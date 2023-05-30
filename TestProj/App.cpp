#include "App.h"

bool App::startup()
{
	setBackgroundColour(0.25f, 0.25f, 0.25f);

	// initialise gizmo primitive counts
	Gizmos::create(10000, 10000, 10000, 10000);

	m_2dRenderer = new aie::Renderer2D();
	m_font = new aie::Font("./font/consolas.ttf", 50);
	m_fontHalf = new aie::Font("./font/consolas.ttf", 25);

	m_windowSize = vec2(getWindowWidth(), getWindowHeight());

	// create simple camera transforms
	m_viewMatrix = glm::lookAt(vec3(0,0,10), vec3(0), vec3(0, 1, 0));
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
		getWindowWidth() / (float)getWindowHeight(),
		0.1f, 1000.f);

	client->InitialiseClientConnection();

	// Set client defaults
	client->Data().SetElement("Size", vec3(.2f, 1.5f, 1.5f));
	client->Data().SetElement("Color", vec4(0.45f, 0.04f, 0.51f, 1.f));
	client->Data().SetElement("Score", 0);
	client->Data().SetElement("Ready", false);

	client->AddOnReceiveCall(1000, [this](GameObject& _gO) {OnBallReceived(_gO); });

	return true;
}

void App::shutdown()
{
	Gizmos::destroy();
	delete client;
	delete m_2dRenderer;
	delete m_font;
	delete m_fontHalf;
}

void App::update(float deltaTime)
{
	// Update client
	client->update(deltaTime);

	// Don't update if a player has already won, or id not set yet
	if (client->ID() == -1)
		return;

	GameSetup(deltaTime);
	CheckInput(deltaTime);

	// Only check collision if the ball has been spawned
	if (client->OtherObjects().contains(1000))
	{
		CheckPaddleCollision();
		CheckScreenCollision();
	}

	// Check if either player has won
	if (m_gameStart)
	{
		int clientScore = client->Data().GetElement<int>("Score");
		int otherScore = client->OtherData(3 - client->ID()).GetElement<int>("Score");

		if (clientScore >= m_maxScore)
			m_winner = client->ID();

		else if (otherScore >= m_maxScore)
			m_winner = 3 - client->ID();

		// If someone has won, do something
		if (m_winner != -1)
		{
			GameObject& ball = client->OtherObjects()[1000];
			ball.networkData.SetElement("Velocity", vec3(0));
			client->SendGameObject(ball);
		}
	}

	// Only send data on a network frame, yes this does send
	// all the data every network frame i know, bad for network
	if (client->NetworkFrame())
		client->SendClientObject();
}

void App::draw()
{
	// Don't draw if id not set yet
	if (client->ID() == -1)
		return;

	// wipe the screen to the background colour
	clearScreen();

	// wipe the gizmos clean for this frame
	Gizmos::clear();

	// update perspective in case window resized
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
		getWindowWidth() / (float)getWindowHeight(),
		0.1f, 1000.f);

	DrawScene();
	DrawSceneUI();
	DrawClientUI(client->Data());

	if (client->OtherObjects().contains(3 - client->ID()))
		DrawClientUI(client->OtherData(3 - client->ID()));
	
	if (m_winner != -1) // game is over, draw win screen
		DrawWinUI();

	Gizmos::draw(m_projectionMatrix * m_viewMatrix);
}

#pragma region Logic

void App::GameSetup(float _dt)
{
	// Check if both clients are ready, then spawn the ball
	if (BothReady() && !m_gameStart)
	{
		m_gameStartTimer -= _dt;

		if (m_gameStartTimer <= 0.f)
		{
			SpawnBall();
			m_gameStart = true;
			m_gameStartTimer = m_storedGameStartTimer;
		}
	}
}

void App::SpawnBall()
{
	GameObject ball = GameObject();
	ball.networkData.SetElement("Size", vec3(0.1));
	ball.networkData.SetElement("Color", vec4(1));
	ball.networkData.SetElement("Velocity", vec3(glm::circularRand(3.f), 0));
	ball.lifeDecays = false;
	ball.id = 1000; // manually setting it is not advised, server will usually handle this
	client->SendGameObject(ball);
}

void App::CheckInput(float _dt)
{
	aie::Input* input = aie::Input::getInstance();

	// Update position and velocity of this frame
	vec3 vel = vec3(0);
	vec3 pos = client->Data().GetElement<vec3>("Position");
	pos.x = (client->ID() % 2 == 0) ? 5.f : -5.f;

	float verticalInput = input->isKeyDown(aie::INPUT_KEY_UP) - input->isKeyDown(aie::INPUT_KEY_DOWN);
	pos.y += 10.f * _dt * verticalInput;
	vel.y = 10.f * verticalInput;

	client->Data().SetElement("Position", pos);
	client->Data().SetElement("Velocity", vel);

	// Update ready status when player presses space
	if (client->Data().GetElement<bool>("Ready") == false 
		&& input->wasKeyPressed(aie::INPUT_KEY_SPACE))
	{
		client->Data().SetElement("Ready", true);
	}

	// Game ended, check if they want to play again
	if (m_winner != -1 && input->wasKeyPressed(aie::INPUT_KEY_R))
	{
		m_winner = -1;
		m_gameStart = false;
		client->Data().SetElement("Ready", false);
		client->Data().SetElement("Score", 0);
	}

	// Quit if the player presses 'esc' or server full
	if (input->isKeyDown(aie::INPUT_KEY_ESCAPE) || client->IsServerFull())
	{
		quit();
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
	GameObject& ball = client->OtherObjects()[1000];
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
	if ((windowPos.x < 0 && client->ID() == 2 && signBall.x < 0) ||
		(windowPos.x > width && client->ID() == 1 && signBall.x > 0))
	{
		if (m_canSetScore)
		{
			ballVel = vec3(glm::circularRand(3.f), 0);
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
		ball.networkData.SetElement("LocalPosition", ballPos);
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

#pragma endregion Logic

#pragma region Visuals

void App::DrawScene()
{
	// Draw our body
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
}

void App::DrawSceneUI()
{
	m_2dRenderer->begin();

	if (BothReady() && !m_gameStart)
	{
		int timer = std::ceil(m_gameStartTimer);
		std::string timerStr = std::to_string(timer);
		float textOffset = m_2dRenderer->measureTextWidth(m_font, timerStr.c_str());

		m_2dRenderer->setRenderColour(1, 1, 1);
		m_2dRenderer->drawText(m_font, timerStr.c_str(),
			m_windowSize.x * 0.5f, m_windowSize.y * 0.75f);
	}

	m_2dRenderer->end();
}

void App::DrawClientUI(NetworkData _data)
{
	vec2 wPos = ToWindowPos(_data.GetElement<vec3>("Position"));
	vec2 wSize = ToWindowSize(_data.GetElement<vec3>("Size")) * 0.5f;

	m_2dRenderer->begin();

	m_2dRenderer->setRenderColour(1, 1, 1);
	
	int score = _data.GetElement<int>("Score");
	m_2dRenderer->drawText(m_font, std::to_string(score).c_str(), wPos.x, m_windowSize.y - 75);

	if (!m_gameStart)
	{
		bool ready = _data.GetElement<bool>("Ready");
		
		if (ready) m_2dRenderer->setRenderColour(0, 1, 0);
		else m_2dRenderer->setRenderColour(1, 0, 0);

		m_2dRenderer->drawCircle(wPos.x, wPos.y + wSize.y + 10, wSize.x);
	}

	m_2dRenderer->end();
}

void App::DrawWinUI()
{
	m_2dRenderer->begin();

	m_2dRenderer->setRenderColour(0, 0, 0);
	m_2dRenderer->drawBox(m_windowSize.x * .5f, m_windowSize.y * .5f, 400, 100);
	m_2dRenderer->drawBox(m_windowSize.x * .5f, m_windowSize.y * .5f - 50, 200, 50);

	std::string temp = "Player " + std::to_string(m_winner) + " Won !";

	const char* text = temp.c_str();
	const char* text2 = "'r' to replay";

	float tOffset = m_2dRenderer->measureTextWidth(m_font, text);
	float tOffset2 = m_2dRenderer->measureTextWidth(m_fontHalf, text2);

	m_2dRenderer->setRenderColour(1, 1, 1);
	m_2dRenderer->drawText(m_font, text, (m_windowSize.x - tOffset) * .5f, m_windowSize.y * .5f - 15);
	m_2dRenderer->drawText(m_fontHalf, text2, (m_windowSize.x - tOffset2) * .5f, m_windowSize.y * .5f - 65);

	m_2dRenderer->end();
}

#pragma endregion Visuals