#include "GameObject.h"
#include <BitStream.h>
#include "../Server/GameMessages.h"

// Store default data into gameobject network data
GameObject::GameObject()
{
	networkData.Insert("Color", vec4());
	networkData.Insert("Position", vec3());
	networkData.Insert("LocalPosition", vec3());
	networkData.Insert("Velocity", vec3());
	networkData.Insert("Size", vec3());
}

GameObject::~GameObject() = default;

void GameObject::Write(RakNet::RakPeerInterface* _pPeerInterface, const RakNet::SystemAddress& _address, bool _broadcast)
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_CLIENT_CLIENT_DATA);
	bs.Write(id);
	
	// Write out amount of data elements
	bs.Write(networkData.Size());

	// Foreach data element, write out...
	for (const auto &i : networkData.Data())
	{
		// ...the name of the element...
		RakNet::RakString key = i.first;
		bs.Write(key);

		// ...and the bytes for each element
		bs.Write(i.second.size());

		for (auto byte : i.second)
		{
			bs.Write(byte);
		}
	}

	_pPeerInterface->Send(&bs, HIGH_PRIORITY, 
		RELIABLE_ORDERED, 0, _address, _broadcast);
}

void GameObject::Read(RakNet::Packet* _packet)
{
	RakNet::BitStream bsIn(_packet->data, _packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
	bsIn.Read(id);

	// Read in amount of data elements
	int dataAmount = 0;
	bsIn.Read(dataAmount);

	// Foreach data element read in...
	for (int i = 0; i < dataAmount; i++)
	{
		// ...the name of the element and...
		RakNet::RakString key;
		bsIn.Read(key);

		// ...the bytes into a vector...
		size_t byteCount;
		bsIn.Read(byteCount);

		std::vector<unsigned char> bytes;
		bytes.resize(byteCount);

		for (int j = 0; j < (int)byteCount; j++)
		{
			bsIn.Read((char*)&bytes[j], sizeof(unsigned char));
		}
		
		// ...then overwrite stored elements
		networkData.SetElementBytes(key.C_String(), bytes);
	}
}

// Update the gameobject's position based on its' velocity
void GameObject::Update(float _deltaTime)
{
	vec3 pos = networkData.GetElement<vec3>("Position");
	pos += networkData.GetElement<vec3>("Velocity") * _deltaTime;
	networkData.SetElement("Position", pos);
}

// Array of possible gameobject colors
vec4 colors[] = {
	vec4(0.5, 0.5, 0.5, 1),
	vec4(  1,   0,   0, 1),
	vec4(  0,   1,   0, 1),
	vec4(  0,   0,   1, 1),
	vec4(  1,   1,   0, 1),
	vec4(  1,   0,   1, 1),
	vec4(  0,   1,   1, 1),
	vec4(  0,   0,   0, 1)
};

// Get gameobject color based in its id
vec4 GameObject::GetColor(int _id)
{
	return colors[_id % 7];
}
