#pragma once
#include <unordered_map>

class NetworkData
{
public:
	// Inserts a key-value pair of {const char*, T}, converting T to bytes
	template <typename T> void Insert(const char* _Key, T _Val)
	{
		// Already contains key, exit
		if (Contains(_Key, m_out))
			return;

		// Doesn't contain key, add it
		std::vector<unsigned char> data = ToBytes(_Val);
		m_data.insert({ _Key, data });
	}

	// Inserts a key-value pair {const char*, known vector of bytes}
	void InsertBytes(const char* _Key, std::vector<unsigned char> _Val)
	{
		// Already contains key, exit
		if (Contains(_Key, m_out))
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
		// Only erase _Key if it exists in the network data
		if (Contains(_Key, m_out))
		{
			m_data.erase(_Key);
		}
	}

	// Clears unordered_map 'm_data'
	void Clear()
	{
		m_data.clear();
	}

	// Gets element with _Key, throws if none exists
	template <typename T> T GetElement(const char* _Key)
	{
		// Doesn't contain key, return new object of type T
		if (!Contains(_Key, m_out))
		{
			return T();
		}
		
		// Does contain key, return bytes -> T
		return FromBytes<T>(m_data[_Key]);
	}

	// Checks if unordered_map 'm_data' contains _Key
	bool Contains(const char* _Key, std::pair<const char* const, std::vector<unsigned char>>*& _Out)
	{
		bool allCharsSame = false;

		for (auto& member : m_data) // loop through each data member in m_data
		{
			size_t size = strlen(member.first);

			// Strings not same length, skip to next one
			if (size != strlen(_Key))
				continue;

			// Count the number of same chars, if this count
			// is same as the length of the string they are equal
			int sameChars = 0;

			for (int i = 0; i < (int)size; i++) // loop through chars of each member name
			{
				if (member.first[i] == _Key[i]) // char is same, update count
					sameChars++;

				else break; // not same, break immediately
			}

			// Char count same, so they are the same string
			if (sameChars == (int)size)
			{
				allCharsSame = true;
				_Out = &member;
				break;
			}
		}

		// If returning false, set _Out to nullptr
		if (!allCharsSame)
			_Out = nullptr;

		return allCharsSame;
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
		// Initialise object of type T
		T out = T();

		// Guarantee that this vector is the same size as the object
		bytes.resize(sizeof(out));
		
		// The address to copy the individual bytes to
		auto copyAddr = reinterpret_cast<unsigned char*>(std::addressof(out));

		// Loop through and copy each byte to T object
		for (auto b : bytes)
		{
			// Get begin / end of this byte
			auto begin = std::addressof(b);
			auto end = begin + sizeof(unsigned char);

			// Copy it to the copy address of T object
			std::copy(begin, end, copyAddr);

			// Move copy address forward by 1 byte
			copyAddr += sizeof(unsigned char);
		}

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