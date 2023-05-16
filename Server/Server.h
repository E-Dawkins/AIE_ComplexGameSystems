#pragma once
#include "RakPeerInterface.h"
#include "../Client/GameObject.h"
#include <unordered_map>

class Server
{
public:
	void Run();

protected:
	void HandleNetworkMessages();
	void SendClientPing(const char* _message);
	void SendNewClientID(RakNet::SystemAddress& _address);
	void ClientDisconnect(RakNet::Packet* _packet);

	void OnSpawnGameObject(RakNet::Packet* _packet);
	void SpawnObject(glm::vec3 _position, glm::vec3 _velocity, float _radius);
	void Despawn(int _id);

	float GetElapsedTime();
	void UpdateObjects();

	RakNet::RakPeerInterface* m_pPeerInterface;

	std::unordered_map<int, GameObject> m_gameObjects;
	int m_nextClientID = 1;

	int m_nextServerID = 1000;
	LARGE_INTEGER m_lastTime;

	const unsigned short PORT = 5456;
};

