#pragma once
#include <glm/mat4x4.hpp>

#include <RakPeerInterface.h>
#include "../Server/GameMessages.h"
#include "GameObject.h"
#include <unordered_map>

#include <thread>
#include <functional>

class Client {
public:

	Client();
	~Client();

	void update();

	void SetIP(const char* _ip)					{ IP = _ip; }
	void SetPORT(unsigned short _port)			{ PORT = _port; }
	void SetInterpolation(int _interpolation)	{ m_interpolationType = _interpolation % 3; };
	void SetFPS(int _fps)						{ FPS = _fps; }
	void SetNetworkFrameDelay(int _delay)		{ NETWORKFRAME = _delay; }

	NetworkData& Data() { return m_gameobject.networkData; }
	bool NetworkFrame() { return FRAMECOUNT % NETWORKFRAME == 0; }

	std::unordered_map<int, GameObject> OtherObjects() { return m_otherClientGameObjects; }

	enum Interpolation
	{
		NONE = 0,
		LINEAR,
		COSINE
	};

	void SendClientObject();
	void SendSpawnedObject(vec3 _spawnPos, vec3 _direction, float _velocity, float _lifetime);
	void InitialiseClientConnection();

protected:
	void HandleNetworkMessages();
	void OnSetClientIDPacket(RakNet::Packet* _packet);
	void OnReceivedClientDataPacket(RakNet::Packet* _packet);
	void OnClientDisconnect();
	void OnReceivedClientDisconnect(RakNet::Packet* _packet);
	void OnDespawn(RakNet::Packet* _packet);

	// Interpolation methods
	int m_interpolationType = Interpolation::LINEAR;
	void Interpolation_None(GameObject& _gameObject);
	void Interpolation_Linear(GameObject& _gameObject, float _dt);
	void Interpolation_Cosine(GameObject& _gameObject, float _dt);

	RakNet::RakPeerInterface* m_pPeerInterface = nullptr;
	const char* IP = "127.0.0.1";
	unsigned short PORT = 5456;

	GameObject m_gameobject;
	glm::vec3 m_facing;

	std::unordered_map<int, GameObject> m_otherClientGameObjects;

	glm::mat4 m_viewMatrix;
	glm::mat4 m_projectionMatrix;

	int FRAMECOUNT = -1; // -1 so first frame is 0
	int NETWORKFRAME = 3; // frame gap between sending network data
	int FPS = 60;

	bool m_shouldUpdate = true;
	std::thread m_updateThread;
};