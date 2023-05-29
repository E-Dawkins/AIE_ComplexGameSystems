#pragma once
#include "Application.h"
#include "Gizmos.h"
#include "Input.h"
#include "Client.h"
#include "Renderer2D.h"
#include "Font.h"
#include <string>
#include <glm/ext.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>

using glm::vec2;
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
	aie::Renderer2D* m_2dRenderer = nullptr;
	aie::Font* m_font = nullptr;
	aie::Font* m_fontHalf = nullptr;

	mat4 m_viewMatrix;
	mat4 m_projectionMatrix;
	vec2 m_windowSize;

	bool m_firstSend = true;
	bool m_canSetScore = true;
	int m_winner = -1;
	int m_maxScore = 1;

	// Logic functions
	void OnFirstSend();
	void CheckInput(float _dt);
	void CheckPaddleCollision();
	void CheckScreenCollision();
	void OnBallReceived(GameObject& _gameobject);

	// Visuals functions
	void DrawScene();
	void DrawUI();
	void DrawWinUI();

	// Helper functions
	vec2 ToWindowPos(glm::vec3 _worldPos)
	{
		vec4 clipPos = m_projectionMatrix * (m_viewMatrix * vec4(_worldPos, 1.0));;
		vec3 ndcPos = vec3(clipPos.x, clipPos.y, clipPos.z) / clipPos.w;
		return ((vec2(ndcPos.x, ndcPos.y) + 1.f) / 2.f) * m_windowSize;
	}
};