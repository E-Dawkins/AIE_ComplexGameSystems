#include "NetworkData.h"

void NetworkData::Insert(const char* _Key, std::any _Val)
{
	data.insert({_Key, _Val});
}

void NetworkData::Erase(const char* _Key)
{
	data.erase(_Key);
}

void NetworkData::Clear()
{
	data.clear();
}

template <typename T>
T NetworkData::GetElement(const char* _Key)
{
	return data[_Key];
}