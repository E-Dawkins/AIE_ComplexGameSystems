#pragma once

#include "Application.h"
#include <glm/mat4x4.hpp>

#include <RakPeerInterface.h>
#include "../Server/GameMessages.h"
#include "../Server/GameObject.h"
#include <unordered_map>

class Client : public aie::Application {
public:

	Client();
	virtual ~Client();

	virtual bool startup();
	virtual void shutdown();

	virtual void update(float deltaTime);
	virtual void draw();

protected:
	void HandleNetworkConnections();
	void InitialiseClientConnection();
	void HandleNetworkMessages();
	void OnSetClientIDPacket(RakNet::Packet* _packet);
	void SendClientGameObject();
	void OnReceivedClientDataPacket(RakNet::Packet* _packet);
	void OnClientDisconnect();
	void OnReceivedClientDisconnect(RakNet::Packet* _packet);
	void SendSpawnBulletPacket();
	void OnDespawn(RakNet::Packet* _packet);

	RakNet::RakPeerInterface* m_pPeerInterface;
	const char* IP = "127.0.0.1";
	const unsigned short PORT = 5456;

	GameObject m_gameobject;
	glm::vec3 m_facing;

	std::unordered_map<int, GameObject> m_otherClientGameObjects;

	glm::mat4	m_viewMatrix;
	glm::mat4	m_projectionMatrix;
};