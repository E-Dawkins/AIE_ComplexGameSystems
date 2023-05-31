#pragma once
#include "Application.h"
#include "Gizmos.h"
#include "Input.h"
#include "Client.h"
#include "Renderer2D.h"
#include "Font.h"
#include <Texture.h>
#include <glm/ext.hpp>
#include <glm/mat4x4.hpp>

using glm::vec2;
using glm::vec3;
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
	aie::Renderer2D* m_2dRenderer = nullptr;
	aie::Font* m_font = nullptr;
	aie::Font* m_fontHalf = nullptr;
	aie::Font* m_font2Thirds = nullptr;
	aie::Texture* m_background = nullptr;
	aie::Texture* m_ballTex = nullptr;
	aie::Texture* m_paddleTex = nullptr;

	mat4 m_viewMatrix;
	mat4 m_projectionMatrix;

	int m_winner = -1;
	bool m_gameStart = false;
	float m_gameStartTimer = 3.f;
	float m_ballSpeed = 5.f;

	// Logic functions
	void GameSetup(float _dt);
	void SpawnBall();
	void CheckInput(float _dt);
	void CheckPaddleCollision();
	void CheckScreenCollision();
	void CheckWinState();

	// Visuals functions
	void DrawScene();
	void DrawSceneUI();
	void DrawClientUI(NetworkData _data, const char* _posKey, bool _isClient = true);
	void DrawWinUI();

	// Helper functions
	vec2 ToWindowPos(glm::vec3 _worldPos)
	{
		vec4 clipPos = m_projectionMatrix * (m_viewMatrix * vec4(_worldPos, 1.0));
		vec3 ndcPos = vec3(clipPos.x, clipPos.y, clipPos.z) / clipPos.w;
		vec2 windowSize = vec2(getWindowWidth(), getWindowHeight());
		return ((vec2(ndcPos.x, ndcPos.y) + 1.f) * 0.5f) * windowSize;
	}

	vec3 ToWorldPos(glm::vec2 _windowPos)
	{
		vec2 windowSize = vec2(getWindowWidth(), getWindowHeight());

	}

	vec2 ToWindowSize(glm::vec3 _worldPos)
	{
		vec4 clipPos = m_projectionMatrix * (m_viewMatrix * vec4(_worldPos, 1.0));
		vec2 windowSize = vec2(getWindowWidth(), getWindowHeight());
		vec3 ndcPos = vec3(clipPos.x, clipPos.y, clipPos.z) / clipPos.w;
		return vec2(ndcPos.x, ndcPos.y) * windowSize * 0.5f;
	}

	bool BothReady()
	{
		bool clientReady = client->Data().GetElement<bool>("Ready");
		bool otherReady = client->OtherData(3 - client->ID()).GetElement<bool>("Ready");

		return clientReady && otherReady;
	}

	vec3 RandomDirection(float _magnitude)
	{
		return vec3(glm::circularRand(_magnitude), 0);
	}
};