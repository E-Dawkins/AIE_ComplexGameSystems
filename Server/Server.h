#pragma once
#include "RakPeerInterface.h"
#include "../Client/GameObject.h"
#include <unordered_map>

class Server
{
public:
	void Run();
	void SetMaxConnections(const unsigned int _max) { MAXCONNECTIONS = _max; }

protected:
	void HandleNetworkMessages();
	void SendClientPing(const char* _message);
	void SendNewClientID(RakNet::SystemAddress& _address);
	void ClientDisconnect(RakNet::Packet* _packet);
	void OnReceivedClientData(RakNet::Packet* _packet);

	void OnSpawnGameObject(RakNet::Packet* _packet);
	void SpawnObject(glm::vec3 _position, glm::vec3 _velocity, glm::vec3 _size);
	void Despawn(int _id);

	float GetElapsedTime();
	void UpdateObjects();


	RakNet::RakPeerInterface* m_pPeerInterface;

	std::unordered_map<int, GameObject> m_gameObjects;
	int m_nextClientID = 1;
	int m_nextServerID = 1000;

	unsigned int MAXCONNECTIONS = 32;
	const unsigned short PORT = 5456;
};

