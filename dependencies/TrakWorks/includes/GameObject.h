#pragma once
#include <glm/ext.hpp>
#include <RakPeerInterface.h>
#include "NetworkData.h"
#include "ClientMessages.h"
#include <vector>

using glm::vec3;
using glm::vec4;

class GameObject
{
public:
	GameObject();
	~GameObject();

	NetworkData networkData;

	// Any data that shouldn't be overridable
	int id = -1;
	float lifetime = 10;
	bool lifeDecays = true;

	void Write(RakNet::RakPeerInterface* _pPeerInterface, const RakNet::SystemAddress& _address, 
		bool _broadcast, RakNet::MessageID _messageID = (RakNet::MessageID)ID_CLIENT_CLIENT_DATA);
	void Read(RakNet::Packet* _packet);

	void Update(float _deltaTime);

	static vec4 GetColor(int _id);
};
