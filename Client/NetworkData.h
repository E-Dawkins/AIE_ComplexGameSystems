#pragma once
#include <unordered_map>
#include <iostream>
#include <array>

class NetworkData
{
public:
	template <typename T>
	void Insert(const char* _Key, T _Val)
	{
		byte* bytes = ToBytes(_Val);
		auto test = FromBytes<T>(bytes);
		m_data.insert({ _Key, bytes });
	}

	template <typename T>
	void SetElement(const char* _Key, T _Val)
	{
		byte* bytes = ToBytes(_Val);
		m_data[_Key] = bytes;
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
		return m_data.contains(_Key);
	}

	std::unordered_map<const char*, const byte*>& Data()
	{
		return m_data;
	}

	int Size()
	{
		return (int)m_data.size();
	}

	template<typename T>
	byte* ToBytes(T object)
	{
		// byte bytes[sizeof(T)];
		byte* bytes = new byte[sizeof(object)];

		// --- Version 1 ---
		// auto begin = reinterpret_cast<byte*>(std::addressof(object));
		// auto end = begin + sizeof(T);
		// std::copy(begin, end, bytes);

		// --- Version 2 ---
		std::memcpy(bytes, &object, sizeof(object));
		auto test = FromBytes<T>(bytes);

		// --- Version 3 ---
		// byte* temp = reinterpret_cast<byte*>(&object);
		// std::copy(temp, temp + sizeof(object), bytes);

		return bytes;
	}

	template<typename T>
	T FromBytes(const byte* bytes)
	{
		T out = T();
		std::memcpy(&out, bytes, sizeof(T));
		return out;
	}

protected:
	std::unordered_map<const char*, const byte*> m_data;
};