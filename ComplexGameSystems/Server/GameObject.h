#pragma once
#include <glm/ext.hpp>
#include <RakPeerInterface.h>

class GameObject
{
public:
	glm::vec3 position;
	glm::vec4 color;
	int id;

	void Write(RakNet::RakPeerInterface* _pPeerInterface,
		const RakNet::SystemAddress& _address, bool _broadcast);
	void Read(RakNet::Packet* _packet);
};

