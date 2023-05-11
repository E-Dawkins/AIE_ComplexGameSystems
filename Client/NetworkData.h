#pragma once
#include <any>
#include <unordered_map>
#include <glm/ext.hpp>
#include <iostream>
#include <vector>

class NetworkData
{
public:
	void Insert(const char* _Key, std::any _Val)
	{
		m_data.insert({ _Key, _Val });
	}

	void SetElement(const char* _Key, std::any _Val)
	{
		m_data[_Key] = _Val;
	}

	void Erase(const char* _Key)
	{
		if (!Contains(_Key))
		{
			std::cout << "Key not defined!\n";
			throw;
		}

		m_data.erase(_Key);
	}

	void Clear()
	{
		m_data.clear();
	}

	template <typename T>
	T GetElement(const char* _Key)
	{
		if (!Contains(_Key))
		{
			std::cout << "Key not defined!\n";
			throw;
		}

		return std::any_cast<T>(m_data[_Key]);
	}

	bool Contains(const char* _Key)
	{
		return m_data.contains(_Key);
	}

	std::unordered_map<const char*, std::any>& Data()
	{
		return m_data;
	}

	int Size()
	{
		return (int)m_data.size();
	}
	
protected:
	std::unordered_map<const char*, std::any> m_data;
};