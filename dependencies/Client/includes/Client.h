#pragma once
#include <glm/mat4x4.hpp>

#include <RakPeerInterface.h>
#include "../Server/GameMessages.h"
#include "GameObject.h"
#include <unordered_map>

#include <functional>

class Client {
public:

	Client();
	~Client();

	void update(float _dt);

	void SetIP(const char* _ip)					{ IP = _ip; }
	void SetPORT(unsigned short _port)			{ PORT = _port; }
	void SetInterpolation(int _interpolation)	{ m_interpolationType = _interpolation % 3; };
	void SetNetworkDelay(float _seconds)		
	{ 
		m_networkDelay = _seconds;
		m_storedDelay = m_networkDelay;
	}

	NetworkData& Data() { return m_gameobject.networkData; }
	bool NetworkFrame() { return m_networkDelay <= 0; }

	int ID() { return m_gameobject.id; }
	bool IsConnected() 
	{ 
		if (m_pPeerInterface == nullptr)
			return false;

		auto state = m_pPeerInterface->GetConnectionState(m_pPeerInterface->GetGUIDFromIndex(0));
		return state == RakNet::IS_CONNECTED;
	}
	bool IsServerFull() { return m_serverFull; }

	std::unordered_map<int, GameObject> OtherObjects() { return m_otherClientGameObjects; }

	enum Interpolation
	{
		NONE = 0,
		LINEAR,
		COSINE
	};

	void SendClientObject();
	void SendGameObject(GameObject _gameObject);
	void InitialiseClientConnection();
	void AddOnReceiveCall(int _id, std::function<void(GameObject&)> _fn);

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
	std::unordered_map<int, std::function<void(GameObject&)>> m_onReceivedFunctions;
	std::unordered_map<int, float> m_otherObjectTs;

	glm::mat4 m_viewMatrix;
	glm::mat4 m_projectionMatrix;

	float m_networkDelay = 0.1f;
	float m_storedDelay = m_networkDelay;

	bool m_serverFull = false;
};