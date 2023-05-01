#pragma once

#include "Application.h"
#include <glm/mat4x4.hpp>

#include <RakPeerInterface.h>
#include "../Server/GameMessages.h"
#include "../Server/GameObject.h"

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

	RakNet::RakPeerInterface* m_pPeerInterface;
	const char* IP = "127.0.0.1";
	const unsigned short PORT = 5456;

	GameObject m_gameobject;

	glm::mat4	m_viewMatrix;
	glm::mat4	m_projectionMatrix;
};