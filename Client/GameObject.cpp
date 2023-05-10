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
	/*bs.Write((char*)&networkData.Data(), sizeof(networkData.Data()));*/

	std::cout << "---Before---\n";
	bs.Write(networkData.Size());

	for (auto i : networkData.Data())
	{
		RakNet::RakString dataName = i.first;
		bs.Write(dataName);
		bs.Write(i.second);
	}

	_pPeerInterface->Send(&bs, HIGH_PRIORITY, 
		RELIABLE_ORDERED, 0, _address, _broadcast);
}

void GameObject::Read(RakNet::Packet* _packet)
{
	RakNet::BitStream bsIn(_packet->data, _packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
	bsIn.Read(id);
	/*bsIn.Read((char*)&networkData.Data(), sizeof(networkData.Data()));*/

	std::cout << "---After---\n";
	unsigned int dataAmount = 0;
	bsIn.Read(dataAmount);

	std::cout << dataAmount << std::endl;

	for (int i = 0; i < dataAmount; i++)
	{
		RakNet::RakString dataName;
		bsIn.Read(dataName);

		std::any dataValue;
		bsIn.Read(dataValue);

		networkData.Insert(dataName.C_String(), dataValue);
		std::cout << dataName.C_String() << std::endl;
	}
	int a = 4;
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
