#include "GameObject.h"
#include <BitStream.h>
#include "../Server/GameMessages.h"
#include <iostream>

GameObject::GameObject()
{
	networkData.Insert("Color", vec4());
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

	bs.Write((char*)&networkData, sizeof(networkData));
		_pPeerInterface->Send(&bs, HIGH_PRIORITY, 
		RELIABLE_ORDERED, 0, _address, _broadcast);
}

void GameObject::Read(RakNet::Packet* _packet)
{
	RakNet::BitStream bsIn(_packet->data, _packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
	bsIn.Read(id);
	bsIn.Read((char*)&networkData, sizeof(networkData));
}

void GameObject::Update(float _deltaTime)
{
	networkData.GetElement<vec3>("Position") 
		+= networkData.GetElement<vec3>("Velocity") * _deltaTime;
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
