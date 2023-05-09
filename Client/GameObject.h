#pragma once
#include <glm/ext.hpp>
#include <RakPeerInterface.h>
#include "NetworkData.h"

using glm::vec3;
using glm::vec4;

class GameObject
{
public:
	GameObject();

	NetworkData networkData;

	// Any data that shouldn't be overridable
	int id;
	float lifetime;

	void Write(RakNet::RakPeerInterface* _pPeerInterface,
		const RakNet::SystemAddress& _address, bool _broadcast);
	void Read(RakNet::Packet* _packet);

	void Update(float _deltaTime);

	static vec4 GetColor(int _id);
};
