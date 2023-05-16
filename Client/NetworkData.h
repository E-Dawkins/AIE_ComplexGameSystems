#pragma once
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <glm/ext.hpp>

class NetworkData
{
public:
	// Inserts a key-value pair of {const char*, T}, converting T to bytes
	template <typename T> void Insert(const char* _Key, T _Val)
	{
		// Already contains key, exit
		if (Contains(_Key))
			return;

		// Doesn't contain key, add it
		std::vector<unsigned char> data = ToBytes(_Val);
		m_data.insert({ _Key, data });
	}

	// Inserts a key-value pair {const char*, known vector of bytes}
	void InsertBytes(const char* _Key, std::vector<unsigned char> _Val)
	{
		// Already contains key, exit
		if (Contains(_Key))
			return;

		// Doesn't contain key, add it
		m_data.insert({ _Key, _Val });
	}

	// Sets element with _Key to _Val, if it exists, otherwise inserts it
	template <typename T> void SetElement(const char* _Key, T _Val)
	{
		// If contains key, overwrite the value...
		if (Contains(_Key, m_out))
		{
			m_out->second = ToBytes(_Val);
			return;
		}

		// ...otherwise insert it
		Insert(_Key, _Val);
	}

	// Sets element _Key to vector of bytes _Val, if it exists, otherwise inserts it
	void SetElementBytes(const char* _Key, std::vector<unsigned char> _Val)
	{
		// Already contains key, set value to _Val...
		if (Contains(_Key, m_out))
		{
			m_out->second = _Val;
			return;
		}

		// ...otherwise insert key-value pair
		InsertBytes(_Key, _Val);
	}

	// Erases element with _Key
	void Erase(const char* _Key)
	{
		// Doesn't contain key, can't erase it, throw exception
		if (!Contains(_Key))
		{
			std::cout << "Key not defined!\n";
			throw;
		}

		// Does contain key, erase the key-value pair
		m_data.erase(_Key);
	}

	// Clears unordered_map 'm_data'
	void Clear()
	{
		m_data.clear();
	}

	// Gets element with _Key, throws if none exists
	template <typename T> T GetElement(const char* _Key)
	{
		// Doesn't contain key, throw exception
		if (!Contains(_Key, m_out))
		{
			std::cout << "Key not defined!\n";
			throw;
		}
		
		// Does contain key, return bytes -> T
		return FromBytes<T>(m_out->second);
	}

	// Checks if unordered_map 'm_data' contains _Key
	bool Contains(const char* _Key)
	{
		for (auto& member : m_data)
		{
			if (*member.first == *_Key)
				return true;
		}

		return false;
	}
	
	// Checks if unordered_map 'm_data' contains _Key,
	// additionally sets _Out to contained key-value pair
	bool Contains(const char* _Key, 
		std::pair<const char* const, std::vector<unsigned char>>*& _Out)
	{
		for (auto& member : m_data)
		{
			if (*member.first == *_Key)
			{
				_Out = &member;
				return true;
			}
		}

		_Out = nullptr;
		return false;
	}

	// Returns amount of elements in unordered_map 'm_data'
	int Size()
	{
		return (int)m_data.size();
	}

	// Converts T object to vector of its' bytes (unsigned char)
	template<typename T> std::vector<unsigned char> ToBytes(T object)
	{
		// Resize vector to byte size of T object
		std::vector<unsigned char> bytes;
		bytes.resize(sizeof(object));

		// Set begin and end of first byte
		auto begin = reinterpret_cast<unsigned char*>(std::addressof(object));
		auto end = begin + sizeof(unsigned char);

		// Loop through all bytes and copy them to vector
		for (int i = 0; i < sizeof(object); i++)
		{
			std::copy(begin, end, &bytes[i]);

			// Adjust begin / end to next byte
			begin = end;
			end += sizeof(unsigned char);
		}

		return bytes;
	}

	// Converts vector of bytes (unsigned char) to T object
	template<typename T> T FromBytes(std::vector<unsigned char>& bytes)
	{
		T out = T();
		
		// Copy byte vector to T out's memory address
		auto begin = reinterpret_cast<unsigned char*>(std::addressof(out));
		std::copy_n(bytes.begin(), bytes.size(), begin);

		return out;
	}

	// Returns reference to unordered_map 'm_data'
	std::unordered_map<const char*, std::vector<unsigned char>>& Data()
	{
		return m_data;
	}

protected:
	std::unordered_map<const char*, std::vector<unsigned char>> m_data;
	std::pair<const char* const, std::vector<unsigned char>>* m_out = nullptr;
};