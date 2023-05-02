#pragma once
#include <glm/ext.hpp>
#include <RakPeerInterface.h>

class GameObject
{
public:
	GameObject();

	glm::vec3 position;
	glm::vec3 localPosition;
	glm::vec3 velocity;
	glm::vec4 color;
	float radius;
	float lifetime;
	int id;

	void Write(RakNet::RakPeerInterface* _pPeerInterface,
		const RakNet::SystemAddress& _address, bool _broadcast);
	void Read(RakNet::Packet* _packet);

	void Update(float _deltaTime);

	static glm::vec4 GetColor(int _id);
};

