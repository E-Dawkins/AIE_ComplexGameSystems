#include "GameObject.h"
#include <BitStream.h>
#include "../Server/GameMessages.h"
#include <iostream>
#include <StringCompressor.h>

// Store default data into gameobject network data
GameObject::GameObject()
{
	networkData.Insert("Color", vec4());
	networkData.Insert("Position", vec3());
	networkData.Insert("LocalPosition", vec3());
	networkData.Insert("Velocity", vec3());
	networkData.Insert("Size", vec3());
}

GameObject::~GameObject()
{
	networkData.Clear();
}

void GameObject::Write(RakNet::RakPeerInterface* _pPeerInterface, 
	const RakNet::SystemAddress& _address, bool _broadcast, RakNet::MessageID _messageID)
{
	RakNet::BitStream bs;
	bs.Write(_messageID);
	
	// Write other gameobject data
	bs.Write(id);
	bs.Write(lifetime);
	bs.Write(lifeDecays);
	
	// Write out amount of data elements
	bs.Write(networkData.Size());

	// Foreach data element, write out...
	for (int i = 0; i < networkData.Size(); i++)
	{
		// ...the name of the element...
		std::vector<char> key = networkData.StringToVector(networkData.Keys()[i]);
		bs.Write((int)key.size());

		for (auto c : key)
		{
			bs.Write(c);
		}

		// ...and the bytes for each element
		bs.Write((int)networkData.Values()[i].size());

		for (auto byte : networkData.Values()[i])
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
	
	// Read in other game object data
	bsIn.Read(id);
	bsIn.Read(lifetime);
	bsIn.Read(lifeDecays);

	// Read in amount of data elements
	int dataAmount = 0;
	bsIn.Read(dataAmount);

	// Foreach data element read in...
	for (int i = 0; i < dataAmount; i++)
	{
		// ...the name of the element and...
		int charCount;
		bsIn.Read(charCount);

		std::vector<char> key;
		key.resize(charCount);

		for (int j = 0; j < charCount; j++)
		{
			bsIn.Read(key[j]);
		}

		const char* keyStr = networkData.VectorToString(key);

		// ...the bytes into a vector...
		int byteCount;
		bsIn.Read(byteCount);

		std::vector<unsigned char> bytes;
		bytes.resize(byteCount);

		for (int j = 0; j < byteCount; j++)
		{
			bsIn.Read(bytes[j]);
		}
		
		// ...then overwrite stored elements
		networkData.SetElementBytes(keyStr, bytes);
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
