#include "GameObject.h"
#include <BitStream.h>
#include "../Server/GameMessages.h"

GameObject::GameObject()
{
	networkData.Insert("Color", vec3());
	networkData.Insert("Position", vec3());
	networkData.Insert("LocalPosition", vec3());
	networkData.Insert("Velocity", vec3());
	networkData.Insert("Radius", 0.f);
}

void GameObject::Write(RakNet::RakPeerInterface* _pPeerInterface, const RakNet::SystemAddress& _address, bool _broadcast)
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_CLIENT_CLIENT_DATA);
	bs.Write(id);
	
	bs.Write(networkData.Size());

	for (const auto &i : networkData.Data())
	{
		RakNet::RakString key = i.first;
		bs.Write(key);

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

	int dataAmount = 0;
	bsIn.Read(dataAmount);

	for (int i = 0; i < dataAmount; i++)
	{
		RakNet::RakString key;
		bsIn.Read(key);

		size_t byteCount;
		bsIn.Read(byteCount);

		std::vector<unsigned char> bytes;
		bytes.resize(byteCount);

		for (int j = 0; j < (int)byteCount; j++)
		{
			bsIn.Read((char*)&bytes[j], sizeof(unsigned char));
		}
		
		networkData.SetElementBytes(key, bytes);
	}
}

void GameObject::Update(float _deltaTime)
{
	vec3 pos = networkData.GetElement<vec3>("Position");
	pos += networkData.GetElement<vec3>("Velocity") * _deltaTime;
	networkData.SetElement("Position", pos);
}

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

vec4 GameObject::GetColor(int _id)
{
	return colors[_id % 7];
}
