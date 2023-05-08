#pragma once
#include <glm/ext.hpp>
#include <RakPeerInterface.h>

#include <vector>
#include <any>

class GameObject
{
public:
	// This data struct serves as a base, and can be overridden
	struct Data
	{
		glm::vec4 color;
		float radius;
		glm::vec3 position;
		glm::vec3 localPosition;
		glm::vec3 velocity;
	} data;

	std::vector<std::any> networkData;

	// Any data that shouldn't be overridable
	int id;
	float lifetime;

	void Write(RakNet::RakPeerInterface* _pPeerInterface,
		const RakNet::SystemAddress& _address, bool _broadcast);
	void Read(RakNet::Packet* _packet);

	void Update(float _deltaTime);

	static glm::vec4 GetColor(int _id);
};

