#pragma once
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <glm/ext.hpp>

class NetworkData
{
public:
	template <typename T>
	void Insert(const char* _Key, T _Val)
	{
		if (Contains(_Key))
			return;

		std::vector<unsigned char> data = ToBytes(_Val);
		m_data.insert({ _Key, data });
	}

	template <typename T>
	void SetElement(const char* _Key, T _Val)
	{
		for (auto& member : m_data)
		{
			if (*member.first == *_Key)
			{
				std::vector<unsigned char> data = ToBytes(_Val);
				member.second = data;
				break;
			}
		}
	}

	void SetElement(const char* _Key, std::vector<unsigned char> _Val)
	{
		for (auto& member : m_data)
		{
			if (*member.first == *_Key)
			{
				member.second = _Val;
				break;
			}
		}
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
		
		T out = FromBytes<T>(m_data[_Key]);
		return out;
	}

	bool Contains(const char* _Key)
	{
		for (auto& member : m_data)
		{
			if (*member.first == *_Key)
				return true;
		}

		return false;
	}

	std::unordered_map<const char*, std::vector<unsigned char>>& Data()
	{
		return m_data;
	}

	int Size()
	{
		return (int)m_data.size();
	}

	template<typename T>
	std::vector<unsigned char> ToBytes(T object)
	{
		std::vector<unsigned char> bytes;
		bytes.resize(sizeof(object));

		auto begin = reinterpret_cast<unsigned char*>(std::addressof(object));
		auto end = begin + sizeof(unsigned char);

		for (int i = 0; i < sizeof(object); i++)
		{
			std::copy(begin, end, &bytes[i]);

			begin = end;
			end += sizeof(unsigned char);
		}

		return bytes;
	}

	template<typename T>
	T FromBytes(std::vector<unsigned char>& bytes)
	{
		T out = T();
		
		auto begin = reinterpret_cast<unsigned char*>(std::addressof(out));
		std::copy_n(bytes.begin(), bytes.size(), begin);

		return out;
	}

protected:
	std::unordered_map<const char*, std::vector<unsigned char>> m_data;
};