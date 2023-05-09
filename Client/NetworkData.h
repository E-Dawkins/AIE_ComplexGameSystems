#pragma once
#include <any>
#include <unordered_map>
#include <glm/ext.hpp>
#include <iostream>

class NetworkData
{
public:
	void Insert(const char* _Key, std::any _Val)
	{
		if (Contains(_Key))
		{
			std::cout << "Key already defined!\n";
			throw;
		}

		data.insert({ _Key, _Val });
	}

	void SetElement(const char* _Key, std::any _Val)
	{
		if (!Contains(_Key))
		{
			Insert(_Key, _Val);
			return;
		}

		data[_Key] = _Val;
	}

	void Erase(const char* _Key)
	{
		if (!Contains(_Key))
		{
			std::cout << "Key not defined!\n";
			throw;
		}

		data.erase(_Key);
	}

	void Clear()
	{
		data.clear();
	}

	template <typename T>
	T GetElement(const char* _Key)
	{
		if (!Contains(_Key))
		{
			std::cout << "Key not defined!\n";
			throw;
		}

		return std::any_cast<T>(data[_Key]);
	}

	bool Contains(const char* _Key)
	{
		return (data.count(_Key) != 0);
	}

	std::unordered_map<const char*, std::any>& Data()
	{
		return data;
	}

protected:
	std::unordered_map<const char*, std::any> data;
};