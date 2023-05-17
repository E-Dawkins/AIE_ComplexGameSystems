#pragma once
#include "Application.h"
#include <glm/mat4x4.hpp>

#include <RakPeerInterface.h>
#include "../Server/GameMessages.h"
#include "GameObject.h"
#include <unordered_map>

class Client : public aie::Application {
public:

	Client() = default;
	virtual ~Client() = default;

	virtual bool startup();
	virtual void shutdown();

	virtual void update(float deltaTime);
	virtual void draw();

	void SetIP(const char* _ip)					{ IP = _ip; }
	void SetPORT(unsigned short _port)			{ PORT = _port; }
	void SetInterpolation(int _interpolation)	{ m_interpolationType = _interpolation % 3; };
	void SetFPS(int _fps)						{ FPS = _fps; }
	void SetNetworkFrameDelay(int _delay)		{ NETWORKFRAME = _delay; }

	enum Interpolation
	{
		NONE = 0,
		LINEAR,
		COSINE
	};

protected:
	void InitialiseClientConnection();
	void HandleNetworkMessages();
	void OnSetClientIDPacket(RakNet::Packet* _packet);
	void SendClientGameObject();
	void OnReceivedClientDataPacket(RakNet::Packet* _packet);
	void OnClientDisconnect();
	void OnReceivedClientDisconnect(RakNet::Packet* _packet);
	void SendSpawnBulletPacket();
	void OnDespawn(RakNet::Packet* _packet);

	// Interpolation methods
	int m_interpolationType = Interpolation::LINEAR;

	void Interpolation_None(GameObject& _gameObject);
	void Interpolation_Linear(GameObject& _gameObject, float _dt);
	void Interpolation_Cosine(GameObject& _gameObject, float _dt);

	RakNet::RakPeerInterface* m_pPeerInterface;
	const char* IP = "127.0.0.1";
	unsigned short PORT = 5456;

	GameObject m_gameobject;
	glm::vec3 m_facing;

	std::unordered_map<int, GameObject> m_otherClientGameObjects;

	glm::mat4	m_viewMatrix;
	glm::mat4	m_projectionMatrix;

	int FRAMECOUNT = -1; // -1 so first frame is 0
	int NETWORKFRAME = 3; // frame gap between sending network data
	int FPS = 60;
};